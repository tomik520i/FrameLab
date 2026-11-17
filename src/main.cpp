#include "Graphics.hpp"
#include "FramePacer.hpp"
#include "DisplayVerification.hpp"
#include "Localization.hpp"
#include "FrameGenerator.hpp"
#include "Comparison.hpp"
#include "AnalysisRecorder.hpp"
#include <shellapi.h>
#include "VideoBenchmark.hpp"
#include "DirectShowCapture.hpp"
#include "DeviceEnumeration.hpp"
#include "CaptureModes.hpp"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <mmsystem.h>
#include <atomic>
#include <array>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <functional>
#include <exception>
using Clock=std::chrono::steady_clock;
double now(){return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();}
std::ofstream trace;
void log(const std::string& s){trace<<std::fixed<<std::setprecision(4)<<now()<<" "<<s<<std::endl;}
struct Packet {std::vector<unsigned char> data;unsigned w=0,h=0;double received=0;uint64_t serial=0;};
struct Mailbox {
 AnalysisRecorder* recorder=nullptr;
 std::mutex mutex;Packet latest;bool available=false;std::vector<unsigned char> signature,previousPixels,normalized;
 std::atomic<uint64_t> raw=0,unique=0,dropped=0,invalid=0;std::atomic<double> lastArrival=0,sourceInterval=1./30;double lastChangedArrival=0;
 std::atomic<float> threshold=0.f;std::atomic<bool> dedup=true;unsigned sw=0,sh=0;
 void ingest(const DirectShowCapture::Frame& f){
  const double arrival=now();lastArrival=arrival;++raw;
  const unsigned fullH=f.sampleHeight?f.sampleHeight:f.height;
  if(!f.data||!f.width||!f.height||f.stride<f.width*4||f.dataSize<size_t(f.stride)*fullH||f.contentLeft+f.width>f.stride/4||f.contentTop+f.height>fullH){++invalid;return;}
  auto sourceRow=[&](unsigned y){const auto sy=f.contentTop+y;return f.data+size_t(f.bottomUp?fullH-1-sy:sy)*f.stride+f.contentLeft*4;};
  // DirectShow MEDIASUBTYPE_RGB32 is BGRX, not BGRA. X is undefined.
  // Normalize before both dedup and rendering so X cannot hide the picture
  // or turn identical RGB frames into false changes.
  normalized.resize(size_t(f.width)*f.height*4);
  for(unsigned y=0;y<f.height;++y){auto src=sourceRow(y);auto dst=normalized.data()+size_t(y)*f.width*4;for(unsigned x=0;x<f.width;++x){uint32_t pixel;memcpy(&pixel,src+x*4,4);pixel|=0xff000000u;memcpy(dst+x*4,&pixel,4);}}
  if(recorder)recorder->raw(normalized.data(),f.width,f.height,raw.load(),arrival);
  auto row=[&](unsigned y){return normalized.data()+size_t(y)*f.width*4;};
  if(raw.load()==1){uint64_t alpha=0,rgb=0;unsigned count=0;for(unsigned y=0;y<f.height;y+=16){auto src=sourceRow(y);for(unsigned x=0;x<f.width;x+=16){alpha+=src[x*4+3];rgb+=src[x*4]+src[x*4+1]+src[x*4+2];++count;}}std::ofstream("logs/input-format.json")<<"{\"raw_x_byte_mean\":"<<double(alpha)/count<<",\"rgb_mean\":"<<double(rgb)/(3*count)<<",\"alpha_forced_opaque\":true}\n";}
  // Exact comparison by default: sparse probes alone miss small moving objects.
  // Optional noise tolerance is approximate and explicitly exposed as such.
  std::vector<unsigned char> sample;sample.reserve(96*54*3);
  for(unsigned gy=0;gy<54;++gy){auto r=row(std::min(f.height-1,(gy*f.height+f.height/2)/54));for(unsigned gx=0;gx<96;++gx){auto x=std::min(f.width-1,(gx*f.width+f.width/2)/96);for(int c=0;c<3;++c)sample.push_back(r[x*4+c]);}}
  bool changed=signature.size()!=sample.size()||sw!=f.width||sh!=f.height;
  if(!changed&&threshold.load()>0){double sad=0;unsigned significant=0;for(size_t i=0;i<sample.size();++i){int d=std::abs(int(sample[i])-int(signature[i]));sad+=d;if(d>8)++significant;}changed=sad/sample.size()>threshold.load()||significant>sample.size()/500;}
  else if(!changed){changed=previousPixels.size()!=size_t(f.width)*f.height*4;for(unsigned y=0;!changed&&y<f.height;++y)changed=memcmp(previousPixels.data()+size_t(y)*f.width*4,row(y),f.width*4)!=0;}
  if(changed){previousPixels.resize(size_t(f.width)*f.height*4);for(unsigned y=0;y<f.height;++y)memcpy(previousPixels.data()+size_t(y)*f.width*4,row(y),f.width*4);}
  signature=std::move(sample);sw=f.width;sh=f.height;if(changed){if(lastChangedArrival>0){double gap=arrival-lastChangedArrival;if(gap>1./240&&gap<.12)sourceInterval=sourceInterval.load()*.8+gap*.2;}lastChangedArrival=arrival;++unique;}
  if(!changed&&dedup)return;
  std::lock_guard lock(mutex);if(available)++dropped;latest.data.resize(size_t(f.width)*f.height*4);
  for(unsigned y=0;y<f.height;++y)memcpy(latest.data.data()+size_t(y)*f.width*4,row(y),f.width*4);
  latest.w=f.width;latest.h=f.height;latest.received=arrival;latest.serial=raw;available=true;
 }
 bool take(Packet& p){std::lock_guard lock(mutex);if(!available)return false;std::swap(p,latest);available=false;return true;}
 void clear(){std::lock_guard lock(mutex);available=false;signature.clear();sw=sh=0;raw=0;unique=0;dropped=0;invalid=0;lastArrival=0;lastChangedArrival=0;sourceInterval=1./30;}
};
struct GpuMonitor {
 HMODULE dll=nullptr;void* device=nullptr;float utilization=-1,memoryMB=-1,temp=-1,power=-1;char name[96]="NVIDIA GPU";
 struct Util{unsigned gpu,memory;};struct Memory{unsigned long long total,free,used;};
 int(__cdecl* shutdown)()=nullptr;int(__cdecl* util)(void*,Util*)=nullptr;int(__cdecl* memory)(void*,Memory*)=nullptr;
 int(__cdecl* temperature)(void*,unsigned,unsigned*)=nullptr;int(__cdecl* watts)(void*,unsigned*)=nullptr;
 GpuMonitor(){dll=LoadLibraryExW(L"nvml.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!dll)return;
  auto init=reinterpret_cast<int(__cdecl*)()>(GetProcAddress(dll,"nvmlInit_v2"));auto get=reinterpret_cast<int(__cdecl*)(unsigned,void**)>(GetProcAddress(dll,"nvmlDeviceGetHandleByIndex_v2"));
  if(!init||!get||init()!=0||get(0,&device)!=0)return;
  shutdown=reinterpret_cast<decltype(shutdown)>(GetProcAddress(dll,"nvmlShutdown"));util=reinterpret_cast<decltype(util)>(GetProcAddress(dll,"nvmlDeviceGetUtilizationRates"));memory=reinterpret_cast<decltype(memory)>(GetProcAddress(dll,"nvmlDeviceGetMemoryInfo"));
  temperature=reinterpret_cast<decltype(temperature)>(GetProcAddress(dll,"nvmlDeviceGetTemperature"));watts=reinterpret_cast<decltype(watts)>(GetProcAddress(dll,"nvmlDeviceGetPowerUsage"));
  auto getName=reinterpret_cast<int(__cdecl*)(void*,char*,unsigned)>(GetProcAddress(dll,"nvmlDeviceGetName"));if(getName)getName(device,name,sizeof(name));
 }
 void poll(){if(!device)return;Util u{};Memory m{};unsigned v;if(util&&util(device,&u)==0)utilization=float(u.gpu);if(memory&&memory(device,&m)==0)memoryMB=float(m.used/1048576.0);if(temperature&&temperature(device,0,&v)==0)temp=float(v);if(watts&&watts(device,&v)==0)power=v/1000.f;}
 ~GpuMonitor(){if(shutdown)shutdown();if(dll)FreeLibrary(dll);}
};
void verifyCaptureCore(const std::filesystem::path& root){
 Mailbox box;std::vector<unsigned char> data(16*16*4,0);for(size_t i=3;i<data.size();i+=4)data[i]=255;
 DirectShowCapture::Frame f{};f.width=f.sampleWidth=16;f.height=f.sampleHeight=16;f.stride=64;f.data=data.data();f.dataSize=data.size();
 Packet p;box.ingest(f);if(!box.take(p))throw std::runtime_error("Core test: missing first frame");
 for(int i=0;i<20;++i)box.ingest(f);if(box.unique!=1||box.raw!=21||box.take(p))throw std::runtime_error("Core test: duplicate counted as new content");
 data[4*(7*16+3)+1]=200;box.ingest(f);if(box.unique!=2||!box.take(p))throw std::runtime_error("Core test: small motion missed");
 f.bottomUp=true;box.ingest(f);if(!box.take(p)||p.data[4*(8*16+3)+1]!=200)throw std::runtime_error("Core test: bottom-up conversion");
 f.dataSize=2;box.ingest(f);if(box.invalid!=1)throw std::runtime_error("Core test: malformed frame accepted");
 f.dataSize=data.size();for(size_t i=3;i<data.size();i+=4)data[i]=0;box.ingest(f);if(box.take(p))throw std::runtime_error("Core test: undefined X byte counted as motion");
 data[0]=123;box.ingest(f);if(!box.take(p)||p.data[3]!=255)throw std::runtime_error("Core test: RGB32 not made opaque");
 std::ofstream(root/L"logs"/L"capture-core-verification.json")<<"{\"passed\":true,\"checks\":[\"exact duplicates\",\"single pixel change\",\"bottom-up image\",\"invalid sample\"]}\n";
}
struct History {
 static constexpr int N=180;std::array<float,N> values{};int pos=0;void add(float v){values[pos]=v;pos=(pos+1)%N;}
};
void chart(const char* id,History& a,History* b,History* c,float maxY,float height){
 ImVec2 origin=ImGui::GetCursorScreenPos();float width=ImGui::GetContentRegionAvail().x;
 ImGui::InvisibleButton(id,ImVec2(width,height));auto d=ImGui::GetWindowDrawList();d->AddRectFilled(origin,ImVec2(origin.x+width,origin.y+height),IM_COL32(13,20,32,255),8);
 for(int i=1;i<4;++i){float y=origin.y+height*i/4;d->AddLine(ImVec2(origin.x,y),ImVec2(origin.x+width,y),IM_COL32(42,53,72,130));}
 auto line=[&](History& h,ImU32 color){ImVec2 prev{};for(int i=0;i<History::N;++i){float v=h.values[(h.pos+i)%History::N];ImVec2 p(origin.x+width*i/(History::N-1),origin.y+height-std::clamp(v/maxY,0.f,1.f)*(height-8)-4);if(i)d->AddLine(prev,p,color,1.8f);prev=p;}};
 line(a,IM_COL32(65,220,190,255));if(b)line(*b,IM_COL32(255,190,90,255));if(c)line(*c,IM_COL32(120,155,255,255));
}
void metric(const char* label,float value,const char* unit,ImVec4 color){ImGui::TextDisabled("%s",label);ImGui::PushStyleColor(ImGuiCol_Text,color);ImGui::SetWindowFontScale(1.55f);if(value<0)ImGui::TextUnformatted("--");else ImGui::Text("%.1f %s",value,unit);ImGui::SetWindowFontScale(1);ImGui::PopStyleColor();}
void verifyInterpolation(Graphics& g,const std::filesystem::path& root,unsigned multiplier=2,unsigned index=1,bool optical=false){
 const unsigned w=1280,h=720;Texture output;output.create(g.device.Get(),w,h);FrameGenerator fg;fg.useOptical=optical;fg.initialize(g.device.Get(),output.texture.Get(),w,h,0,root/L"runtime");
 std::vector<unsigned char> image(size_t(w)*h*4);
 for(unsigned frame=0;frame<2;++frame){for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){bool square=x>=320+64*frame&&x<480+64*frame&&y>=250&&y<410;size_t i=(size_t(y)*w+x)*4;image[i]=square?60:20;image[i+1]=square?230:15;image[i+2]=square?40:10;image[i+3]=255;}fg.upload(image.data(),frame);}
 fg.generate(1,false,index,multiplier);double deadline=now()+20;while(!fg.ready()){if(now()>deadline)throw std::runtime_error("VFG test timed out");Sleep(1);}
 // Diagnostic readback ONLY; the live path never downloads generated pixels.
 D3D11_TEXTURE2D_DESC desc{};output.texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ComPtr<ID3D11Texture2D> readback;check(g.device->CreateTexture2D(&desc,nullptr,&readback),"Test readback");g.context->CopyResource(readback.Get(),output.texture.Get());
 D3D11_MAPPED_SUBRESOURCE map{};check(g.context->Map(readback.Get(),0,D3D11_MAP_READ,0,&map),"Test map");double sum=0,weighted=0,blendDifference=0;
 for(unsigned y=0;y<h;++y){auto row=(const unsigned char*)map.pData+size_t(y)*map.RowPitch;for(unsigned x=0;x<w;++x){double weight=std::max(0,int(row[x*4+1])-15);sum+=weight;weighted+=weight*x;bool a=x>=320&&x<480&&y>=250&&y<410,b=x>=384&&x<544&&y>=250&&y<410;double blend=15+215*((a?1.:0)*(1.-double(index)/multiplier)+(b?1.:0)*double(index)/multiplier);blendDifference+=std::abs(row[x*4+1]-blend);}}
 g.context->Unmap(readback.Get(),0);double center=sum?weighted/sum:-1;double expected=399.5+64.*index/multiplier;bool passed=std::abs(center-expected)<4&&sum>160*160*215*.7&&sum<160*160*215*1.3&&blendDifference>1000;
 std::ofstream report(root/L"logs"/(std::string(optical?"optical-verification-":"vfg-verification-")+std::to_string(multiplier)+"-"+std::to_string(index)+".json"));report<<"{\"passed\":"<<(passed?"true":"false")<<",\"previous_center\":399.5,\"current_center\":463.5,\"expected_position\":"<<expected<<",\"generated_center\":"<<center<<",\"different_from_linear_blend_sum\":"<<blendDifference<<",\"gpu_ms\":"<<fg.gpuMs()<<"}\n";
 log("VFG pixel verification center="+std::to_string(center)+" passed="+std::to_string(passed));if(!passed)throw std::runtime_error("VFG output did not contain the expected intermediate position");
}
Graphics* graphics=nullptr;bool resized=false;unsigned resizeW=0,resizeH=0;bool running=true,showPanel=true,fullscreen=false,imageOnly=false;
// DefWindowProc owns the thread during an interactive resize. Keep the same
// render tick alive from that modal loop; never render recursively.
std::function<void()> modalFrame;std::exception_ptr modalFailure;
bool inSizeMove=false,frameBusy=false;uint64_t modalFrames=0;
constexpr UINT_PTR resizeTimer=0xF17;
void renderModalFrame() noexcept {
 if(!modalFrame||frameBusy||!running)return;
 try {modalFrame();++modalFrames;}catch(...){modalFailure=std::current_exception();running=false;}
}
struct FrameTickBinding {
 HWND window;
 FrameTickBinding(HWND h,std::function<void()> tick):window(h){modalFailure=nullptr;modalFrame=std::move(tick);}
 ~FrameTickBinding(){KillTimer(window,resizeTimer);modalFrame={};inSizeMove=false;}
};
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
LRESULT WINAPI windowProc(HWND h,UINT msg,WPARAM w,LPARAM l){
 if(ImGui_ImplWin32_WndProcHandler(h,msg,w,l))return 1;
 if(msg==WM_ENTERSIZEMOVE){inSizeMove=true;modalFrames=0;SetTimer(h,resizeTimer,16,nullptr);log("Interactive resize/move entered");return 0;}
 if(msg==WM_EXITSIZEMOVE){KillTimer(h,resizeTimer);inSizeMove=false;RECT rc{};GetClientRect(h,&rc);resizeW=rc.right;resizeH=rc.bottom;resized=true;renderModalFrame();log("Interactive resize/move exited; live ticks="+std::to_string(modalFrames));return 0;}
 if(msg==WM_TIMER&&w==resizeTimer){renderModalFrame();return 0;}
 if(msg==WM_SIZE&&w!=SIZE_MINIMIZED){resized=true;resizeW=LOWORD(l);resizeH=HIWORD(l);if(inSizeMove)renderModalFrame();return 0;}
 if(msg==WM_ERASEBKGND)return 1;
 if(msg==WM_CLOSE){running=false;return 0;}if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}
 if(msg==WM_KEYDOWN&&w==VK_ESCAPE&&imageOnly){SendMessageW(h,WM_KEYDOWN,VK_F11,0);return 0;}
 if(msg==WM_KEYDOWN&&w==VK_F1){showPanel=!showPanel;return 0;}
 if(msg==WM_KEYDOWN&&w==VK_F11){static RECT old{};fullscreen=!fullscreen;imageOnly=fullscreen;if(fullscreen){GetWindowRect(h,&old);MONITORINFO m{sizeof(m)};GetMonitorInfo(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&m);SetWindowLongPtr(h,GWL_STYLE,WS_POPUP|WS_VISIBLE);SetWindowPos(h,HWND_TOP,m.rcMonitor.left,m.rcMonitor.top,m.rcMonitor.right-m.rcMonitor.left,m.rcMonitor.bottom-m.rcMonitor.top,SWP_FRAMECHANGED);}else{SetWindowLongPtr(h,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE);SetWindowPos(h,nullptr,old.left,old.top,old.right-old.left,old.bottom-old.top,SWP_FRAMECHANGED|SWP_NOZORDER);}return 0;}
 return DefWindowProc(h,msg,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR cmd,int){
 wchar_t exe[MAX_PATH];GetModuleFileNameW(nullptr,exe,MAX_PATH);const auto root=std::filesystem::path(exe).parent_path();SetCurrentDirectoryW(root.c_str());
 std::filesystem::create_directories(root/L"logs");auto session=std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
 trace.open(root/L"logs"/(session+".log"));std::ofstream csv(root/L"logs"/(session+".csv"));
 csv<<"elapsed_s,raw_input_fps,content_change_fps,content_submissions_fps,present_calls_fps,generated_fps,callback_to_present_mean_ms,callback_to_present_p95_ms,vfg_gpu_ms,gpu_total_percent,vram_total_mb,mailbox_drops,invalid_samples,os_observed_content_fps,presentation_queue,presentation_drops,os_observed_generated_fps,display_mode_hz\n";
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);timeBeginPeriod(1);SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 int result=0;
 try{
  std::wstring args=cmd;
  if(args.find(L"--verify-core")!=std::wstring::npos){verifyCaptureCore(root);timeEndPeriod(1);CoUninitialize();return 0;}
  if(args.find(L"--verify-devices")!=std::wstring::npos){std::ofstream out(root/L"logs"/L"device-modes.json");out<<"[";bool first=true;for(auto& d:enumerateVideoCaptureDevices()){if(!first)out<<",";first=false;out<<"{\"name\":"<<std::quoted(d.friendlyName)<<",\"modes\":[";bool f=true;for(auto m:enumerateVideoModes(d.monikerDisplayName)){if(!f)out<<",";f=false;out<<"{\"width\":"<<m.width<<",\"height\":"<<m.height<<",\"fps\":"<<m.frameRate<<",\"min_interval_100ns\":"<<m.minFrameInterval<<",\"max_interval_100ns\":"<<m.maxFrameInterval<<"}";}out<<"]}";}out<<"]";timeEndPeriod(1);CoUninitialize();return 0;}
  bool demoRequested=args.find(L"--demo")!=std::wstring::npos,captureRequested=args.find(L"--capture")!=std::wstring::npos;
  bool fg=args.find(L"--fg")!=std::wstring::npos;bool test1440=args.find(L"--1440")!=std::wstring::npos;
  double stopAfter=0;auto at=args.find(L"--seconds ");if(at!=std::wstring::npos)stopAfter=_wtof(args.c_str()+at+10);
  WNDCLASSW wc{};wc.lpfnWndProc=windowProc;wc.hInstance=instance;wc.lpszClassName=L"FrameLabWindow";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);
  HWND window=CreateWindowW(wc.lpszClassName,L"FrameLab | Capture Frame Generation",WS_OVERLAPPEDWINDOW,100,80,1480,900,nullptr,nullptr,instance,nullptr);
  Graphics g;g.init(window);graphics=&g;
  if(args.find(L"--verify-vfg")!=std::wstring::npos){for(unsigned m=2;m<=4;++m)for(unsigned i=1;i<m;++i)verifyInterpolation(g,root,m,i,args.find(L"--optical")!=std::wstring::npos);DestroyWindow(window);timeEndPeriod(1);CoUninitialize();return 0;}
  if(args.find(L"--benchmark-video")!=std::wstring::npos){runVideoBenchmark(g,root,args.find(L"--no-auto-cut")==std::wstring::npos);DestroyWindow(window);timeEndPeriod(1);CoUninitialize();return 0;}
  {std::ifstream language(root/L"language.txt"); language>>czech;}
  IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;
  ImFontConfig fontConfig{};fontConfig.SizePixels=17;
  io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf",17,&fontConfig,([]{static const ImWchar ranges[]={0x20,0x17f,0x2000,0x206f,0};return ranges;})());
  ImGui::StyleColorsDark();auto& style=ImGui::GetStyle();style.WindowRounding=0;style.FrameRounding=6;style.GrabRounding=6;style.WindowPadding={20,18};style.ItemSpacing={10,6};style.FramePadding={10,6};
  style.Colors[ImGuiCol_WindowBg]={.035f,.05f,.078f,1};style.Colors[ImGuiCol_Button]={.10f,.21f,.26f,1};style.Colors[ImGuiCol_ButtonHovered]={.13f,.35f,.38f,1};style.Colors[ImGuiCol_CheckMark]={.25f,.88f,.75f,1};style.Colors[ImGuiCol_FrameBg]={.075f,.105f,.15f,1};
  ImGui_ImplWin32_Init(window);ImGui_ImplDX11_Init(g.device.Get(),g.context.Get());ShowWindow(window,SW_SHOW);UpdateWindow(window);
  auto devices=enumerateVideoCaptureDevices();int selected=0;for(int i=0;i<int(devices.size());++i){log("Device: "+devices[i].friendlyName+" "+devices[i].monikerDisplayName);}
  std::vector<VideoModeInfo> modes,resolutions;double requestedFps=0;
  auto refreshModes=[&]{modes=devices.empty()?std::vector<VideoModeInfo>{}:enumerateVideoModes(devices[selected].monikerDisplayName);resolutions.clear();for(auto m:modes){bool found=false;for(auto r:resolutions)if(r.width==m.width&&r.height==m.height)found=true;if(!found)resolutions.push_back(m);}requestedFps=0;};refreshModes();
  auto outputs=enumerateAudioOutputDevices();outputs.insert(outputs.begin(),{"","Windows default output"});
  int audioOutput=0,audioVolume=75;bool audioMuted=false;std::string savedOutput;
  {std::ifstream settings(root/L"audio-settings.txt");if(settings){settings>>std::quoted(savedOutput)>>audioVolume>>audioMuted;audioVolume=std::clamp(audioVolume,0,100);}}
  for(int i=0;i<int(outputs.size());++i){log("Audio output: "+outputs[i].friendlyName);if(outputs[i].monikerDisplayName==savedOutput)audioOutput=i;}
  auto saveAudio=[&]{std::ofstream(root/L"audio-settings.txt")<<std::quoted(outputs[audioOutput].monikerDisplayName)<<' '<<audioVolume<<' '<<audioMuted<<'\n';};
  GpuMonitor gpu;AnalysisRecorder analysis;Mailbox mailbox;mailbox.recorder=&analysis;DirectShowCapture capture;std::atomic<bool> demoRun=false;std::jthread demo;
  bool live=false,isDemo=false,vfgInitialized=false,havePrevious=false,pending=false,vsync=args.find(L"--no-vsync")==std::wstring::npos,initFailed=false;
  int quality=args.find(L"--medium")!=std::wstring::npos?1:args.find(L"--high")!=std::wstring::npos?2:args.find(L"--low")!=std::wstring::npos?0:1,resolution=0;unsigned current=0,w=0,h=0;Texture originals[2],generated;FrameGenerator vfg;vfg.useOptical=args.find(L"--optical")!=std::wstring::npos;ID3D11ShaderResourceView* displayed=nullptr;
  bool automaticCuts=args.find(L"--no-auto-cut")==std::wstring::npos;
  int multiplier=args.find(L"--4x")!=std::wstring::npos?4:args.find(L"--3x")!=std::wstring::npos?3:2;unsigned generatedIndex=1;int sampleStride=args.find(L"--input15")!=std::wstring::npos?4:args.find(L"--input20")!=std::wstring::npos?3:2;
  struct QueuedImage {Texture image;bool generated;uint64_t id,a,b;double position,sourceTime;};
  std::vector<Texture> imagePool;
  std::deque<QueuedImage> readyImages; FramePacer pacer;Texture presentedImage;
  uint64_t presentationDrops=0; double displayedAt=0;
  std::ofstream pacingLog;if(args.find(L"--verify-pacing")!=std::wstring::npos){pacingLog.open(root/L"logs"/(session+"-pacing.csv"));pacingLog<<"time_s,generated,source_id,queue\n";}
  auto enqueueImage=[&](Texture& texture,bool generatedFrame,uint64_t id,uint64_t a,uint64_t b,double position,double sourceTime){
   // Bounded latency: discard stale output when the display cannot keep up.
   if(readyImages.size()>=6){presentationDrops+=readyImages.size();readyImages.clear();}
   QueuedImage q{};if(imagePool.empty())q.image.create(g.device.Get(),w,h);else{q.image=std::move(imagePool.back());imagePool.pop_back();}g.context->CopyResource(q.image.texture.Get(),texture.texture.Get());
   q.generated=generatedFrame;q.id=id;q.a=a;q.b=b;q.position=position;q.sourceTime=sourceTime;readyImages.push_back(std::move(q));
  };
  Comparison comparison;uint64_t pairId=0,sourceIds[2]{};bool autoExported=false,autoRecorded=false,autoAnalysis=false;std::filesystem::path exportedAnalysis;HANDLE analysisExporter=nullptr;bool displayedGenerated=false;uint64_t displayedId=0,displayedA=0,displayedB=0;double displayedPosition=0;
  std::string status="Ready. Select a capture device or start a test source.";Packet packet;double previousArrival=0,pairArrival=0,pairInterval=1./30,displaySourceTime=0;
  bool newContent=false,newGenerated=false;uint64_t totalContent=0,totalPresents=0,totalGenerated=0;
  float inFps=0,contentFps=0,outFps=0,presentFps=0,genFps=0,latency=-1,p95=-1,gpuMs=-1,dxgiRate=-1;
  DisplayVerification displayVerification;float confirmedGenFps=-1;uint64_t lastConfirmedContent=0,lastConfirmedGen=0;
  History inputHistory,uniqueHistory,outputHistory,latencyHistory,gpuTimeHistory,utilHistory;
  std::vector<float> latencySamples;double start=now(),lastStats=start;uint64_t lastRaw=0,lastUnique=0,lastContent=0,lastPresents=0,lastGenerated=0;
  auto resetPipeline=[&]{analysis.finish("Source or settings changed");analysis.cancelPending();comparison.clear();readyImages.clear();imagePool.clear();pacer.reset();displayVerification.clearHistory();g.context->PSSetShaderResources(0,0,nullptr);vfg.reset();vfgInitialized=false;havePrevious=false;pending=false;initFailed=false;gpuMs=-1;displayed=nullptr;current=0;previousArrival=0;};
  auto stopSource=[&]{demoRun=false;if(demo.joinable())demo.join();capture.stop();live=false;isDemo=false;resetPipeline();mailbox.clear();lastRaw=lastUnique=0;};
  auto startDemo=[&]{stopSource();isDemo=true;live=true;demoRun=true;status="TEST SOURCE: 30 unique frames inside a 60 Hz stream. Synthetic input.";
   const bool largeDemo=test1440;
   demo=std::jthread([&,largeDemo](std::stop_token token){const unsigned dw=largeDemo?2560:1280,dh=largeDemo?1440:720;std::vector<unsigned char> pixels(size_t(dw)*dh*4);uint64_t tick=0;auto next=Clock::now();
    while(demoRun&&!token.stop_requested()){if(tick%2==0){unsigned phase=unsigned((tick/2)*9)%dw;for(unsigned y=0;y<dh;++y)for(unsigned x=0;x<dw;++x){size_t i=(size_t(y)*dw+x)*4;bool box=((x+dw-phase)%dw)<dw/8&&y>dh/3&&y<2*dh/3;bool grid=x%80<2||y%80<2;pixels[i]=box?125:grid?48:23;pixels[i+1]=box?230:grid?42:18;pixels[i+2]=box?55:grid?36:12;pixels[i+3]=255;}}
     DirectShowCapture::Frame f{};f.width=f.sampleWidth=dw;f.height=f.sampleHeight=dh;f.stride=dw*4;f.data=pixels.data();f.dataSize=pixels.size();mailbox.ingest(f);++tick;next+=std::chrono::nanoseconds(16666667);std::this_thread::sleep_until(next);}
   });log("Demo started");};
  auto startSample=[&]{stopSource();auto path=root/L"samples"/L"csgo-1080p60.bgra";if(!std::filesystem::exists(path)||std::filesystem::file_size(path)<1920ull*1080*4*2){status="Sample missing: run tools/prepare_sample.py";return;}isDemo=true;live=true;demoRun=true;status="CS:GO replay | 1080p | "+std::to_string(60/sampleStride)+" input FPS | video only";
   int stride=sampleStride;demo=std::jthread([&,path,stride](std::stop_token token){
    const size_t bytes=1920ull*1080*4;std::ifstream file(path,std::ios::binary);std::vector<unsigned char> pixels(bytes);auto next=Clock::now();size_t index=0,count=std::filesystem::file_size(path)/bytes;
    while(demoRun&&!token.stop_requested()){file.seekg((index%count)*bytes);if(!file.read((char*)pixels.data(),bytes))break;DirectShowCapture::Frame f{};f.width=f.sampleWidth=1920;f.height=f.sampleHeight=1080;f.stride=1920*4;f.data=pixels.data();f.dataSize=bytes;mailbox.ingest(f);index+=stride;next+=std::chrono::nanoseconds(1000000000ll*stride/60);std::this_thread::sleep_until(next);}
   });};
  auto startCapture=[&]{stopSource();if(devices.empty()){status="No DirectShow capture device found.";return;}try{DirectShowCapture::Options o;o.deviceMoniker=devices[selected].monikerDisplayName;o.enableAudio=true;o.audioOutputMoniker=outputs[audioOutput].monikerDisplayName;o.audioVolume=audioVolume;o.audioMuted=audioMuted;if(resolution>0&&resolution<=int(resolutions.size())){auto mode=resolutions[resolution-1];o.desiredWidth=mode.width;o.desiredHeight=mode.height;o.desiredFrameRate=requestedFps;}
    capture.start([&](const auto& f){mailbox.ingest(f);},o);live=true;status="Capture running: "+devices[selected].friendlyName;log(status);
   }catch(const std::exception& e){status=e.what();log(status);}};
  if(args.find(L"--sample")!=std::wstring::npos)startSample();else if(demoRequested)startDemo();else if(captureRequested)startCapture();
  log("FrameLab started GPU="+std::string(gpu.name)+" quality="+std::to_string(quality)+" vsync="+std::to_string(vsync));
  auto renderFrame=[&]{
   if(frameBusy||!running)return;
   frameBusy=true;struct FrameGuard{~FrameGuard(){frameBusy=false;}} frameGuard;
   if(stopAfter>0&&now()-start>stopAfter){running=false;return;}
   if(resized){resized=false;const auto rw=resizeW,rh=resizeH;if(rw&&rh)g.resize(rw,rh);}
   if(IsIconic(window))return;
   if(g.waitable)WaitForSingleObject(g.waitable,20);
   double t=now();analysis.poll(g);analysis.tick();if(analysisExporter&&WaitForSingleObject(analysisExporter,0)==WAIT_OBJECT_0){CloseHandle(analysisExporter);analysisExporter=nullptr;}
   if(!analysis.active&&!analysis.writing&&!analysis.folder.empty()&&analysis.folder!=exportedAnalysis&&std::filesystem::exists(analysis.folder/L"result.json")){
    std::wstring command=L"python \""+(root.parent_path()/L"tools"/L"export_analysis.py").wstring()+L"\" \""+analysis.folder.wstring()+L"\"";STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};
    if(CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,root.parent_path().c_str(),&si,&pi)){CloseHandle(pi.hThread);analysisExporter=pi.hProcess;exportedAnalysis=analysis.folder;}else{status="Clip exporter could not start: "+std::to_string(GetLastError());exportedAnalysis=analysis.folder;}
   }
   auto startAnalysis=[&]{try{if(analysisExporter){status="Wait for the previous clips to finish exporting";return;}if(!live||!w||!h){status="Start a source before recording";return;}analysis.backendName=vfg.name();analysis.start(root,isDemo?"Test/file source":devices[selected].friendlyName,w,h,quality,multiplier,automaticCuts&&!vfg.useOptical,vsync,mailbox.dedup,mailbox.threshold,gpu.name,fg,inFps,contentFps);}catch(const std::exception& e){status=e.what();analysis.startError(status);log("Recording start failed: "+status);}};
   if(!autoAnalysis&&t-start>3&&args.find(L"--record-analysis")!=std::wstring::npos&&w&&h){startAnalysis();autoAnalysis=true;}

   auto error=capture.consumeLastError();if(!error.empty()){status=error;log(status);}
   try{
  if(pending&&vfg.ready()){comparison.backendName=vfg.name();comparison.capture(g,originals[1-current],generated,originals[current],w,h,++pairId,sourceIds[1-current],sourceIds[current],pairInterval*1000,quality,double(generatedIndex)/multiplier);
   if(!autoRecorded&&pairId>=150&&args.find(L"--record-pairs")!=std::wstring::npos){comparison.startRecording();autoRecorded=true;}
   analysis.generated(g,generated,w,h,pairId,sourceIds[1-current],sourceIds[current],double(generatedIndex)/multiplier);
   comparison.record(g,root,pairArrival);
   if(!autoExported&&pairId>=15&&args.find(L"--export-pair")!=std::wstring::npos){log("Comparison exported: "+comparison.save(g,root).string());autoExported=true;}
   pending=false;gpuMs=vfg.gpuMs();
   enqueueImage(generated,true,pairId,sourceIds[1-current],sourceIds[current],double(generatedIndex)/multiplier,pairArrival);
   if(generatedIndex<unsigned(multiplier-1)){++generatedIndex;vfg.generate(current,false,generatedIndex,multiplier);pending=true;}
   else enqueueImage(originals[current],false,sourceIds[current],sourceIds[current],sourceIds[current],0,pairArrival);
  }
    // Never overwrite a texture while its generated frame or real partner is pending.
    if(!pending&&readyImages.size()<2&&mailbox.take(packet)){
     if(packet.w!=w||packet.h!=h){resetPipeline();w=packet.w;h=packet.h;for(auto& tex:originals)tex.create(g.device.Get(),w,h);generated.create(g.device.Get(),w,h);presentedImage.create(g.device.Get(),w,h);log("Input size "+std::to_string(w)+"x"+std::to_string(h));}
     if(fg&&!vfgInitialized&&!initFailed){status=std::string("Loading ")+vfg.name();log(status);vfg.initialize(g.device.Get(),generated.texture.Get(),w,h,unsigned(quality),root/L"runtime",automaticCuts);vfgInitialized=true;havePrevious=false;status=std::string(vfg.name())+" active | "+std::to_string(w)+" x "+std::to_string(h);log(status);}
     current=1-current;sourceIds[current]=packet.serial;g.context->UpdateSubresource(originals[current].texture.Get(),0,nullptr,packet.data.data(),w*4,0);
     if(fg&&vfgInitialized)vfg.upload(packet.data.data(),current);
     pairArrival=packet.received;pairInterval=havePrevious?std::clamp(packet.received-previousArrival,1./120,.1):1./30;
     if(fg&&vfgInitialized&&havePrevious&&packet.received-previousArrival<.12){generatedIndex=1;vfg.generate(current,false,generatedIndex,multiplier);pending=true;}
     else enqueueImage(originals[current],false,packet.serial,packet.serial,packet.serial,0,packet.received);
     havePrevious=true;previousArrival=packet.received;
    }
   }catch(const std::exception& e){status=std::string("VFG disabled: ")+e.what();log(status);fg=false;initFailed=true;pending=false;readyImages.clear();vfg.reset();vfgInitialized=false;havePrevious=false;displayed=originals[current].view.Get();}
   pacer.setCadence(mailbox.sourceInterval.load(),fg&&vfgInitialized?multiplier:1);
   if(!readyImages.empty()&&(!fg||pacer.due(now()))){
    auto q=std::move(readyImages.front());readyImages.pop_front();
    g.context->CopyResource(presentedImage.texture.Get(),q.image.texture.Get());displayed=presentedImage.view.Get();
    newContent=true;newGenerated=q.generated;displaySourceTime=q.sourceTime;
    displayedGenerated=q.generated;displayedId=q.id;displayedA=q.a;displayedB=q.b;displayedPosition=q.position;imagePool.push_back(std::move(q.image));
   }
   if(t-lastStats>=1){double elapsed=t-lastStats;auto raw=mailbox.raw.load(),uni=mailbox.unique.load();inFps=float((raw-lastRaw)/elapsed);contentFps=float((uni-lastUnique)/elapsed);outFps=float((totalContent-lastContent)/elapsed);presentFps=float((totalPresents-lastPresents)/elapsed);genFps=float((totalGenerated-lastGenerated)/elapsed);
    lastRaw=raw;lastUnique=uni;lastContent=totalContent;lastPresents=totalPresents;lastGenerated=totalGenerated;
    latency=-1;p95=-1;if(!latencySamples.empty()){float sum=0;for(float x:latencySamples)sum+=x;latency=sum/latencySamples.size();std::sort(latencySamples.begin(),latencySamples.end());p95=latencySamples[std::min(latencySamples.size()-1,size_t(std::ceil(latencySamples.size()*.95))-1)];latencySamples.clear();}
    gpu.poll();auto previousMonitor=displayVerification.monitor;displayVerification.updateMonitor(g.swap.Get());if(previousMonitor!=displayVerification.monitor)log("Display mode: "+std::to_string(displayVerification.displayWidth)+"x"+std::to_string(displayVerification.displayHeight)+" @ "+std::to_string(displayVerification.displayHz)+" Hz");
    dxgiRate=displayVerification.available?float((displayVerification.content-lastConfirmedContent)/elapsed):-1;
    confirmedGenFps=displayVerification.available?float((displayVerification.generated-lastConfirmedGen)/elapsed):-1;
    lastConfirmedContent=displayVerification.content;lastConfirmedGen=displayVerification.generated;
    inputHistory.add(inFps);uniqueHistory.add(contentFps);outputHistory.add(outFps);latencyHistory.add(std::max(0.f,latency));gpuTimeHistory.add(std::max(0.f,gpuMs));utilHistory.add(std::max(0.f,gpu.utilization));
    csv<<std::fixed<<std::setprecision(3)<<t-start<<','<<inFps<<','<<contentFps<<','<<outFps<<','<<presentFps<<','<<genFps<<','<<latency<<','<<p95<<','<<gpuMs<<','<<gpu.utilization<<','<<gpu.memoryMB<<','<<mailbox.dropped<<','<<mailbox.invalid<<','<<dxgiRate<<','<<readyImages.size()<<','<<presentationDrops<<','<<confirmedGenFps<<','<<displayVerification.displayHz<<std::endl;lastStats=t;
   }
   ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();ImVec2 size=io.DisplaySize;float panel=showPanel?std::min(czech?490.f:430.f,size.x*.65f):0.f;
   if(imageOnly){
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(size);ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::Begin("Fullscreen image",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
    if(displayed&&w&&h){float scale=std::min(size.x/w,size.y/h);ImGui::SetCursorPos({(size.x-w*scale)/2,(size.y-h*scale)/2});ImGui::Image((ImTextureID)(intptr_t)displayed,{w*scale,h*scale});}ImGui::End();ImGui::PopStyleVar();
   }else{
   ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({size.x-panel,size.y});
   ImGui::Begin("Preview",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
   ImGui::PushStyleColor(ImGuiCol_Text,{.3f,.9f,.78f,1});ImGui::TextUnformatted("FRAME / LAB");ImGui::PopStyleColor();ImGui::SameLine();ImGui::TextDisabled("Capture  >  NVIDIA GPU  >  Display");ImGui::SameLine();if(ImGui::Button(tr("Fullscreen image (F11)")))SendMessageW(window,WM_KEYDOWN,VK_F11,0);
   if(isDemo){ImGui::SameLine();ImGui::TextColored({1,.7f,.3f,1},"  TEST SOURCE");}
   ImGui::Separator();auto area=ImGui::GetContentRegionAvail();float aw=area.x;
   const float comparisonHeight=comparison.enabled?std::min(355.f,std::max(230.f,area.y*.36f)):35.f;
   float ah=std::max(100.f,area.y-comparisonHeight-38);auto startPos=ImGui::GetCursorPos();
   if(displayed&&w&&h){float scale=std::min(aw/w,ah/h);float iw=w*scale,ih=h*scale;ImGui::SetCursorPos({startPos.x+(aw-iw)/2,startPos.y+(ah-ih)/2});ImGui::Image((ImTextureID)(intptr_t)displayed,{iw,ih});}
   else {ImGui::TextUnformatted(tr("Waiting for capture or test source..."));}
   ImGui::SetCursorPos({startPos.x,startPos.y+ah+4});
   comparison.draw(g,root,comparisonHeight);
   ImGui::TextDisabled("%s | %u x %u | F1 dashboard | F11 fullscreen",fg&&vfgInitialized?vfg.name():tr("Native preview"),w,h);
   if(live&&now()-mailbox.lastArrival.load()>2)ImGui::TextColored({1,.65f,.3f,1},"Waiting for capture samples. Check signal / HDCP / other capture apps.");
   ImGui::End();
   if(showPanel){ImGui::SetNextWindowPos({size.x-panel,0});ImGui::SetNextWindowSize({panel,size.y});ImGui::Begin("Telemetry",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    int languageChoice=czech?1:0;if(ImGui::Combo("Language / Jazyk",&languageChoice,"English\000Čeština\000\000")){czech=languageChoice==1;std::ofstream(root/L"language.txt")<<czech;}
    ImGui::TextUnformatted(tr("LIVE TELEMETRY"));ImGui::TextDisabled("%s",gpu.name);ImGui::Separator();
    if(ImGui::BeginCombo(tr("Source"),devices.empty()?"No capture devices":devices[selected].friendlyName.c_str())){for(int i=0;i<int(devices.size());++i)if(ImGui::Selectable(devices[i].friendlyName.c_str(),selected==i)){if(selected!=i){stopSource();selected=i;resolution=0;refreshModes();}}ImGui::EndCombo();}
    ImGui::Text(tr("Delivered frames: %u x %u | %.1f FPS"),w,h,inFps);ImGui::TextDisabled(tr("HDMI signal: not reported by this API"));
    auto modeName=[&](int i){if(i==0)return std::string(tr("Automatic (device)"));auto m=resolutions.at(i-1);return std::to_string(m.width)+" x "+std::to_string(m.height)+"  ["+aspectLabel(m.width,m.height)+"]"+(m.width==3840&&m.height==2160?" 4K":"");};
    if(ImGui::BeginCombo(tr("Resolution"),modeName(resolution).c_str())){for(int i=0;i<=int(resolutions.size());++i)if(ImGui::Selectable(modeName(i).c_str(),resolution==i)){resolution=i;requestedFps=0;}ImGui::EndCombo();}
    auto fpsLabel=[&](double f){if(f==0)return std::string(tr("Automatic (device)"));char text[48];snprintf(text,sizeof(text),"%.3f FPS",f);return std::string(text);};
    ImGui::BeginDisabled(resolution==0);
    if(ImGui::BeginCombo(tr("Capture FPS"),fpsLabel(requestedFps).c_str())){if(ImGui::Selectable(tr("Automatic (device)"),requestedFps==0))requestedFps=0;
     if(resolution>0){auto m=resolutions[resolution-1];for(double f:captureRates(modes,m.width,m.height))if(ImGui::Selectable(fpsLabel(f).c_str(),std::abs(requestedFps-f)<.005))requestedFps=f;}
     ImGui::EndCombo();}ImGui::EndDisabled();
    if(live&&!isDemo&&capture.negotiatedFrameRate()>0)ImGui::Text(tr("Driver negotiated: %.3f FPS"),capture.negotiatedFrameRate());
    if(w&&h)ImGui::Text(tr("Image aspect: %s (preserved)"),aspectLabel(w,h).c_str());
    ImGui::TextWrapped(tr("Capture FPS is not an output limit. Frame generation follows changing input frames."));
    ImGui::TextDisabled(tr("Apply with Start capture. Automatic leaves FPS to the driver."));
    if(ImGui::IsItemHovered())ImGui::SetTooltip(tr("Select a resolution to request FPS. Rates come from driver defaults and ranges; the driver may round them. Automatic is not HDMI signal detection."));
    if(ImGui::Button(tr("Start capture")))startCapture();ImGui::SameLine();if(ImGui::Button(tr("Test 30 FPS"))){fg=true;startDemo();}ImGui::SameLine();if(ImGui::Button(tr("Stop"))){stopSource();status="Stopped";}
    ImGui::BeginDisabled(analysis.active||analysis.writing||analysisExporter);if(ImGui::Button(tr("Record 15 s analysis")))startAnalysis();ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button(tr("Clips folder"))&&!analysis.folder.empty())ShellExecuteW(window,L"open",analysis.folder.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
    ImGui::TextWrapped("%s",analysis.message().c_str());
    if(!analysis.folder.empty()){auto clipState=!std::filesystem::exists(analysis.folder)?"Recording folder no longer exists":std::filesystem::exists(analysis.folder/L"clips.json")?"Clips ready":std::filesystem::exists(analysis.folder/L"export-error.txt")?"Clip export failed; see export-error.txt":analysis.active||analysis.writing?"Collecting source frames":"Creating clips...";ImGui::TextDisabled("%s | RAW %llu / VFG %llu",tr(clipState),(unsigned long long)analysis.rawCount.load(),(unsigned long long)analysis.genCount.load());}
    int backend=vfg.useOptical?1:0;bool change=ImGui::Combo(tr("Backend"),&backend,"NVIDIA VFG (RTX 40/50)\000D3D11 Optical Flow (experimental)\000\000");if(change){resetPipeline();vfg.useOptical=backend==1;}
    change|=ImGui::Checkbox(tr("Frame generation"),&fg);ImGui::SameLine();ImGui::SetNextItemWidth(135);ImGui::BeginDisabled(vfg.useOptical);change|=ImGui::Combo("##quality",&quality,czech?"Nízká\000Střední\000Vysoká\000\000":"Low\000Medium\000High\000\000");ImGui::EndDisabled();int multiplierChoice=multiplier-2;change|=ImGui::Combo(tr("Multiplier"),&multiplierChoice,"2x\0003x\0004x\000\000");multiplier=multiplierChoice+2;if(multiplier>2)ImGui::TextWrapped(tr("Experimental: 3x/4x can increase artifacts and latency."));ImGui::BeginDisabled(vfg.useOptical);change|=ImGui::Checkbox(tr("Automatic scene-cut detection"),&automaticCuts);ImGui::EndDisabled();if(vfg.useOptical)ImGui::TextWrapped(tr("Experimental optical flow: no AI model or automatic cut detection."));if(change){resetPipeline();status=fg?"VFG enabled. Waiting for a new source frame.":"Native preview; VFG disabled.";}
    if(ImGui::Button(tr("Play CS:GO sample")))startSample();ImGui::SameLine();ImGui::SetNextItemWidth(120);int strideChoice=sampleStride-2;if(ImGui::Combo(tr("Next replay FPS"),&strideChoice,"30\00020\00015\000\000")){sampleStride=strideChoice+2;}
    ImGui::Text(tr("Presentation queue: %u | discarded: %llu"),unsigned(readyImages.size()),(unsigned long long)presentationDrops);
    bool dd=mailbox.dedup; if(ImGui::Checkbox(tr("Remove repeated input frames"),&dd))mailbox.dedup=dd;
    if(ImGui::IsItemHovered())ImGui::SetTooltip(tr("Exact duplicate detection by default. Optional noise tolerance uses sparse RGB samples. Static scenes can read 0 FPS; this is not engine telemetry."));
    ImGui::Checkbox(tr("VSync (tear-free)"),&vsync);
    ImGui::SetNextItemWidth(285);
    if(ImGui::BeginCombo(tr("Output"),outputs[audioOutput].friendlyName.c_str())){for(int i=0;i<int(outputs.size());++i)if(ImGui::Selectable(outputs[i].friendlyName.c_str(),audioOutput==i)){audioOutput=i;saveAudio();if(live&&!isDemo)startCapture();}ImGui::EndCombo();}
    ImGui::SetNextItemWidth(215);bool volumeChanged=ImGui::SliderInt(tr("Volume"),&audioVolume,0,100,"%d%%");ImGui::SameLine();volumeChanged|=ImGui::Checkbox(tr("Mute"),&audioMuted);if(volumeChanged){capture.setAudioVolume(audioVolume,audioMuted);saveAudio();}
    if(live&&!isDemo)ImGui::TextColored(capture.audioConnected()?ImVec4(.3f,.85f,.7f,1):ImVec4(1,.65f,.3f,1),"%s",capture.audioStatus().c_str());
    ImGui::TextWrapped("%s",tr(status.c_str()));ImGui::Separator();
    if(ImGui::BeginTable("metrics",2)){ImGui::TableNextColumn();metric(tr("INPUT / capture"),inFps,"fps",{.25f,.86f,.75f,1});ImGui::TableNextColumn();metric(tr("CONTENT / estimated"),contentFps,"fps",{1,.75f,.35f,1});ImGui::TableNextColumn();metric(tr("OUTPUT / content submitted"),outFps,"fps",{.48f,.64f,1,1});ImGui::TableNextColumn();metric(tr("GENERATED / submitted"),genFps,"fps",{.48f,.64f,1,1});ImGui::EndTable();}
    ImGui::Separator();ImGui::Text(tr("Display mode: %u x %u | %u Hz"),displayVerification.displayWidth,displayVerification.displayHeight,displayVerification.displayHz);
    if(ImGui::BeginTable("verified-display",2)){ImGui::TableNextColumn();metric(tr("WINDOWS / confirmed IDs"),dxgiRate,"fps",{.3f,.9f,.65f,1});ImGui::TableNextColumn();metric(tr("GENERATED / confirmed"),confirmedGenFps,"fps",{.3f,.9f,.65f,1});ImGui::EndTable();}
    ImGui::TextWrapped(tr("Observed display confirmations: lower bound, not all frames may be sampled. Repeated IDs count once; SDK pixel copies can still have new IDs."));
    if(dxgiRate<0)ImGui::TextWrapped(tr("Display verification unavailable. Submitted FPS alone does not prove displayed FPS."));
    if(displayVerification.haveImage)ImGui::Text(tr("Last OS-confirmed image: %s #%llu | present #%u"),displayVerification.lastGenerated?"G":"R",(unsigned long long)displayVerification.lastImage,displayVerification.lastConfirmed);
    chart("FPS",inputHistory,&uniqueHistory,&outputHistory,std::max(70.f,std::max(inFps,outFps)*1.2f),70);ImGui::TextDisabled(tr("Input / content / output | last 180 seconds"));
    if(ImGui::BeginTable("latency",2)){ImGui::TableNextColumn();metric(tr("CALLBACK > PRESENT / mean"),latency,"ms",{.25f,.86f,.75f,1});ImGui::TableNextColumn();metric(tr("VFG + GPU output copy"),gpuMs,"ms",{1,.75f,.35f,1});ImGui::EndTable();}
    chart(tr("Latency"),latencyHistory,&gpuTimeHistory,nullptr,std::max(50.f,std::max(latency,gpuMs)*1.2f),50);
    ImGui::TextDisabled(tr("App latency P95: %.1f ms | excludes card + scanout"),p95);
    chart("GPU",utilHistory,nullptr,nullptr,100,35);ImGui::Text(tr("GPU total %.0f%% | VRAM %.0f MB | %.0f C | %.0f W"),gpu.utilization,gpu.memoryMB,gpu.temp,gpu.power);
    ImGui::TextDisabled(tr("Present calls %.1f/s | observed content %.1f/s"),presentFps,dxgiRate);ImGui::TextDisabled(tr("-1 = unavailable. Physical monitor FPS not measured."));
    ImGui::TextDisabled(tr("Mailbox drops %llu | invalid %llu"),(unsigned long long)mailbox.dropped.load(),(unsigned long long)mailbox.invalid.load());
    if(ImGui::CollapsingHeader(tr("Measurement details"))){float threshold=mailbox.threshold;if(ImGui::SliderFloat(tr("Dedup sensitivity"),&threshold,0,5,"%.2f"))mailbox.threshold=threshold;ImGui::TextWrapped(tr("Latency starts when the capture callback receives pixels and ends after Present returns. Generated frames use the newer source timestamp. GPU utilization is the whole GPU, including other apps. CSV is saved every second under app/logs. HDMI audio uses the selected DirectSound output. Audio latency and A/V offset are not measured."));}
    ImGui::End();
   }
   }
   ImGui::Render();g.begin();ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());auto hr=g.present(inSizeMove?false:vsync);
   if(hr==S_OK){if(newContent){pacer.presented(now());displayedAt=now();if(pacingLog)pacingLog<<std::fixed<<std::setprecision(6)<<displayedAt-start<<','<<newGenerated<<','<<displayedId<<','<<readyImages.size()<<'\n';}if(displayed)analysis.presented(displayedGenerated,displayedId,displayedA,displayedB,displayedPosition);++totalPresents;if(newContent){++totalContent;if(newGenerated)++totalGenerated;latencySamples.push_back(float((now()-displaySourceTime)*1000));}}
   else if(FAILED(hr))throw std::runtime_error("Present failed "+std::to_string(unsigned(hr)));
   if(hr==S_OK&&displayed){displayVerification.submitted(g.swap.Get(),displayedGenerated,displayedId);if(auto c=displayVerification.observe(g.swap.Get()))analysis.confirmed(c->submission.generated,c->submission.image,c->submission.present,c->refresh,c->syncRefresh,c->syncQpc);}
   newContent=false;newGenerated=false;
   if(!vsync&&!inSizeMove)std::this_thread::sleep_for(std::chrono::milliseconds(1));
  };
  {
   FrameTickBinding binding(window,renderFrame);
   while(running){
    MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT)running=false;TranslateMessage(&msg);DispatchMessageW(&msg);}
    if(modalFailure)std::rethrow_exception(modalFailure);
    if(!running)break;
    if(stopAfter>0&&now()-start>stopAfter)break;
    if(IsIconic(window)){std::this_thread::sleep_for(std::chrono::milliseconds(30));continue;}
    renderFrame();
   }
  }
  stopSource();if(analysisExporter)CloseHandle(analysisExporter);log("Finished: content="+std::to_string(totalContent)+" generated="+std::to_string(totalGenerated));ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();graphics=nullptr;DestroyWindow(window);
 }catch(const std::exception& e){log(std::string("FATAL ")+e.what());MessageBoxA(nullptr,e.what(),"FrameLab",MB_ICONERROR);result=1;}
 timeEndPeriod(1);CoUninitialize();return result;
}
