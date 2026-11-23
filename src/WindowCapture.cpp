#include "WindowCapture.hpp"
#include "Graphics.hpp"
#include <dwmapi.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <algorithm>
#include <chrono>
using namespace winrt::Windows::Graphics;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
bool WindowCapture::supported(){try{return GraphicsCaptureSession::IsSupported();}catch(...){return false;}}
std::vector<WindowCapture::Window> WindowCapture::enumerate(){
 std::vector<Window> result;EnumWindows([](HWND h,LPARAM p)->BOOL{
  DWORD pid=0;GetWindowThreadProcessId(h,&pid);DWORD cloaked=0;DwmGetWindowAttribute(h,DWMWA_CLOAKED,&cloaked,sizeof(cloaked));
  if(pid==GetCurrentProcessId()||!IsWindowVisible(h)||cloaked||!GetWindowTextLengthW(h)||(GetWindowLongPtrW(h,GWL_EXSTYLE)&WS_EX_TOOLWINDOW))return TRUE;
  RECT rc{};GetClientRect(h,&rc);if(rc.right<32||rc.bottom<32)return TRUE;
  wchar_t name[1024]{};GetWindowTextW(h,name,1024);char utf8[4096]{};WideCharToMultiByte(CP_UTF8,0,name,-1,utf8,sizeof(utf8),nullptr,nullptr);
  reinterpret_cast<std::vector<Window>*>(p)->push_back({h,pid,utf8});return TRUE;
 },reinterpret_cast<LPARAM>(&result));return result;
}
void WindowCapture::stop(){
 if(worker_.joinable()){
  worker_.request_stop();HANDLE thread=worker_.native_handle();bool quit=false;WPARAM quitCode=0;
  // WGC creation/teardown can send messages to the captured HWND's owning
  // thread. A blind join on that thread deadlocks source replacement/closing.
  while(WaitForSingleObject(thread,0)==WAIT_TIMEOUT){
   if(MsgWaitForMultipleObjectsEx(1,&thread,20,QS_ALLINPUT,MWMO_INPUTAVAILABLE)==WAIT_OBJECT_0+1){MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){if(message.message==WM_QUIT){quit=true;quitCode=message.wParam;}else{TranslateMessage(&message);DispatchMessageW(&message);}}}
  }
  worker_.join();if(quit)PostQuitMessage(int(quitCode));
 }
 active_=paused_=false;target_=nullptr;process_=0;
}
void WindowCapture::start(HWND target,int adapter,int fps,DirectShowCapture::FrameHandler handler,bool allowOwnProcessForTest){
 stop();DWORD pid=0;GetWindowThreadProcessId(target,&pid);
 if(!IsWindow(target)||!pid||(!allowOwnProcessForTest&&pid==GetCurrentProcessId()))throw std::runtime_error("Choose another application's window, not FrameLab itself");
 if(!supported())throw std::runtime_error("Windows Graphics Capture is unavailable (Windows 10 1903+ required)");
 target_=target;process_=pid;rate_=std::clamp(fps,1,240);frames_=0;{std::lock_guard lock(mutex_);error_.clear();}active_=true;
 worker_=std::jthread([this,target,pid,adapter,handler=std::move(handler)](std::stop_token stop){
  winrt::init_apartment(winrt::apartment_type::multi_threaded);
  try{
   Graphics g;g.initDevice(adapter);ComPtr<IDXGIDevice> dxgi;check(g.device.As(&dxgi),"Capture DXGI device");
   winrt::com_ptr<IInspectable> inspectable;winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(),inspectable.put()));auto device=inspectable.as<IDirect3DDevice>();
   auto interop=winrt::get_activation_factory<GraphicsCaptureItem,IGraphicsCaptureItemInterop>();GraphicsCaptureItem item{nullptr};winrt::check_hresult(interop->CreateForWindow(target,winrt::guid_of<GraphicsCaptureItem>(),winrt::put_abi(item)));
   auto size=item.Size();auto pool=Direct3D11CaptureFramePool::CreateFreeThreaded(device,DirectXPixelFormat::B8G8R8A8UIntNormalized,2,size);auto session=pool.CreateCaptureSession(item);
   if(winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession",L"IsCursorCaptureEnabled"))session.IsCursorCaptureEnabled(false);
   session.StartCapture();ComPtr<ID3D11Texture2D> staging;D3D11_TEXTURE2D_DESC desc{};
   auto due=std::chrono::steady_clock::now();const auto interval=std::chrono::nanoseconds(1000000000ll/rate_);
   while(!stop.stop_requested()){
    DWORD currentPid=0;GetWindowThreadProcessId(target,&currentPid);if(!IsWindow(target)||currentPid!=pid)throw std::runtime_error("Captured window was closed. Select a window again.");
    paused_=IsIconic(target)||!IsWindowVisible(target);if(paused_)due=std::chrono::steady_clock::now()+std::chrono::milliseconds(250);auto frame=pool.TryGetNextFrame();if(!frame){Sleep(2);continue;}
    auto content=frame.ContentSize();if(content.Width<=0||content.Height<=0){frame.Close();continue;}
    if(content.Width!=size.Width||content.Height!=size.Height){frame.Close();size=content;pool.Recreate(device,DirectXPixelFormat::B8G8R8A8UIntNormalized,2,size);staging.Reset();due=std::chrono::steady_clock::now()+std::chrono::milliseconds(100);continue;}
    auto time=std::chrono::steady_clock::now();if(paused_||time+std::chrono::milliseconds(1)<due){frame.Close();Sleep(1);continue;}due=std::max(due+interval,time);
    auto surface=frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();ComPtr<ID3D11Texture2D> texture;winrt::check_hresult(surface->GetInterface(IID_PPV_ARGS(&texture)));texture->GetDesc(&desc);
    if(!staging){auto d=desc;d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.MiscFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;check(g.device->CreateTexture2D(&d,nullptr,&staging),"Capture readback texture");}
    RECT bounds{},client{};if(FAILED(DwmGetWindowAttribute(target,DWMWA_EXTENDED_FRAME_BOUNDS,&bounds,sizeof(bounds))))GetWindowRect(target,&bounds);GetClientRect(target,&client);POINT origin{};ClientToScreen(target,&origin);
    // Client-only pixels keep the overlay aligned with the target's input coordinates.
    int left=origin.x-bounds.left,top=origin.y-bounds.top,cw=client.right,ch=client.bottom;
    if(left<0||top<0||cw<=0||ch<=0||left+cw>content.Width||top+ch>content.Height){frame.Close();continue;}
    g.context->CopyResource(staging.Get(),texture.Get());D3D11_MAPPED_SUBRESOURCE map{};check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"Capture frame readback");
    DirectShowCapture::Frame f{};f.width=cw;f.height=ch;f.sampleWidth=desc.Width;f.sampleHeight=desc.Height;f.contentLeft=left;f.contentTop=top;f.stride=map.RowPitch;f.data=static_cast<const unsigned char*>(map.pData);f.dataSize=size_t(map.RowPitch)*desc.Height;f.timestamp100ns=frame.SystemRelativeTime().count();
    try{handler(f);++frames_;}catch(...){g.context->Unmap(staging.Get(),0);throw;}g.context->Unmap(staging.Get(),0);frame.Close();
   }
   session.Close();pool.Close();
  }catch(const winrt::hresult_error& e){std::lock_guard lock(mutex_);error_=winrt::to_string(e.message());}catch(const std::exception& e){std::lock_guard lock(mutex_);error_=e.what();}
  active_=false;winrt::uninit_apartment();
 });
}
