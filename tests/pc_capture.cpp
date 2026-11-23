#include "WindowCapture.hpp"
#include "DisplayAwake.hpp"
#include "GpuSelection.hpp"
#include <Windows.h>
#include <chrono>
#include <atomic>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>

static int tick=0,clicks=0;static bool fixtureRunning=true;
LRESULT CALLBACK fixture(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_TIMER){++tick;InvalidateRect(h,nullptr,FALSE);return 0;}
 if(m==WM_LBUTTONDOWN){++clicks;InvalidateRect(h,nullptr,FALSE);return 0;}
 if(m==WM_PAINT){PAINTSTRUCT ps{};auto dc=BeginPaint(h,&ps);RECT r{};GetClientRect(h,&r);auto bg=CreateSolidBrush(RGB(20,40,60));FillRect(dc,&r,bg);DeleteObject(bg);
  RECT box{30+(tick*5)%std::max(1,int(r.right-140)),100,110+(tick*5)%std::max(1,int(r.right-140)),180};auto green=CreateSolidBrush(RGB(80,230,40));FillRect(dc,&box,green);DeleteObject(green);
  SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));std::wstring text=L"FrameLab test source | Clicks: "+std::to_wstring(clicks)+L" | Frame: "+std::to_wstring(tick);TextOutW(dc,25,25,text.c_str(),int(text.size()));std::wstring caption=L"John, wait! The code is 1234.";TextOutW(dc,45,r.bottom-70,caption.c_str(),int(caption.size()));EndPaint(h,&ps);return 0;}
 if(m==WM_CLOSE){fixtureRunning=false;DestroyWindow(h);return 0;}return DefWindowProcW(h,m,w,l);
}
void pump(int ms){auto until=GetTickCount64()+ms;while(GetTickCount64()<until){MSG m;while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}Sleep(2);}}
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
int wmain(int argc,wchar_t** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 WNDCLASSW c{};c.lpfnWndProc=fixture;c.hInstance=GetModuleHandleW(nullptr);c.lpszClassName=L"FrameLabCaptureFixture";c.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&c);
 HWND source=CreateWindowW(c.lpszClassName,L"FrameLab Capture Test Source",WS_OVERLAPPEDWINDOW|WS_VISIBLE,80,80,800,520,nullptr,nullptr,c.hInstance,nullptr);SetTimer(source,1,33,nullptr);pump(200);
 try{
  if(argc>1&&std::wstring(argv[1])==L"--fixture"){
   std::wofstream(argc>2?argv[2]:L"fixture-hwnd.txt")<<reinterpret_cast<uintptr_t>(source);auto until=GetTickCount64()+600000;while(fixtureRunning&&GetTickCount64()<until)pump(50);return 0;
  }
  {DisplayAwake awake;awake.update(true);require(awake.active()&&!awake.error(),"Power request failed");auto old=SetThreadExecutionState(ES_CONTINUOUS);require((old&(ES_DISPLAY_REQUIRED|ES_SYSTEM_REQUIRED))==(ES_DISPLAY_REQUIRED|ES_SYSTEM_REQUIRED),"Power request not set");SetThreadExecutionState(old);awake.update(false);old=SetThreadExecutionState(ES_CONTINUOUS);require(!(old&(ES_DISPLAY_REQUIRED|ES_SYSTEM_REQUIRED)),"Power request not released");}
  {DisplayAwake awake;awake.update(true);}require(!(SetThreadExecutionState(ES_CONTINUOUS)&(ES_DISPLAY_REQUIRED|ES_SYSTEM_REQUIRED)),"Destructor leaked power request");
  GpuSelection list;list.load(L"nonexistent-gpu-settings.test");
  for(const auto& adapter:list.items){
   std::cerr<<"start adapter "<<adapter.index<<std::endl;
   WindowCapture capture;bool rejected=false;try{capture.start(source,adapter.index,60,[](auto&){});}catch(...){rejected=true;}require(rejected,"Own-window recursion not blocked");
   std::atomic<int> bad=0,changed=0,phase=0;std::atomic<unsigned> latestW=0;uint64_t lastSum=0;
   capture.start(source,adapter.index,60,[&](const auto& f){
    latestW=f.width;auto first=f.data+size_t(f.contentTop)*f.stride+f.contentLeft*4;
    if(abs(int(first[0])-60)>3||abs(int(first[1])-40)>3||abs(int(first[2])-20)>3){++bad;if(bad<=5)std::cerr<<"bad pixel phase="<<phase<<" bgr="<<int(first[0])<<","<<int(first[1])<<","<<int(first[2])<<" size="<<f.width<<"x"<<f.height<<std::endl;}
    uint64_t sum=0;for(unsigned y=80;y<std::min(f.height,220u);y+=5)for(unsigned x=0;x<f.width;x+=3)sum+=first[size_t(y)*f.stride+x*4+1]*uint64_t(x+1);if(lastSum&&sum!=lastSum)++changed;lastSum=sum;
   },true);
   pump(2200);std::cerr<<"initial frames "<<capture.frames()<<" error "<<capture.error()<<std::endl;require(capture.active(),capture.error().c_str());require(capture.frames()>10,"No window capture frames");require(bad==0,"Wrong client crop / incorrect colors");auto before=changed.load();
   phase=1;HWND cover=CreateWindowW(L"STATIC",L"OCCLUDER",WS_POPUP|WS_VISIBLE,65,65,850,600,nullptr,nullptr,c.hInstance,nullptr);SetWindowPos(cover,HWND_TOPMOST,65,65,850,600,0);pump(1300);require(changed>before+5,"Capture froze behind another window");DestroyWindow(cover);
   phase=2;SetWindowPos(source,nullptr,80,80,960,600,SWP_NOZORDER);pump(1300);RECT client{};GetClientRect(source,&client);require(latestW==unsigned(client.right),"Capture resize failed");
   phase=3;ShowWindow(source,SW_MINIMIZE);pump(300);require(capture.paused(),"Minimized capture not paused");phase=4;ShowWindow(source,SW_RESTORE);pump(800);require(!capture.paused(),"Capture did not resume");
   std::cout<<"{\"adapter\":"<<adapter.index<<",\"gpu\":\""<<adapter.name<<"\",\"frames\":"<<capture.frames()<<",\"changes\":"<<changed<<",\"bad_crop\":"<<bad<<",\"passed\":true}"<<std::endl;
   capture.stop();require(bad==0,"Invalid client pixels during window transitions");require(!capture.active(),"Capture stop failed");
   list.selected=adapter.index;auto path=std::filesystem::temp_directory_path()/L"framelab-gpu-selection-test.txt";require(list.save(path),"GPU preference save failed");GpuSelection reread;reread.load(path);require(reread.selected==adapter.index,"GPU LUID preference mismatch");std::filesystem::remove(path);
  }
  WindowCapture closed;closed.start(source,list.items.front().index,30,[](auto&){},true);pump(300);DestroyWindow(source);pump(300);require(!closed.active()&&!closed.error().empty(),"Closed source not detected");
  std::cout<<"{\"power_lifecycle\":true,\"gpu_selection\":true,\"closed_source\":true}"<<std::endl;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;DestroyWindow(source);CoUninitialize();return 1;}CoUninitialize();return 0;
}
