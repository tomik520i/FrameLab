#include "Enhancer.hpp"
#include "NrResolution.hpp"
#include <algorithm>

namespace {
void createTexture(ID3D11Device* device,Texture& texture,unsigned w,unsigned h,DXGI_FORMAT format,bool shared,HANDLE* handle){
 D3D11_TEXTURE2D_DESC d{};d.Width=w;d.Height=h;d.ArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Format=format;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
 if(format==DXGI_FORMAT_R8G8B8A8_UNORM)d.BindFlags|=D3D11_BIND_UNORDERED_ACCESS;
 if(shared)d.MiscFlags=D3D11_RESOURCE_MISC_SHARED_NTHANDLE|D3D11_RESOURCE_MISC_SHARED;
 check(device->CreateTexture2D(&d,nullptr,&texture.texture),"Enhancer texture");check(device->CreateShaderResourceView(texture.texture.Get(),nullptr,&texture.view),"Enhancer texture view");
 if(shared){ComPtr<IDXGIResource1> resource;check(texture.texture.As(&resource),"Enhancer shared resource");check(resource->CreateSharedHandle(nullptr,DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,nullptr,handle),"Enhancer shared handle");}
}
}
Enhancer::Enhancer(Graphics& graphics,unsigned width,unsigned height,unsigned maxHeight,const std::filesystem::path& app,const std::filesystem::path& runtime,unsigned passes):g(graphics),w(width),h(height),passCount(passes),runtimePath(std::filesystem::absolute(runtime)),cachePath(std::filesystem::absolute(app/L"nr-cache")){
 try{
  if(passes<1||passes>4)throw std::runtime_error("NR passes must be between 1 and 4");
  if(!std::filesystem::is_regular_file(runtimePath/L"nvngx_dlssnr.dll"))throw std::runtime_error("NR runtime missing: "+(runtimePath/L"nvngx_dlssnr.dll").string());
  const auto dimensions=nrDimensions(w,h,maxHeight);iw=dimensions.first;ih=dimensions.second;
  auto path=std::filesystem::absolute(app/L"nr-host"/L"nvngx.dll");library=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);if(!library)throw std::runtime_error("FrameLab NR adapter missing or failed to load (Win32 "+std::to_string(GetLastError())+")");
  create=reinterpret_cast<NrCreateFn>(GetProcAddress(library,"NrCreate"));process=reinterpret_cast<NrProcessFn>(GetProcAddress(library,"NrProcess"));destroy=reinterpret_cast<NrDestroyFn>(GetProcAddress(library,"NrDestroy"));if(!create||!process||!destroy)throw std::runtime_error("Incompatible FrameLab NR adapter");
  ComPtr<ID3D11Device5> device;check(g.device.As(&device),"Enhancer D3D11.4 device");check(g.context.As(&context),"Enhancer D3D11.4 context");
  createTexture(g.device.Get(),input,w,h,DXGI_FORMAT_B8G8R8A8_UNORM,true,&inputHandle);createTexture(g.device.Get(),output,w,h,DXGI_FORMAT_R8G8B8A8_UNORM,true,&outputHandle);
  createTexture(g.device.Get(),original,w,h,DXGI_FORMAT_B8G8R8A8_UNORM,false,nullptr);createTexture(g.device.Get(),result,w,h,DXGI_FORMAT_R8G8B8A8_UNORM,false,nullptr);
  check(device->CreateFence(0,D3D11_FENCE_FLAG_SHARED,IID_PPV_ARGS(&inputFence)),"Enhancer input fence");check(inputFence->CreateSharedHandle(nullptr,GENERIC_ALL,nullptr,&fenceHandle),"Enhancer fence handle");
  ComPtr<IDXGIDevice> dx;check(g.device.As(&dx),"Enhancer DXGI device");ComPtr<IDXGIAdapter> adapter;check(dx->GetAdapter(&adapter),"Enhancer adapter");DXGI_ADAPTER_DESC desc{};check(adapter->GetDesc(&desc),"Enhancer adapter identity");
  NrConfig c{};c.passes=passCount;c.adapter=desc.AdapterLuid;c.width=w;c.height=h;c.inferenceWidth=iw;c.inferenceHeight=ih;c.input=inputHandle;c.output=outputHandle;c.inputFence=fenceHandle;c.runtimeDirectory=runtimePath.c_str();c.cacheDirectory=cachePath.c_str();
  worker=std::thread([this,c]{run(c);});
 }catch(const std::exception& e){fail(e.what());finished=true;}
}
void Enhancer::fail(const std::string& text){std::lock_guard lock(mutex);failure=text;phase=State::Failed;}
std::string Enhancer::error()const{std::lock_guard lock(mutex);return failure;}
void Enhancer::run(NrConfig c){
 char error[768]{};void* host=nullptr;
 try{
  host=create(&c,error,sizeof(error));if(!host)throw std::runtime_error(error);
  phase=State::Idle;
  for(;;){NrFrame frame;{std::unique_lock lock(mutex);cv.wait(lock,[&]{return stop||queued;});if(stop)break;frame=job;queued=false;}
   float ms=0;if(!process(host,&frame,&ms,error,sizeof(error)))throw std::runtime_error(error);
   elapsedMs=ms;phase=State::Ready;
  }
 }catch(const std::exception& e){fail(e.what());}
 if(host)destroy(host);if(phase!=State::Failed)phase=State::Stopped;finished=true;
}
bool Enhancer::submit(ID3D11Texture2D* source,bool reset,bool motion){
 if(phase!=State::Idle)return false;
 D3D11_TEXTURE2D_DESC d{};source->GetDesc(&d);if(d.Width!=w||d.Height!=h||d.Format!=DXGI_FORMAT_B8G8R8A8_UNORM)throw std::runtime_error("Enhancer source dimensions/format changed");
 g.context->CopyResource(input.texture.Get(),source);check(context->Signal(inputFence.Get(),++sequence),"Enhancer source signal");g.context->Flush();
 {std::lock_guard lock(mutex);if(stop)return false;job={sequence,reset,motion};queued=true;phase=State::Working;}cv.notify_one();return true;
}
bool Enhancer::take(){
 if(phase!=State::Ready)return false;
 // Worker observed D3D12 completion before publishing Ready. A subsequent
 // input-fence signal orders these copies before the next inference write.
 g.context->CopyResource(original.texture.Get(),input.texture.Get());g.context->CopyResource(result.texture.Get(),output.texture.Get());hasResult=true;phase=State::Idle;return true;
}
void Enhancer::requestStop(){std::lock_guard lock(mutex);stop=true;cv.notify_all();}
Enhancer::~Enhancer(){requestStop();if(worker.joinable())worker.join();if(fenceHandle)CloseHandle(fenceHandle);if(outputHandle)CloseHandle(outputHandle);if(inputHandle)CloseHandle(inputHandle);if(library)FreeLibrary(library);}
