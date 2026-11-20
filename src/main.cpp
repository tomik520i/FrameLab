#include "Graphics.hpp"
#include "FramePacer.hpp"
#include "RecordedReplay.hpp"
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
#include "UiTheme.hpp"
#include "UiPreview.hpp"
#include "NrControls.hpp"
#include "NrHistory.hpp"
#include "TextureConvert.hpp"
#include "GpuPollWait.hpp"
#include "Diagnostics.hpp"
#include "SystemTelemetry.hpp"
#include "VideoFileReplay.hpp"
#include "LocalVideoPlayer.hpp"
#include <commdlg.h>
#include "SubtitleControls.hpp"
#include "RecordingOutput.hpp"
#include "RecordingLocation.hpp"
#include "LiveSession.hpp"
#include "OutputStats.hpp"
#include "SubtitleTrace.hpp"
#include "SubtitleStylePicker.hpp"
#include "DisplayControls.hpp"
#include "WindowChrome.hpp"
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
struct Packet {std::vector<unsigned char> data;unsigned w=0,h=0;double received=0;uint64_t serial=0,scene=0,sourceEpoch=0;};
struct Mailbox {
 SubtitleTranslator* subtitles=nullptr;
 AnalysisRecorder* recorder=nullptr;
 std::mutex mutex;Packet latest;bool available=false;std::vector<unsigned char> signature,previousPixels,normalized;
 std::atomic<uint64_t> raw=0,unique=0,dropped=0,invalid=0;std::atomic<double> lastArrival=0,sourceInterval=1./30;double lastChangedArrival=0;
 uint64_t scene=0,sourceEpoch=0;LONGLONG lastPts=-1;
 std::atomic<float> threshold=0.f;std::atomic<bool> dedup=true;unsigned sw=0,sh=0;
 void ingest(const DirectShowCapture::Frame& f,uint64_t epoch=0){
  if(epoch!=sourceEpoch){sourceEpoch=epoch;++scene;lastPts=-1;lastChangedArrival=0;signature.clear();if(subtitles)subtitles->reset();}
  const double arrival=now();lastArrival=arrival;++raw;
  const unsigned fullH=f.sampleHeight?f.sampleHeight:f.height;
  if(!f.data||!f.width||!f.height||f.stride<f.width*4||f.dataSize<size_t(f.stride)*fullH||f.contentLeft+f.width>f.stride/4||f.contentTop+f.height>fullH){++invalid;return;}
  auto sourceRow=[&](unsigned y){const auto sy=f.contentTop+y;return f.data+size_t(f.bottomUp?fullH-1-sy:sy)*f.stride+f.contentLeft*4;};
  // DirectShow MEDIASUBTYPE_RGB32 is BGRX, not BGRA. X is undefined.
  // Normalize before both dedup and rendering so X cannot hide the picture
  // or turn identical RGB frames into false changes.
  normalized.resize(size_t(f.width)*f.height*4);
  for(unsigned y=0;y<f.height;++y){auto src=sourceRow(y);auto dst=normalized.data()+size_t(y)*f.width*4;for(unsigned x=0;x<f.width;++x){uint32_t pixel;memcpy(&pixel,src+x*4,4);pixel|=0xff000000u;memcpy(dst+x*4,&pixel,4);}}
  if(subtitles)subtitles->submit(normalized.data(),f.width,f.height,arrival);
  if(recorder)recorder->raw(normalized.data(),f.width,f.height,raw.load(),arrival);
  auto row=[&](unsigned y){return normalized.data()+size_t(y)*f.width*4;};
  if(raw.load()==1){uint64_t alpha=0,rgb=0;unsigned count=0;for(unsigned y=0;y<f.height;y+=16){auto src=sourceRow(y);for(unsigned x=0;x<f.width;x+=16){alpha+=src[x*4+3];rgb+=src[x*4]+src[x*4+1]+src[x*4+2];++count;}}std::ofstream("logs/input-format.json")<<"{\"raw_x_byte_mean\":"<<double(alpha)/count<<",\"rgb_mean\":"<<double(rgb)/(3*count)<<",\"alpha_forced_opaque\":true}\n";}
  // Exact comparison by default: sparse probes alone miss small moving objects.
  // Optional noise tolerance is approximate and explicitly exposed as such.
  std::vector<unsigned char> sample;sample.reserve(96*54*3);
  for(unsigned gy=0;gy<54;++gy){auto r=row(std::min(f.height-1,(gy*f.height+f.height/2)/54));for(unsigned gx=0;gx<96;++gx){auto x=std::min(f.width-1,(gx*f.width+f.width/2)/96);for(int c=0;c<3;++c)sample.push_back(r[x*4+c]);}}
  bool changed=signature.size()!=sample.size()||sw!=f.width||sh!=f.height;
  double sceneDifference=0;if(signature.size()==sample.size())for(size_t i=0;i<sample.size();++i)sceneDifference+=std::abs(int(sample[i])-int(signature[i]));
  if(changed||(lastPts>=0&&f.timestamp100ns<uint64_t(lastPts))||sceneDifference>sample.size()*55.0)++scene;
  lastPts=LONGLONG(f.timestamp100ns);
  if(!changed&&threshold.load()>0){double sad=0;unsigned significant=0;for(size_t i=0;i<sample.size();++i){int d=std::abs(int(sample[i])-int(signature[i]));sad+=d;if(d>8)++significant;}changed=sad/sample.size()>threshold.load()||significant>sample.size()/500;}
  else if(!changed){changed=previousPixels.size()!=size_t(f.width)*f.height*4;for(unsigned y=0;!changed&&y<f.height;++y)changed=memcmp(previousPixels.data()+size_t(y)*f.width*4,row(y),f.width*4)!=0;}
  if(changed){previousPixels.resize(size_t(f.width)*f.height*4);for(unsigned y=0;y<f.height;++y)memcpy(previousPixels.data()+size_t(y)*f.width*4,row(y),f.width*4);}
  signature=std::move(sample);sw=f.width;sh=f.height;if(changed){if(lastChangedArrival>0){double gap=arrival-lastChangedArrival;if(gap>1./240&&gap<.12)sourceInterval=sourceInterval.load()*.8+gap*.2;}lastChangedArrival=arrival;++unique;}
  if(!changed&&dedup)return;
  std::lock_guard lock(mutex);if(available)++dropped;latest.data.resize(size_t(f.width)*f.height*4);
  for(unsigned y=0;y<f.height;++y)memcpy(latest.data.data()+size_t(y)*f.width*4,row(y),f.width*4);
  latest.w=f.width;latest.h=f.height;latest.received=arrival;latest.serial=raw;latest.scene=scene;latest.sourceEpoch=sourceEpoch;available=true;
 }
 bool take(Packet& p){std::lock_guard lock(mutex);if(!available)return false;std::swap(p,latest);available=false;return true;}
 void clear(){std::lock_guard lock(mutex);available=false;signature.clear();sw=sh=0;scene=sourceEpoch=0;lastPts=-1;raw=0;unique=0;dropped=0;invalid=0;lastArrival=0;lastChangedArrival=0;sourceInterval=1./30;}
};
struct GpuMonitor {
 HMODULE dll=nullptr;void* device=nullptr;float utilization=-1,memoryMB=-1,freeMemoryMB=-1,temp=-1,power=-1;char name[96]="NVIDIA GPU";
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
 void poll(){if(!device)return;Util u{};Memory m{};unsigned v;if(util&&util(device,&u)==0)utilization=float(u.gpu);if(memory&&memory(device,&m)==0){memoryMB=float(m.used/1048576.0);freeMemoryMB=float(m.free/1048576.0);}if(temperature&&temperature(device,0,&v)==0)temp=float(v);if(watts&&watts(device,&v)==0)power=v/1000.f;}
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
 ImGui::InvisibleButton(id,ImVec2(width,height));auto d=ImGui::GetWindowDrawList();d->AddRectFilled(origin,ImVec2(origin.x+width,origin.y+height),ImGui::ColorConvertFloat4ToU32(ui::panel),6);
 for(int i=1;i<4;++i){float y=origin.y+height*i/4;d->AddLine(ImVec2(origin.x,y),ImVec2(origin.x+width,y),IM_COL32(50,59,49,150));}
 auto line=[&](History& h,ImU32 color){ImVec2 prev{};for(int i=0;i<History::N;++i){float v=h.values[(h.pos+i)%History::N];ImVec2 p(origin.x+width*i/(History::N-1),origin.y+height-std::clamp(v/maxY,0.f,1.f)*(height-8)-4);if(i)d->AddLine(prev,p,color,1.8f);prev=p;}};
 line(a,ImGui::ColorConvertFloat4ToU32(ui::input));if(b)line(*b,ImGui::ColorConvertFloat4ToU32(ui::orange));if(c)line(*c,ImGui::ColorConvertFloat4ToU32(ui::accent));
}
void metric(const char* label,float value,const char* unit,ImVec4 color){ImGui::PushFont(ui::mono);ImGui::PushStyleColor(ImGuiCol_Text,ui::muted);ImGui::TextWrapped("%s",label);ImGui::PopStyleColor();ImGui::PopFont();ImGui::PushStyleColor(ImGuiCol_Text,color);ImGui::PushFont(ui::number);if(value<0)ImGui::TextUnformatted("--");else ImGui::Text("%.1f %s",value,unit);ImGui::PopFont();ImGui::PopStyleColor();}
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
ULONGLONG pointerActivity=0;POINT lastPointer{LONG_MIN,LONG_MIN};bool reportedCursorHidden=false;
bool hideOutputCursor(){return imageOnly&&GetTickCount64()-pointerActivity>=2000;}
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
 if(msg==WM_THEMECHANGED||msg==WM_SETTINGCHANGE)applyWindowChrome(h);
 if(msg==WM_MOUSEMOVE){POINT p{short(LOWORD(l)),short(HIWORD(l))};if(p.x!=lastPointer.x||p.y!=lastPointer.y){lastPointer=p;pointerActivity=GetTickCount64();if(imageOnly)SetCursor(LoadCursor(nullptr,IDC_ARROW));}}
 if(msg==WM_LBUTTONDOWN||msg==WM_RBUTTONDOWN||msg==WM_MOUSEWHEEL)pointerActivity=GetTickCount64();
 if(msg==WM_SETCURSOR&&imageOnly&&LOWORD(l)==HTCLIENT){SetCursor(hideOutputCursor()?nullptr:LoadCursor(nullptr,IDC_ARROW));return 1;}
 if(ImGui_ImplWin32_WndProcHandler(h,msg,w,l))return 1;
 // ImGui receives F10 above; do not let DefWindowProc enter the system menu
 // and suspend normal playback when toggling the fullscreen stats overlay.
 if(w==VK_F10&&(msg==WM_KEYDOWN||msg==WM_KEYUP||msg==WM_SYSKEYDOWN||msg==WM_SYSKEYUP))return 0;
 if(msg==WM_ENTERSIZEMOVE){inSizeMove=true;modalFrames=0;SetTimer(h,resizeTimer,16,nullptr);log("Interactive resize/move entered");return 0;}
 if(msg==WM_EXITSIZEMOVE){KillTimer(h,resizeTimer);inSizeMove=false;RECT rc{};GetClientRect(h,&rc);resizeW=rc.right;resizeH=rc.bottom;resized=true;renderModalFrame();log("Interactive resize/move exited; live ticks="+std::to_string(modalFrames));return 0;}
 if(msg==WM_TIMER&&w==resizeTimer){renderModalFrame();return 0;}
 if(msg==WM_SIZE&&w!=SIZE_MINIMIZED){resized=true;resizeW=LOWORD(l);resizeH=HIWORD(l);if(inSizeMove)renderModalFrame();return 0;}
 if(msg==WM_ERASEBKGND)return 1;
 if(msg==WM_CLOSE){running=false;return 0;}if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}
 if(msg==WM_KEYDOWN&&w==VK_ESCAPE&&imageOnly){SendMessageW(h,WM_KEYDOWN,VK_F11,0);return 0;}
 if(msg==WM_KEYDOWN&&w==VK_F1){if(!imageOnly)showPanel=!showPanel;return 0;}
 if(msg==WM_KEYDOWN&&w==VK_F11){static RECT old{};fullscreen=!fullscreen;imageOnly=fullscreen;pointerActivity=GetTickCount64();SetCursor(LoadCursor(nullptr,IDC_ARROW));if(fullscreen){GetWindowRect(h,&old);MONITORINFO m{sizeof(m)};GetMonitorInfo(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&m);SetWindowLongPtr(h,GWL_STYLE,WS_POPUP|WS_VISIBLE);SetWindowPos(h,HWND_TOP,m.rcMonitor.left,m.rcMonitor.top,m.rcMonitor.right-m.rcMonitor.left,m.rcMonitor.bottom-m.rcMonitor.top,SWP_FRAMECHANGED);}else{SetWindowLongPtr(h,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE);SetWindowPos(h,nullptr,old.left,old.top,old.right-old.left,old.bottom-old.top,SWP_FRAMECHANGED|SWP_NOZORDER);}return 0;}
 return DefWindowProc(h,msg,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR cmd,int){
 wchar_t exe[MAX_PATH];GetModuleFileNameW(nullptr,exe,MAX_PATH);const auto root=std::filesystem::path(exe).parent_path();SetCurrentDirectoryW(root.c_str());
 std::filesystem::create_directories(root/L"logs");auto session=std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
 trace.open(root/L"logs"/(session+".log"));std::ofstream csv(root/L"logs"/(session+".csv"));
 csv<<"elapsed_s,raw_input_fps,content_change_fps,content_submissions_fps,present_calls_fps,generated_fps,callback_to_present_mean_ms,callback_to_present_p95_ms,vfg_gpu_ms,gpu_total_percent,vram_total_mb,mailbox_drops,invalid_samples,os_observed_content_fps,presentation_queue,presentation_drops,os_observed_generated_fps,display_mode_hz,processed_pair_gap_ms,nr_worker_ms,nr_inference_gpu_ms,nr_ready_to_take_ms,loop_gap_max_ms,present_cpu_total_ms,window_foreground,window_fullscreen,nr_enabled,nr_height_limit,nr_passes,source_interval_ms,nr_economy,nr_evaluations_fps\n";
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);timeBeginPeriod(1);SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 int result=0;
 try{
  std::wstring args=cmd;
  // Diagnostic A/B switch: retain the previous display-bound work loop.
  const bool legacyNrScheduling=args.find(L"--legacy-nr-scheduling")!=std::wstring::npos;
  const bool legacyNrTrim=args.find(L"--legacy-nr-trim")!=std::wstring::npos;
  const bool legacyOutputWait=args.find(L"--legacy-output-wait")!=std::wstring::npos;
  // A/B diagnostic: make the process-local timer/QoS request explicit. Never
  // changes a Windows power plan or a driver's global background FPS limit.
  const bool backgroundQos=args.find(L"--background-qos")!=std::wstring::npos;
  if(backgroundQos){PROCESS_POWER_THROTTLING_STATE policy{};policy.Version=PROCESS_POWER_THROTTLING_CURRENT_VERSION;policy.ControlMask=PROCESS_POWER_THROTTLING_EXECUTION_SPEED|PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;policy.StateMask=0;const auto ok=SetProcessInformation(GetCurrentProcess(),ProcessPowerThrottling,&policy,sizeof(policy));log("Background QoS request="+std::to_string(ok)+" error="+std::to_string(ok?0:GetLastError()));}
  int argumentCount=0;auto argumentValues=CommandLineToArgvW(GetCommandLineW(),&argumentCount);std::vector<std::wstring> arguments;
  if(argumentValues){for(int i=1;i<argumentCount;++i)arguments.emplace_back(argumentValues[i]);LocalFree(argumentValues);}
  auto argument=[&](const wchar_t* key,std::wstring fallback=L""){for(size_t i=0;i+1<arguments.size();++i)if(arguments[i]==key)return arguments[i+1];return fallback;};
  const auto previewPath=argument(L"--ui-preview");
  const bool uiPreview=!previewPath.empty()||args.find(L"--verify-recording")!=std::wstring::npos;
  LiveSession resume;if(LiveSession::shouldResume(arguments)&&!uiPreview)resume.load(root/L"runtime-session.txt");
  const int previewTab=uiPreview?std::clamp(_wtoi(argument(L"--preview-tab",L"0").c_str()),0,8):-1;
  const double previewDelay=std::clamp(_wtof(argument(L"--preview-delay",L"1.6").c_str()),1.6,25.);
  if(args.find(L"--verify-core")!=std::wstring::npos){verifyCaptureCore(root);timeEndPeriod(1);CoUninitialize();return 0;}
  if(args.find(L"--verify-devices")!=std::wstring::npos){std::ofstream out(root/L"logs"/L"device-modes.json");out<<"[";bool first=true;for(auto& d:enumerateVideoCaptureDevices()){if(!first)out<<",";first=false;out<<"{\"name\":"<<std::quoted(d.friendlyName)<<",\"modes\":[";bool f=true;for(auto m:enumerateVideoModes(d.monikerDisplayName)){if(!f)out<<",";f=false;out<<"{\"width\":"<<m.width<<",\"height\":"<<m.height<<",\"fps\":"<<m.frameRate<<",\"min_interval_100ns\":"<<m.minFrameInterval<<",\"max_interval_100ns\":"<<m.maxFrameInterval<<"}";}out<<"]}";}out<<"]";timeEndPeriod(1);CoUninitialize();return 0;}
  bool demoRequested=args.find(L"--demo")!=std::wstring::npos,captureRequested=args.find(L"--capture")!=std::wstring::npos;
  if(uiPreview){captureRequested=false;imageOnly=args.find(L"--preview-fullscreen")!=std::wstring::npos;}
  bool fg=args.find(L"--fg")!=std::wstring::npos;bool test1440=args.find(L"--1440")!=std::wstring::npos;
  double stopAfter=0;auto at=args.find(L"--seconds ");if(at!=std::wstring::npos)stopAfter=_wtof(args.c_str()+at+10);
  if(uiPreview)stopAfter=std::max(5.,previewDelay+2);
  WNDCLASSW wc{};wc.lpfnWndProc=windowProc;wc.hInstance=instance;wc.lpszClassName=L"FrameLabWindow";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);
  int initialWidth=1480,initialHeight=900;
  if(uiPreview){initialWidth=std::clamp(_wtoi(argument(L"--preview-width",L"1480").c_str()),640,3840);initialHeight=std::clamp(_wtoi(argument(L"--preview-height",L"900").c_str()),480,2160);}
  HWND window=CreateWindowW(wc.lpszClassName,L"FrameLab | preview.16 | OCR + frame pacing",uiPreview?WS_POPUP:WS_OVERLAPPEDWINDOW,100,80,initialWidth,initialHeight,nullptr,nullptr,instance,nullptr);
  applyWindowChrome(window);
  Graphics g;g.init(window);graphics=&g;
  if(args.find(L"--verify-vfg")!=std::wstring::npos){for(unsigned m=2;m<=4;++m)for(unsigned i=1;i<m;++i)verifyInterpolation(g,root,m,i,args.find(L"--optical")!=std::wstring::npos);DestroyWindow(window);timeEndPeriod(1);CoUninitialize();return 0;}
  if(args.find(L"--benchmark-video")!=std::wstring::npos){runVideoBenchmark(g,root,args.find(L"--no-auto-cut")==std::wstring::npos);DestroyWindow(window);timeEndPeriod(1);CoUninitialize();return 0;}
  {std::ifstream language(root/L"language.txt"); language>>czech;}
  if(uiPreview)czech=args.find(L"--preview-cs")!=std::wstring::npos;
  IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;
  ui::initialize();
  ImGui_ImplWin32_Init(window);ImGui_ImplDX11_Init(g.device.Get(),g.context.Get());if(!uiPreview){ShowWindow(window,SW_SHOW);UpdateWindow(window);}
  auto devices=enumerateVideoCaptureDevices();int selected=0;for(int i=0;i<int(devices.size());++i){log("Device: "+devices[i].friendlyName+" "+devices[i].monikerDisplayName);}
  if(auto requestedDevice=argument(L"--capture-device");!requestedDevice.empty()){
   const int bytes=WideCharToMultiByte(CP_UTF8,0,requestedDevice.data(),int(requestedDevice.size()),nullptr,0,nullptr,nullptr);
   std::string name(bytes,'\0');WideCharToMultiByte(CP_UTF8,0,requestedDevice.data(),int(requestedDevice.size()),name.data(),bytes,nullptr,nullptr);
   auto found=std::find_if(devices.begin(),devices.end(),[&](const auto& device){return device.friendlyName==name;});
   if(found==devices.end())throw std::runtime_error("Requested capture device not found: "+name);
   selected=int(found-devices.begin());
  }
  std::vector<VideoModeInfo> modes,resolutions;double requestedFps=0;
  auto refreshModes=[&]{modes=devices.empty()?std::vector<VideoModeInfo>{}:enumerateVideoModes(devices[selected].monikerDisplayName);resolutions.clear();for(auto m:modes){bool found=false;for(auto r:resolutions)if(r.width==m.width&&r.height==m.height)found=true;if(!found)resolutions.push_back(m);}requestedFps=0;};refreshModes();
  auto outputs=enumerateAudioOutputDevices();outputs.insert(outputs.begin(),{"","Windows default output"});
  int audioOutput=0,audioVolume=75;bool audioMuted=false;std::string savedOutput;
  {std::ifstream settings(root/L"audio-settings.txt");if(settings){settings>>std::quoted(savedOutput)>>audioVolume>>audioMuted;audioVolume=std::clamp(audioVolume,0,100);}}
  for(int i=0;i<int(outputs.size());++i){log("Audio output: "+outputs[i].friendlyName);if(outputs[i].monikerDisplayName==savedOutput)audioOutput=i;}
  auto saveAudio=[&]{std::ofstream(root/L"audio-settings.txt")<<std::quoted(outputs[audioOutput].monikerDisplayName)<<' '<<audioVolume<<' '<<audioMuted<<'\n';};
  bool isLocalVideo=false,subtitleAutoPaused=false,subtitleWarmupOptout=false;std::filesystem::path localVideoPath;
  auto fileOutputs=LocalVideoPlayer::audioOutputs();int fileAudioOutput=0;std::wstring savedFileOutput;
  {std::wifstream settings(root/L"file-audio-settings.txt");settings>>std::quoted(savedFileOutput);}
  for(int i=0;i<int(fileOutputs.size());++i)if(fileOutputs[i].id==savedFileOutput)fileAudioOutput=i;
  uint64_t acceptedFileEpoch=0;float seekPosition=0;bool editingSeek=false;
  SubtitleSettings subtitleSettings;subtitleSettings.load(root/L"subtitle-settings.txt");bool subtitleStartupPending=args.find(L"--subtitles")!=std::wstring::npos;OcrSetup ocrSetup;
  if(uiPreview&&args.find(L"--preview-subtitle-edit")!=std::wstring::npos)subtitleSettings.editOverlay=true;
  const bool rapidOcrReady=SubtitleTranslator::rapidOcrAvailable(root);
  SubtitleTrace subtitleTrace;if(auto path=argument(L"--subtitle-trace");!path.empty())subtitleTrace.open(path);
  Diagnostics diagnostics;SystemTelemetry systemTelemetry;
  auto savedSubtitleSettings=subtitleSettings;SubtitleModels subtitleModels(root);SubtitleTranslator subtitles(root);subtitles.setDiagnostics(&diagnostics);subtitles.configure(subtitleSettings);SubtitleSnapshot frozenSubtitle;
  SubtitleStylePicker subtitleStylePicker;SubtitleInspector subtitleInspector;SubtitleProfiles subtitleProfiles;subtitleProfiles.load(root/L"subtitle-profiles.txt");
  DisplayControls displayControls;displayControls.load(root/L"display-settings.txt");
  GpuMonitor gpu;AnalysisRecorder analysis;RecordingLocation recordingLocation;recordingLocation.load(root,analysis);RecordingOutput recordingOutput;Mailbox mailbox;mailbox.subtitles=&subtitles;mailbox.recorder=&analysis;DirectShowCapture capture;std::atomic<bool> demoRun=false;std::jthread demo;LocalVideoPlayer localVideo(root/L"media"/L"vlc");
  std::mutex replayErrorMutex;std::string replayError;
  bool live=false,isDemo=false,vfgInitialized=false,havePrevious=false,pending=false,vsync=args.find(L"--no-vsync")==std::wstring::npos,initFailed=false;
  int quality=args.find(L"--medium")!=std::wstring::npos?1:args.find(L"--high")!=std::wstring::npos?2:args.find(L"--low")!=std::wstring::npos?0:1,resolution=0;unsigned current=0,w=0,h=0;Texture originals[2],generated;FrameGenerator vfg;vfg.useOptical=args.find(L"--optical")!=std::wstring::npos;ID3D11ShaderResourceView* displayed=nullptr;
  // Explicit capture startup preserves a reviewed live setup across a local update.
  if(!argument(L"--capture-width").empty()&&!argument(L"--capture-height").empty()){
   const int cw=_wtoi(argument(L"--capture-width").c_str()),ch=_wtoi(argument(L"--capture-height").c_str());
   auto mode=std::find_if(resolutions.begin(),resolutions.end(),[&](auto m){return int(m.width)==cw&&int(m.height)==ch;});
   if(mode==resolutions.end())throw std::runtime_error("Requested capture resolution is not advertised by this device");
   resolution=int(mode-resolutions.begin())+1;
   if(!argument(L"--capture-fps").empty()){requestedFps=std::stod(argument(L"--capture-fps"));if(!std::isfinite(requestedFps)||requestedFps<=0||requestedFps>1000)throw std::runtime_error("Invalid capture FPS");}
  }
  bool automaticCuts=args.find(L"--no-auto-cut")==std::wstring::npos;
  int multiplier=args.find(L"--4x")!=std::wstring::npos?4:args.find(L"--3x")!=std::wstring::npos?3:2;unsigned generatedIndex=1;int sampleStride=args.find(L"--input60")!=std::wstring::npos?1:args.find(L"--input15")!=std::wstring::npos?4:args.find(L"--input20")!=std::wstring::npos?3:2;
  struct QueuedImage {Texture image,enhanced;bool nrApplied=false;bool generated;uint64_t id,a,b,scene;double position,sourceTime;};
  std::vector<Texture> imagePool,enhancedPool;
  std::deque<QueuedImage> readyImages; FramePacer pacer;Texture presentedImage;
  bool freezePreview=false;std::string activeVideoName;
  Texture enhancedOriginals[2],enhancedGenerated,presentedEnhanced;TextureConvert nrConvert;FrameGenerator enhancedVfg;
  bool ecoNeedsSubmit=false,ecoWaiting=false,rawFgDone=false,enhancedFgDone=false,displayedEco=false;
  bool pipelineEco=false;uint64_t lastNrFrames=0;float nrFps=0;
  NrControls nrControls;nrControls.load(root/L"nr-settings.txt");nrControls.enabled=FRAMELAB_WITH_NR&&args.find(L"--nr")!=std::wstring::npos;auto savedNrControls=nrControls;
  if(args.find(L"--nr-economy")!=std::wstring::npos)nrControls.economy=true;
  if(args.find(L"--nr-standard")!=std::wstring::npos)nrControls.economy=false;
  if(!argument(L"--nr-height").empty()){auto limit=_wtoi(argument(L"--nr-height").c_str());nrControls.height=limit==0?0:std::clamp(limit,180,8192);}
  if(!argument(L"--nr-passes").empty())nrControls.passes=std::clamp(_wtoi(argument(L"--nr-passes").c_str()),1,4);
  std::unique_ptr<Enhancer> enhancer;std::vector<std::unique_ptr<Enhancer>> retiringEnhancers;QueuedImage nrPending{};std::string nrFailure;
  uint64_t currentScene=0,displayedScene=0;NrHistory nrHistory;bool nrReset=true,nrWasEnabled=nrControls.enabled,nrDiscardPending=false;
  auto retireEnhancer=[&]{if(enhancer){if(displayed==enhancer->original.view.Get())displayed=presentedImage.view.Get();enhancer->requestStop();retiringEnhancers.push_back(std::move(enhancer));}nrReset=true;nrHistory.clear();};
  uint64_t presentationDrops=0; double displayedAt=0,lastDraw=-1,nrSubmittedAt=0,nrQueueWaitMs=0,lastContentAt=0;
  bool presentationPermit=false;bool debugFrames=true,debugText=true,fullscreenStats=false;int debugDuration=0;std::string lastDebugSettings;
  double presentationWaitMs=0,loopPrevious=0,loopGapMaxMs=0,presentCpuMs=0;uint64_t occludedPresents=0;
  std::ofstream pacingLog;if(args.find(L"--verify-pacing")!=std::wstring::npos){pacingLog.open(root/L"logs"/(session+"-pacing.csv"));pacingLog<<"time_s,generated,source_id,queue,source_A,source_B,phase,callback_to_present_ms\n";}
  auto enqueueImage=[&](Texture& texture,bool generatedFrame,uint64_t id,uint64_t a,uint64_t b,double position,double sourceTime,Texture* enhanced=nullptr){
   // Bounded latency: discard stale output when the display cannot keep up.
   if(readyImages.size()>=6){presentationDrops+=readyImages.size();readyImages.clear();}
   QueuedImage q{};if(imagePool.empty())q.image.create(g.device.Get(),w,h);else{q.image=std::move(imagePool.back());imagePool.pop_back();}g.context->CopyResource(q.image.texture.Get(),texture.texture.Get());
   q.generated=generatedFrame;q.id=id;q.a=a;q.b=b;q.position=position;q.sourceTime=sourceTime;q.scene=currentScene;if(enhanced){if(enhancedPool.empty())q.enhanced.create(g.device.Get(),w,h);else{q.enhanced=std::move(enhancedPool.back());enhancedPool.pop_back();}g.context->CopyResource(q.enhanced.texture.Get(),enhanced->texture.Get());q.nrApplied=true;}readyImages.push_back(std::move(q));
  };
  Comparison comparison;uint64_t pairId=0,sourceIds[2]{};bool autoExported=false,autoRecorded=false,autoAnalysis=false;std::filesystem::path exportedAnalysis;HANDLE analysisExporter=nullptr;bool displayedGenerated=false;uint64_t displayedId=0,displayedA=0,displayedB=0;double displayedPosition=0;
  std::string status="Ready. Select a capture device or start a test source.";Packet packet;double previousArrival=0,pairArrival=0,pairInterval=1./30,displaySourceTime=0;
  bool newContent=false,newGenerated=false;uint64_t totalContent=0,totalPresents=0,totalGenerated=0;
  float inFps=0,contentFps=0,outFps=0,presentFps=0,genFps=0,latency=-1,p95=-1,gpuMs=-1,dxgiRate=-1;
  DisplayVerification displayVerification;float confirmedGenFps=-1;uint64_t lastConfirmedContent=0,lastConfirmedGen=0;
  History inputHistory,uniqueHistory,outputHistory,latencyHistory,gpuTimeHistory,utilHistory;
  std::vector<float> latencySamples;double start=now(),lastStats=start;uint64_t lastRaw=0,lastUnique=0,lastContent=0,lastPresents=0,lastGenerated=0;
  auto settingsJson=[&]{
   auto values=Diagnostics::object({{"width",double(w)},{"height",double(h)},{"capture_requested_fps",requestedFps},{"fg",double(fg)},{"multiplier",double(multiplier)},{"quality",double(quality)},{"vsync",double(vsync)},{"dedup",double(mailbox.dedup.load())},{"automatic_cuts",double(automaticCuts)},{"nr_enabled",double(nrControls.enabled)},{"nr_economy",double(nrControls.economy)},{"nr_height_limit",double(nrControls.height)},{"nr_passes",double(nrControls.passes)},{"nr_strength",nrControls.strength},{"nr_motion",double(nrControls.motion)},{"nr_runtime",double(nrControls.runtime)},{"nr_view",double(nrControls.view)},{"subtitles",double(subtitleSettings.enabled)},{"subtitle_model",double(subtitleSettings.model)},{"ocr_engine",double(subtitleSettings.ocrEngine)},{"ocr_interval_ms",double(subtitleSettings.intervalMs)},{"ocr_left",subtitleSettings.left},{"ocr_top",subtitleSettings.top},{"ocr_right",subtitleSettings.right},{"ocr_bottom",subtitleSettings.bottom},{"caption_hold_s",subtitleSettings.holdSeconds},{"caption_linger_s",subtitleSettings.lingerSeconds},{"subtitle_font_percent",subtitleSettings.fontPercent},{"subtitle_position",subtitleSettings.position},{"subtitle_horizontal",subtitleSettings.horizontal},{"subtitle_placement",double(subtitleSettings.placement)},{"subtitle_background",subtitleSettings.background},{"subtitle_show_original",double(subtitleSettings.showOriginal)},{"subtitle_cover_source",double(subtitleSettings.coverSource)},{"subtitle_horizontal_align",double(subtitleSettings.horizontalAlign)},{"subtitle_vertical_align",double(subtitleSettings.verticalAlign)},{"subtitle_gap_percent",subtitleSettings.gapPercent},{"subtitle_edge_fallback",double(subtitleSettings.edgeFallback)},{"display_fit",double(displayControls.fit)},{"audio_volume",double(audioVolume)},{"audio_muted",double(audioMuted)},{"nr_split",nrControls.split},{"legacy_nr_scheduling",double(legacyNrScheduling)}});
   const auto& sp=subtitleSettings.style;auto styleJson=Diagnostics::object({{"enabled",double(sp.enabled)},{"height_ratio",sp.height},{"r",sp.rgb[0]},{"g",sp.rgb[1]},{"b",sp.rgb[2]},{"height_tolerance",sp.heightTolerance},{"color_tolerance",sp.colorTolerance},{"inspector_open",double(subtitleSettings.inspectOcr)},{"sample_id",double(sp.sampleId)},{"sample_frame_height",double(sp.sampleFrameHeight)}});
   values.pop_back();return values+",\"ocr_style\":"+styleJson+",\"backend\":"+Diagnostics::quote(vfg.name())+",\"source\":"+Diagnostics::quote(isDemo?(activeVideoName.empty()?"synthetic":activeVideoName):(devices.empty()?"none":devices[selected].friendlyName))+"}";
  };
  auto startDebug=[&]{
   lastContentAt=0;lastDebugSettings=settingsJson();auto meta="{\"build\":\"preview.16\",\"gpu\":"+Diagnostics::quote(gpu.name)+",\"cpu\":"+Diagnostics::quote(systemTelemetry.cpuName)+",\"logical_processors\":"+std::to_string(systemTelemetry.threads)+",\"settings\":"+lastDebugSettings+",\"timing_note\":\"App latency excludes capture hardware and scanout. CPU/GPU totals include other apps. NR GPU phases use D3D12 timestamps; worker time also includes GPU waits. OCR/MT times include IPC; displayed caption delay includes queues and reading hold.\"}";
   diagnostics.start(root/L"logs",meta,debugFrames,debugText,debugDuration);
  };
  auto resetPipeline=[&]{ecoNeedsSubmit=ecoWaiting=rawFgDone=enhancedFgDone=displayedEco=false;lastNrFrames=0;nrFps=0;enhancedVfg.reset();freezePreview=false;subtitles.reset();retireEnhancer();analysis.finish("Source or settings changed");analysis.cancelPending();comparison.clear();readyImages.clear();imagePool.clear();enhancedPool.clear();pacer.reset();displayVerification.clearHistory();g.context->PSSetShaderResources(0,0,nullptr);vfg.reset();vfgInitialized=false;havePrevious=false;pending=false;initFailed=false;gpuMs=-1;displayed=nullptr;current=0;previousArrival=0;};
  auto stopSource=[&]{localVideo.stop();isLocalVideo=false;subtitleAutoPaused=false;subtitleWarmupOptout=false;acceptedFileEpoch=0;demoRun=false;if(demo.joinable()){demo.request_stop();demo.join();}capture.stop();live=false;isDemo=false;activeVideoName.clear();resetPipeline();mailbox.clear();lastRaw=lastUnique=0;};
  auto startDemo=[&]{stopSource();isDemo=true;live=true;demoRun=true;status="TEST SOURCE: 30 unique frames inside a 60 Hz stream. Synthetic input.";
   const bool largeDemo=test1440;
   demo=std::jthread([&,largeDemo](std::stop_token token){const unsigned dw=largeDemo?2560:1280,dh=largeDemo?1440:720;std::vector<unsigned char> pixels(size_t(dw)*dh*4);uint64_t tick=0;auto next=Clock::now();
    while(demoRun&&!token.stop_requested()){if(tick%2==0){unsigned phase=unsigned((tick/2)*9)%dw;for(unsigned y=0;y<dh;++y)for(unsigned x=0;x<dw;++x){size_t i=(size_t(y)*dw+x)*4;bool box=((x+dw-phase)%dw)<dw/8&&y>dh/3&&y<2*dh/3;bool grid=x%80<2||y%80<2;pixels[i]=box?125:grid?48:23;pixels[i+1]=box?230:grid?42:18;pixels[i+2]=box?55:grid?36:12;pixels[i+3]=255;}}
     DirectShowCapture::Frame f{};f.width=f.sampleWidth=dw;f.height=f.sampleHeight=dh;f.stride=dw*4;f.data=pixels.data();f.dataSize=pixels.size();mailbox.ingest(f);++tick;next+=std::chrono::nanoseconds(16666667);std::this_thread::sleep_until(next);}
   });log("Demo started");};
  auto startSample=[&](std::filesystem::path requested={}){stopSource();auto video=requested.empty()?root/L"samples"/L"action.mp4":requested;
   if(!requested.empty()&&!std::filesystem::is_regular_file(video)){status="Video file not found: "+video.string();return;}
   if(std::filesystem::exists(video)){activeVideoName=video.filename()==L"action.mp4"?"CS:GO | 1920 x 1080 | 3 s":video.filename()==L"gta.mp4"?"GTA capture | 1280 x 720 | 3 s":video.filename().string();log("Video source: "+video.string());isDemo=true;live=true;demoRun=true;status="Action gameplay | "+std::to_string(60/sampleStride)+" FPS limit | video only";const double rate=60.0/sampleStride;
    demo=std::jthread([&,video,rate](std::stop_token token){try{replayVideoFile(video,rate,[&](const auto& frame){mailbox.ingest(frame);},token);}catch(const std::exception& e){std::lock_guard lock(replayErrorMutex);replayError=e.what();}});return;}
   auto path=root/L"samples"/L"csgo-1080p60.bgra";if(!std::filesystem::exists(path)||std::filesystem::file_size(path)<1920ull*1080*4*2){status="Action sample missing: expected samples/action.mp4";return;}isDemo=true;live=true;demoRun=true;status="CS:GO replay | 1080p | "+std::to_string(60/sampleStride)+" input FPS | video only";
   int stride=sampleStride;demo=std::jthread([&,path,stride](std::stop_token token){
    const size_t bytes=1920ull*1080*4;std::ifstream file(path,std::ios::binary);std::vector<unsigned char> pixels(bytes);auto next=Clock::now();size_t index=0,count=std::filesystem::file_size(path)/bytes;
    while(demoRun&&!token.stop_requested()){file.seekg((index%count)*bytes);if(!file.read((char*)pixels.data(),bytes))break;DirectShowCapture::Frame f{};f.width=f.sampleWidth=1920;f.height=f.sampleHeight=1080;f.stride=1920*4;f.data=pixels.data();f.dataSize=bytes;mailbox.ingest(f);index+=stride;next+=std::chrono::nanoseconds(1000000000ll*stride/60);std::this_thread::sleep_until(next);}
   });};
  auto startLocalVideo=[&](std::filesystem::path path,double position=0,bool paused=false){
   stopSource();localVideoPath=std::move(path);isLocalVideo=true;isDemo=true;live=true;
   auto filename=localVideoPath.filename().u8string();activeVideoName.assign(filename.begin(),filename.end());
   status=nrControls.text("Opening local video...","Otevírání místního videa...");
   localVideo.open(localVideoPath,fileOutputs[fileAudioOutput].id,audioVolume,audioMuted,[&](const auto& frame,uint64_t epoch){mailbox.ingest(frame,epoch);});
   if(position>0)localVideo.seek(position);localVideo.pause(paused);
  };
  auto chooseLocalVideo=[&]{
   wchar_t file[32768]{};OPENFILENAMEW picker{sizeof(picker)};picker.hwndOwner=window;
   picker.lpstrFilter=L"Video files\0*.mp4;*.m4v;*.mov;*.mkv;*.avi;*.wmv;*.webm;*.m2ts;*.ts\0All files\0*.*\0\0";
   picker.lpstrFile=file;picker.nMaxFile=DWORD(std::size(file));picker.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER;
   picker.lpstrTitle=czech?L"Otevřít vlastní video / film":L"Open your video / movie";
   if(GetOpenFileNameW(&picker))startLocalVideo(file);
   else if(auto error=CommDlgExtendedError())status="Video picker error: "+std::to_string(error);
  };
  auto startCapture=[&]{stopSource();if(devices.empty()){status="No DirectShow capture device found.";return;}try{DirectShowCapture::Options o;o.deviceMoniker=devices[selected].monikerDisplayName;o.enableAudio=true;o.audioOutputMoniker=outputs[audioOutput].monikerDisplayName;o.audioVolume=audioVolume;o.audioMuted=audioMuted;if(resolution>0&&resolution<=int(resolutions.size())){auto mode=resolutions[resolution-1];o.desiredWidth=mode.width;o.desiredHeight=mode.height;o.desiredFrameRate=requestedFps;}
    capture.start([&](const auto& f){mailbox.ingest(f);},o);live=true;status="Capture running: "+devices[selected].friendlyName;log(status);
   }catch(const std::exception& e){status=e.what();log(status);}};
  auto startRecordedReplay=[&]{
   stopSource();auto replay=std::make_shared<RecordedReplay>();
   replay->load(argument(L"--replay-recording"),std::stod(argument(L"--replay-from",L"9.5")),std::stod(argument(L"--replay-length",L"3")));
   isDemo=true;live=true;demoRun=true;status="Recorded RAW replay | original arrival timing | video only";
   log("Recorded RAW replay: "+std::filesystem::path(argument(L"--replay-recording")).string()+" frames="+std::to_string(replay->frames.size())+" legacy_wait="+std::to_string(legacyOutputWait));
   demo=std::jthread([&,replay](std::stop_token token){auto begin=Clock::now();size_t cycle=0;
    while(demoRun&&!token.stop_requested()){
     for(const auto& frame:replay->frames){
      auto deadline=begin+std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(cycle*replay->duration+frame.time));
      std::this_thread::sleep_until(deadline);if(!demoRun||token.stop_requested())return;
      DirectShowCapture::Frame f{};f.width=f.sampleWidth=replay->width;f.height=f.sampleHeight=replay->height;f.stride=replay->width*4;f.data=frame.pixels.data();f.dataSize=frame.pixels.size();mailbox.ingest(f);
     }++cycle;
    }
   });
  };

  auto saveLiveSession=[&]{
   if(uiPreview)return;LiveSession state;
   state.put("live",live);state.put("source_kind",isLocalVideo?"file":isDemo?"demo":"capture");state.put("capture_device",devices.empty()?"":devices[selected].friendlyName);
   state.put("capture_width",resolution>0?resolutions[resolution-1].width:0);state.put("capture_height",resolution>0?resolutions[resolution-1].height:0);state.put("capture_fps",requestedFps);
   state.put("fg",fg);state.put("quality",quality);state.put("multiplier",multiplier);state.put("optical",vfg.useOptical);state.put("automatic_cuts",automaticCuts);state.put("dedup",mailbox.dedup.load());state.put("dedup_threshold",mailbox.threshold.load());state.put("vsync",vsync);state.put("sample_stride",sampleStride);
   state.put("nr_enabled",nrControls.enabled);state.put("subtitles_enabled",subtitleSettings.enabled);state.put("subtitle_show_region",subtitleSettings.showRegion);state.put("subtitle_edit",subtitleSettings.editOverlay);
   state.put("comparison_enabled",comparison.enabled);state.put("comparison_zoom",comparison.zoom);state.put("comparison_x",comparison.focusX);state.put("comparison_y",comparison.focusY);state.put("show_panel",showPanel);state.put("fullscreen",fullscreen);
   state.put("debug_active",diagnostics.active());state.put("debug_frames",debugFrames);state.put("debug_text",debugText);state.put("debug_duration",debugDuration);state.put("fullscreen_stats",fullscreenStats);
   auto local=localVideo.snapshot();auto path=localVideoPath.u8string();state.put("file",std::string(path.begin(),path.end()));state.put("file_position",local.position);state.put("file_paused",local.paused);
   RECT rect{};GetWindowRect(window,&rect);state.put("window_x",rect.left);state.put("window_y",rect.top);state.put("window_width",rect.right-rect.left);state.put("window_height",rect.bottom-rect.top);state.put("window_maximized",IsZoomed(window));
   state.save(root/L"runtime-session.txt");
  };
  if(resume.valid){
   auto name=resume.text("capture_device");bool found=false;for(size_t i=0;i<devices.size();++i)if(devices[i].friendlyName==name){selected=int(i);found=true;break;}
   refreshModes();resolution=0;auto cw=resume.number("capture_width"),ch=resume.number("capture_height");for(size_t i=0;i<resolutions.size();++i)if(resolutions[i].width==cw&&resolutions[i].height==ch)resolution=int(i)+1;
   requestedFps=resume.number("capture_fps");fg=resume.number("fg",fg)!=0;quality=std::clamp(int(resume.number("quality",quality)),0,2);multiplier=std::clamp(int(resume.number("multiplier",2)),2,4);vfg.useOptical=resume.number("optical")!=0;automaticCuts=resume.number("automatic_cuts",1)!=0;mailbox.dedup=resume.number("dedup",1)!=0;mailbox.threshold=float(resume.number("dedup_threshold"));vsync=resume.number("vsync",1)!=0;sampleStride=std::clamp(int(resume.number("sample_stride",2)),1,4);
   nrControls.enabled=FRAMELAB_WITH_NR&&resume.number("nr_enabled")!=0;subtitleStartupPending=resume.number("subtitles_enabled")!=0;subtitleSettings.showRegion=resume.number("subtitle_show_region")!=0;subtitleSettings.editOverlay=resume.number("subtitle_edit")!=0;
   comparison.enabled=resume.number("comparison_enabled",1)!=0;comparison.zoom=float(resume.number("comparison_zoom",1));comparison.focusX=float(resume.number("comparison_x",.5));comparison.focusY=float(resume.number("comparison_y",.5));showPanel=resume.number("show_panel",1)!=0;
   debugFrames=resume.number("debug_frames",1)!=0;debugText=resume.number("debug_text",1)!=0;debugDuration=std::clamp(int(resume.number("debug_duration")),0,900);fullscreenStats=resume.number("fullscreen_stats")!=0;
   SetWindowPos(window,nullptr,int(resume.number("window_x",100)),int(resume.number("window_y",80)),std::max(640,int(resume.number("window_width",1480))),std::max(480,int(resume.number("window_height",900))),SWP_NOZORDER|SWP_NOACTIVATE);
   if(resume.number("window_maximized"))ShowWindow(window,SW_SHOWMAXIMIZED);if(resume.number("fullscreen"))SendMessageW(window,WM_KEYDOWN,VK_F11,0);
   if(resume.number("live")){auto kind=resume.text("source_kind");if(kind=="file")startLocalVideo(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(resume.text("file").c_str()))),resume.number("file_position"),resume.number("file_paused")!=0);else if(kind=="demo")startDemo();else if(found)startCapture();else status="Saved capture device unavailable; choose a source";}
   if(resume.number("debug_active"))startDebug();
  }
  if(!argument(L"--file").empty())startLocalVideo(argument(L"--file"));else if(!argument(L"--replay-recording").empty())startRecordedReplay();else if(args.find(L"--sample")!=std::wstring::npos||!argument(L"--video").empty())startSample(argument(L"--video"));else if(demoRequested)startDemo();else if(captureRequested)startCapture();else if(!resume.valid&&args.find(L"--welcome")!=std::wstring::npos)startSample(root/L"samples"/L"gta.mp4");
  if(args.find(L"--fullscreen-stats")!=std::wstring::npos)fullscreenStats=true;
  log("FrameLab started GPU="+std::string(gpu.name)+" quality="+std::to_string(quality)+" vsync="+std::to_string(vsync));
  if(!argument(L"--debug-seconds").empty()){debugDuration=std::clamp(_wtoi(argument(L"--debug-seconds").c_str()),5,900);debugText=args.find(L"--debug-text")!=std::wstring::npos;startDebug();}
  else if(args.find(L"--debug-start")!=std::wstring::npos){debugDuration=0;debugFrames=args.find(L"--debug-no-frames")==std::wstring::npos;debugText=args.find(L"--debug-no-text")==std::wstring::npos;startDebug();}
  GpuPollWait gpuPollWait;
  auto renderFrame=[&]{
   if(frameBusy||!running)return;
   frameBusy=true;struct FrameGuard{~FrameGuard(){frameBusy=false;}} frameGuard;
   const double loopNow=now();if(loopPrevious>0)loopGapMaxMs=std::max(loopGapMaxMs,(loopNow-loopPrevious)*1000);loopPrevious=loopNow;
   if(stopAfter>0&&now()-start>stopAfter){running=false;return;}
   if(resized){resized=false;const auto rw=resizeW,rh=resizeH;if(rw&&rh)g.resize(rw,rh);}
   if(IsIconic(window))return;
   const bool ecoActive=nrControls.enabled&&nrControls.economy;
   if(ecoActive!=pipelineEco){resetPipeline();pipelineEco=ecoActive;log(ecoActive?"NR order: source > NR > FG (eco)":"NR order: source > FG > NR (standard)");}
   if(legacyOutputWait&&g.waitable)presentationPermit=WaitForSingleObject(g.waitable,20)==WAIT_OBJECT_0;
   if(subtitleStartupPending){const auto capability=ocrSetup.snapshot();if(subtitleSettings.ocrEngine==0||capability.available>=0){subtitleStartupPending=false;subtitleSettings.enabled=subtitleSettings.ocrEngine==0?rapidOcrReady:capability.available==1;subtitles.configure(subtitleSettings);}}
   auto localSnapshot=localVideo.snapshot();
   if(isLocalVideo&&localSnapshot.frames&&status==nrControls.text("Opening local video...","Otevírání místního videa..."))status=nrControls.text("Local video playback","Přehrávání místního videa");
   if(isLocalVideo&&!localSnapshot.opening&&!localSnapshot.ended){
    const auto caption=subtitles.snapshot(localSnapshot.paused);
    const bool warming=subtitleSettings.enabled&&(caption.loading||caption.activeModel!=subtitleSettings.model);
    if(warming&&!subtitleWarmupOptout&&!localSnapshot.paused){localVideo.pause(true);subtitleAutoPaused=true;localSnapshot.paused=true;}
    else if(subtitleAutoPaused&&!warming){subtitles.reset();localVideo.pause(false);subtitleAutoPaused=false;localSnapshot.paused=false;}
   }
   subtitleTrace.write(isLocalVideo?localSnapshot.position:now()-start,subtitles.snapshot(isLocalVideo&&localSnapshot.paused));
   if(isLocalVideo&&!localSnapshot.error.empty()){status=localSnapshot.error;live=false;}
   double t=now();diagnostics.tick();analysis.poll(g);analysis.tick();if(analysisExporter&&WaitForSingleObject(analysisExporter,0)==WAIT_OBJECT_0){CloseHandle(analysisExporter);analysisExporter=nullptr;}
   if(!analysis.active&&!analysis.writing&&!analysis.folder.empty()&&analysis.folder!=exportedAnalysis&&std::filesystem::exists(analysis.folder/L"result.json")){
    const auto portablePython=root.parent_path()/L"tools"/L"python"/L"python.exe";
    const std::wstring python=std::filesystem::exists(portablePython)?L"\""+portablePython.wstring()+L"\"":L"python";
    std::wstring command=python+L" \""+(root.parent_path()/L"tools"/L"export_analysis.py").wstring()+L"\" \""+analysis.folder.wstring()+L"\"";STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};
    if(CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,root.parent_path().c_str(),&si,&pi)){CloseHandle(pi.hThread);analysisExporter=pi.hProcess;exportedAnalysis=analysis.folder;}else{status="Clip exporter could not start: "+std::to_string(GetLastError());exportedAnalysis=analysis.folder;}
   }
   auto startAnalysis=[&]{try{if(analysisExporter){status="Wait for the previous clips to finish exporting";return;}if(!live||!w||!h){status="Start a source before recording";return;}analysis.backendName=vfg.name();analysis.pipelineSettings=settingsJson();analysis.outputScheduler=legacyOutputWait?"legacy-display-wait":"independent-gpu-pump-content-deadline";analysis.start(root,isDemo?"Test/file source":devices[selected].friendlyName,w,h,quality,multiplier,automaticCuts&&!vfg.useOptical,vsync,mailbox.dedup,mailbox.threshold,gpu.name,fg,inFps,contentFps);subtitleSettings.save(analysis.folder/L"subtitle-settings.txt");nrControls.save(analysis.folder/L"nr-settings.txt");analysis.settings(settingsJson());}catch(const std::exception& e){status=e.what();analysis.startError(status);log("Recording start failed: "+status);}};
   if(!autoAnalysis&&t-start>3&&args.find(L"--record-analysis")!=std::wstring::npos&&w&&h){startAnalysis();autoAnalysis=true;}

   auto error=capture.consumeLastError();if(!error.empty()){status=error;log(status);}
   {std::lock_guard lock(replayErrorMutex);if(!replayError.empty()){status="Replay failed: "+replayError;log(status);replayError.clear();live=false;}}
   try{
  if(pending){if(!rawFgDone)rawFgDone=vfg.ready();if(ecoActive&&!enhancedFgDone)enhancedFgDone=enhancedVfg.ready();}
  if(pending&&rawFgDone&&(!ecoActive||enhancedFgDone)){comparison.backendName=vfg.name();comparison.capture(g,originals[1-current],generated,originals[current],w,h,++pairId,sourceIds[1-current],sourceIds[current],pairInterval*1000,quality,double(generatedIndex)/multiplier);
   if(!autoRecorded&&pairId>=150&&args.find(L"--record-pairs")!=std::wstring::npos){comparison.startRecording();autoRecorded=true;}
   analysis.generated(g,generated,w,h,pairId,sourceIds[1-current],sourceIds[current],double(generatedIndex)/multiplier);
   comparison.record(g,root,pairArrival);
   if(!autoExported&&pairId>=15&&args.find(L"--export-pair")!=std::wstring::npos){log("Comparison exported: "+comparison.save(g,root).string());autoExported=true;}
   pending=false;rawFgDone=enhancedFgDone=false;gpuMs=vfg.gpuMs()+(ecoActive?enhancedVfg.gpuMs():0);
   enqueueImage(generated,true,pairId,sourceIds[1-current],sourceIds[current],double(generatedIndex)/multiplier,pairArrival,ecoActive?&enhancedGenerated:nullptr);
   if(generatedIndex<unsigned(multiplier-1)){++generatedIndex;vfg.generate(current,false,generatedIndex,multiplier);if(ecoActive)enhancedVfg.generate(current,false,generatedIndex,multiplier);pending=true;}
   else enqueueImage(originals[current],false,sourceIds[current],sourceIds[current],sourceIds[current],0,pairArrival,ecoActive?&enhancedOriginals[current]:nullptr);
  }
    // Never overwrite a texture while its generated frame or real partner is pending.
    if(!freezePreview&&!pending&&!ecoNeedsSubmit&&!ecoWaiting&&readyImages.size()<2&&mailbox.take(packet)){
     if(isLocalVideo&&packet.sourceEpoch!=acceptedFileEpoch){resetPipeline();acceptedFileEpoch=packet.sourceEpoch;}
     currentScene=packet.scene;
     if(packet.w!=w||packet.h!=h){resetPipeline();w=packet.w;h=packet.h;for(auto& tex:originals)tex.create(g.device.Get(),w,h);for(auto& tex:enhancedOriginals)tex={};enhancedGenerated={};presentedEnhanced={};generated.create(g.device.Get(),w,h);presentedImage.create(g.device.Get(),w,h);log("Input size "+std::to_string(w)+"x"+std::to_string(h));}
     if(fg&&!vfgInitialized&&!initFailed){status=std::string("Loading ")+vfg.name();log(status);vfg.initialize(g.device.Get(),generated.texture.Get(),w,h,unsigned(quality),root/L"runtime",automaticCuts);vfgInitialized=true;havePrevious=false;status=std::string(vfg.name())+" active | "+std::to_string(w)+" x "+std::to_string(h);log(status);}
     if(ecoActive&&!enhancedOriginals[0].texture){for(auto& tex:enhancedOriginals)tex.create(g.device.Get(),w,h);enhancedGenerated.create(g.device.Get(),w,h);presentedEnhanced.create(g.device.Get(),w,h);}
     if(ecoActive&&fg&&vfgInitialized&&!havePrevious&&!ecoWaiting){enhancedVfg.useOptical=vfg.useOptical;enhancedVfg.initialize(g.device.Get(),enhancedGenerated.texture.Get(),w,h,unsigned(quality),root/L"runtime",automaticCuts);}
     current=1-current;sourceIds[current]=packet.serial;g.context->UpdateSubresource(originals[current].texture.Get(),0,nullptr,packet.data.data(),w*4,0);
     if(fg&&vfgInitialized)vfg.upload(packet.data.data(),current);
     pairArrival=packet.received;pairInterval=havePrevious?std::clamp(packet.received-previousArrival,1./120,.1):1./30;
     if(ecoActive){ecoNeedsSubmit=true;}
     else if(fg&&vfgInitialized&&havePrevious&&packet.received-previousArrival<.12){generatedIndex=1;vfg.generate(current,false,generatedIndex,multiplier);pending=true;}
     else enqueueImage(originals[current],false,packet.serial,packet.serial,packet.serial,0,packet.received);
     if(!ecoActive){havePrevious=true;previousArrival=packet.received;}
    }
   }catch(const std::exception& e){status=std::string("VFG disabled: ")+e.what();log(status);fg=false;initFailed=true;pending=false;displayedEco=false;rawFgDone=enhancedFgDone=false;enhancedVfg.reset();readyImages.clear();vfg.reset();vfgInitialized=false;havePrevious=false;displayed=originals[current].view.Get();}
   // Keep advancing completed GPU work while the previous presentation is
   // waiting for the display. Dequeue only when a new Present is possible.
   // Polling is bounded to 1 ms and the existing output queue remains bounded.
   pacer.setCadence(mailbox.sourceInterval.load(),fg&&vfgInitialized?multiplier:1);
   if(legacyNrScheduling&&!legacyOutputWait){
    const auto time=now();const bool imageDue=!readyImages.empty()&&(!fg||pacer.due(time));
    // A repeated video image needs no redraw. Keep controls responsive at 60 Hz,
    // but leave the rest of the CPU/GPU budget for new interpolation phases.
    if((!imageDue&&time-lastDraw<1./60)||(!uiPreview&&g.waitable&&WaitForSingleObject(g.waitable,0)!=WAIT_OBJECT_0)){
     gpuPollWait.wait();return;
    }
   }
   retiringEnhancers.erase(std::remove_if(retiringEnhancers.begin(),retiringEnhancers.end(),[](const auto& nr){return nr->stopped();}),retiringEnhancers.end());
   if(nrControls.enabled!=nrWasEnabled){nrReset=true;if(!nrControls.enabled)nrDiscardPending=true;nrWasEnabled=nrControls.enabled;}
   if(nrControls.enabled&&!enhancer&&retiringEnhancers.empty()&&w&&h&&live){
    enhancer=std::make_unique<Enhancer>(g,w,h,nrControls.height,root,root/L"runtime"/(nrControls.runtime?L"nr-original":L"nr-sf"),nrControls.passes);nrFailure.clear();nrReset=true;nrDiscardPending=false;
   }
   if(enhancer&&enhancer->state()==Enhancer::State::Failed){nrFailure=enhancer->error();log("NR disabled: "+nrFailure);nrControls.enabled=false;retireEnhancer();}
   if(ecoActive&&!freezePreview&&enhancer){try{
    if(ecoWaiting&&enhancer->take()){
     auto timing=enhancer->timings();diagnostics.event("nr_complete",{{"frame_id",double(sourceIds[current])},{"generated",0},{"economy",1},{"worker_ms",enhancer->processingMs()},{"inference_gpu_ms",timing.gpu.inferenceGpuMs}});
     ecoWaiting=false;nrConvert.copy(g,enhancer->result.view.Get(),enhancedOriginals[current],w,h);
     if(fg&&vfgInitialized)enhancedVfg.upload(enhancedOriginals[current].texture.Get(),current);
     if(fg&&vfgInitialized&&havePrevious&&pairArrival-previousArrival<.12){generatedIndex=1;vfg.generate(current,false,generatedIndex,multiplier);enhancedVfg.generate(current,false,generatedIndex,multiplier);pending=true;rawFgDone=enhancedFgDone=false;}
     else enqueueImage(originals[current],false,sourceIds[current],sourceIds[current],sourceIds[current],0,pairArrival,&enhancedOriginals[current]);
     havePrevious=true;previousArrival=pairArrival;
    }
    if(ecoNeedsSubmit&&enhancer->state()==Enhancer::State::Idle){
     const bool reset=nrHistory.needsReset(currentScene,pairArrival,nrReset||!nrControls.motion);
     if(enhancer->submit(originals[current].texture.Get(),reset,nrControls.motion)){
      ecoNeedsSubmit=false;ecoWaiting=true;nrSubmittedAt=now();nrQueueWaitMs=(nrSubmittedAt-pairArrival)*1000;nrReset=false;
      nrHistory.submitted(currentScene,pairArrival,reset);diagnostics.event("nr_submit",{{"frame_id",double(sourceIds[current])},{"generated",0},{"economy",1},{"reset",double(reset)},{"capture_to_submit_ms",nrQueueWaitMs}});
     }
    }
   }catch(const std::exception& e){nrFailure=e.what();log("NR eco disabled: "+nrFailure);nrControls.enabled=false;retireEnhancer();}}
   // NR computes one frame ahead. Presentation cadence gates consumption,
   // never the start of the next inference. This avoids adding inference time
   // to every display interval and keeps the GPU pump independent of redraws.
   if(!legacyNrScheduling&&!presentationPermit)presentationPermit=uiPreview||!g.waitable||WaitForSingleObject(g.waitable,0)==WAIT_OBJECT_0;
   const bool displayReady=legacyNrScheduling||presentationPermit;
   const bool outputDue=!fg||pacer.due(now());
   if(!ecoActive&&!freezePreview&&enhancer&&(legacyNrScheduling||!nrControls.enabled||(displayReady&&outputDue))&&enhancer->take()){
    auto timing=enhancer->timings();diagnostics.event("nr_complete",{{"frame_id",double(nrPending.id)},{"generated",double(nrPending.generated)},{"worker_ms",enhancer->processingMs()},{"submit_to_ready_ms",timing.submitToReadyMs},{"ready_to_take_ms",timing.readyToTakeMs},{"prepare_gpu_ms",timing.gpu.prepareGpuMs},{"inference_gpu_ms",timing.gpu.inferenceGpuMs},{"finish_gpu_ms",timing.gpu.finishGpuMs}});
    if(nrControls.enabled&&!nrDiscardPending){g.context->CopyResource(presentedImage.texture.Get(),enhancer->original.texture.Get());displayed=enhancer->original.view.Get();newContent=true;newGenerated=nrPending.generated;displaySourceTime=nrPending.sourceTime;displayedGenerated=nrPending.generated;displayedId=nrPending.id;displayedA=nrPending.a;displayedB=nrPending.b;displayedPosition=nrPending.position;displayedScene=nrPending.scene;}
    else nrReset=true;
    nrDiscardPending=false;
   }
   const bool nrActive=!ecoActive&&nrControls.enabled&&enhancer&&enhancer->state()!=Enhancer::State::Loading;
   if(!freezePreview&&nrActive&&enhancer->state()==Enhancer::State::Idle&&nrReset&&readyImages.empty()&&displayed){
    nrPending.generated=displayedGenerated;nrPending.id=displayedId;nrPending.a=displayedA;nrPending.b=displayedB;nrPending.position=displayedPosition;nrPending.sourceTime=displaySourceTime;nrPending.scene=displayedScene;
    if(enhancer->submit(presentedImage.texture.Get(),true,nrControls.motion)){nrSubmittedAt=now();nrReset=false;nrHistory.submitted(displayedScene,displaySourceTime,true);}
   }
   if(!freezePreview&&!readyImages.empty()&&(nrActive?((!legacyNrScheduling||outputDue)&&enhancer->state()==Enhancer::State::Idle):(displayReady&&outputDue))){
   if(nrActive){
    auto recycle=[&](auto& skipped){imagePool.push_back(std::move(skipped.image));};
    if(legacyNrTrim){if(readyImages.size()>2)while(readyImages.size()>1){recycle(readyImages.front());readyImages.pop_front();++presentationDrops;}}
    else presentationDrops+=trimNrQueue(readyImages,fg?multiplier:1,mailbox.sourceInterval.load(),now(),recycle);
   }
    auto q=std::move(readyImages.front());readyImages.pop_front();
    if(nrActive){
     const bool reset=nrHistory.needsReset(q.scene,q.sourceTime,nrReset||!nrControls.motion);
     if(enhancer->submit(q.image.texture.Get(),reset,nrControls.motion)){nrSubmittedAt=now();nrQueueWaitMs=(nrSubmittedAt-q.sourceTime)*1000;diagnostics.event("nr_submit",{{"frame_id",double(q.id)},{"generated",double(q.generated)},{"reset",double(reset)},{"capture_to_submit_ms",nrQueueWaitMs}});nrPending=std::move(q);imagePool.push_back(std::move(nrPending.image));nrHistory.submitted(nrPending.scene,nrPending.sourceTime,reset);nrReset=false;}
    }else{
    g.context->CopyResource(presentedImage.texture.Get(),q.image.texture.Get());displayed=presentedImage.view.Get();displayedEco=q.nrApplied;if(q.nrApplied){g.context->CopyResource(presentedEnhanced.texture.Get(),q.enhanced.texture.Get());enhancedPool.push_back(std::move(q.enhanced));}
    newContent=true;newGenerated=q.generated;displaySourceTime=q.sourceTime;
    displayedGenerated=q.generated;displayedId=q.id;displayedA=q.a;displayedB=q.b;displayedPosition=q.position;displayedScene=q.scene;imagePool.push_back(std::move(q.image));
    }
   }
   if(t-lastStats>=1){saveLiveSession();double elapsed=t-lastStats;nrFps=float((nrHistory.frames>=lastNrFrames?nrHistory.frames-lastNrFrames:0)/elapsed);lastNrFrames=nrHistory.frames;auto raw=mailbox.raw.load(),uni=mailbox.unique.load();inFps=float((raw-lastRaw)/elapsed);contentFps=float((uni-lastUnique)/elapsed);outFps=float((totalContent-lastContent)/elapsed);presentFps=float((totalPresents-lastPresents)/elapsed);genFps=float((totalGenerated-lastGenerated)/elapsed);
    lastRaw=raw;lastUnique=uni;lastContent=totalContent;lastPresents=totalPresents;lastGenerated=totalGenerated;
    latency=-1;p95=-1;if(!latencySamples.empty()){float sum=0;for(float x:latencySamples)sum+=x;latency=sum/latencySamples.size();std::sort(latencySamples.begin(),latencySamples.end());p95=latencySamples[std::min(latencySamples.size()-1,size_t(std::ceil(latencySamples.size()*.95))-1)];latencySamples.clear();}
    gpu.poll();systemTelemetry.poll();auto previousMonitor=displayVerification.monitor;displayVerification.updateMonitor(g.swap.Get());if(previousMonitor!=displayVerification.monitor)log("Display mode: "+std::to_string(displayVerification.displayWidth)+"x"+std::to_string(displayVerification.displayHeight)+" @ "+std::to_string(displayVerification.displayHz)+" Hz");
    dxgiRate=displayVerification.available?float((displayVerification.content-lastConfirmedContent)/elapsed):-1;
    confirmedGenFps=displayVerification.available?float((displayVerification.generated-lastConfirmedGen)/elapsed):-1;
    lastConfirmedContent=displayVerification.content;lastConfirmedGen=displayVerification.generated;
    inputHistory.add(inFps);uniqueHistory.add(contentFps);outputHistory.add(outFps);latencyHistory.add(std::max(0.f,latency));gpuTimeHistory.add(std::max(0.f,gpuMs));utilHistory.add(std::max(0.f,gpu.utilization));
    if(analysis.active)analysis.settings(settingsJson());
    if(diagnostics.active()){
     auto currentSettings=settingsJson();if(currentSettings!=lastDebugSettings){diagnostics.settings(currentSettings);lastDebugSettings=currentSettings;}
     auto sub=subtitles.snapshot(isLocalVideo&&localSnapshot.paused);auto& sp=sub.performance;auto nt=enhancer?enhancer->timings():Enhancer::Timings{};
     RECT clientRect{};GetClientRect(window,&clientRect);
     diagnostics.sample({{"window_foreground",double(GetForegroundWindow()==window)},{"window_fullscreen",double(fullscreen)},{"window_client_width",double(clientRect.right)},{"window_client_height",double(clientRect.bottom)},{"background_qos",double(backgroundQos)},{"presentation_wait_ms",presentationWaitMs},{"loop_gap_max_ms",loopGapMaxMs},{"present_cpu_total_ms",presentCpuMs},{"occluded_presents",double(occludedPresents)},{"input_fps",inFps},{"changed_fps",contentFps},{"output_submitted_fps",outFps},{"generated_fps",genFps},{"present_calls_fps",presentFps},{"os_confirmed_fps",dxgiRate},{"os_generated_fps",confirmedGenFps},{"display_hz",double(displayVerification.displayHz)},{"app_latency_mean_ms",latency},{"app_latency_p95_ms",p95},{"fg_gpu_ms",gpuMs},{"gpu_percent",gpu.utilization},{"vram_used_mb",gpu.memoryMB},{"vram_free_mb",gpu.freeMemoryMB},{"gpu_temp_c",gpu.temp},{"gpu_power_w",gpu.power},{"cpu_system_percent",systemTelemetry.cpu},{"cpu_app_percent",systemTelemetry.appCpu},{"ram_used_mb",systemTelemetry.ramUsedMB},{"ram_total_mb",systemTelemetry.ramTotalMB},{"app_working_set_mb",systemTelemetry.appRamMB},{"app_private_mb",systemTelemetry.appPrivateMB},{"capture_drops",double(mailbox.dropped.load())},{"invalid_inputs",double(mailbox.invalid.load())},{"presentation_drops",double(presentationDrops)},{"output_queue",double(readyImages.size())},{"nr_enabled",double(nrControls.enabled)},{"nr_economy",double(nrControls.economy)},{"nr_evaluations_fps",nrFps},{"nr_state",enhancer?double(enhancer->state()):-1},{"nr_worker_ms",enhancer?enhancer->processingMs():-1},{"nr_submit_to_ready_ms",nt.submitToReadyMs},{"nr_ready_to_take_ms",nt.readyToTakeMs},{"nr_prepare_gpu_ms",nt.gpu.prepareGpuMs},{"nr_inference_gpu_ms",nt.gpu.inferenceGpuMs},{"nr_finish_gpu_ms",nt.gpu.finishGpuMs},{"nr_capture_to_submit_ms",nrQueueWaitMs},{"nr_history_resets",double(nrHistory.resets)},{"nr_frames",double(nrHistory.frames)},{"ocr_ms",sub.ocrMilliseconds},{"ocr_queue_ms",sp.ocrQueueMs},{"ocr_busy",double(sp.ocrBusy)},{"ocr_submitted",double(sp.submitted)},{"ocr_completed",double(sp.ocrCompleted)},{"ocr_superseded",double(sp.ocrSuperseded)},{"ocr_errors",double(sp.ocrErrors)},{"translation_ms",sp.translationMs},{"translation_queue_ms",sp.translationQueueMs},{"translation_busy",double(sp.translationBusy)},{"translations",double(sp.translations)},{"translation_cache_hits",double(sp.cacheHits)},{"translation_drops",double(sp.translationDrops)},{"translation_errors",double(sp.translationErrors)},{"caption_display_delay_ms",sp.displayLatencyMs},{"captions_shown",double(sp.shown)},{"caption_drops",double(sp.captionDrops)},{"subtitle_queue",double(sub.pending)},{"model_load_ms",sp.modelLoadMs},{"model_loading",double(sub.loading)},{"log_dropped_rows",double(diagnostics.dropped())}});
    }
    const auto nrTiming=enhancer?enhancer->timings():Enhancer::Timings{};
    csv<<std::fixed<<std::setprecision(3)<<t-start<<','<<inFps<<','<<contentFps<<','<<outFps<<','<<presentFps<<','<<genFps<<','<<latency<<','<<p95<<','<<gpuMs<<','<<gpu.utilization<<','<<gpu.memoryMB<<','<<mailbox.dropped<<','<<mailbox.invalid<<','<<dxgiRate<<','<<readyImages.size()<<','<<presentationDrops<<','<<confirmedGenFps<<','<<displayVerification.displayHz<<','<<pairInterval*1000<<','<<(enhancer?enhancer->processingMs():-1)<<','<<nrTiming.gpu.inferenceGpuMs<<','<<nrTiming.readyToTakeMs<<','<<loopGapMaxMs<<','<<presentCpuMs<<','<<(GetForegroundWindow()==window)<<','<<fullscreen<<','<<nrControls.enabled<<','<<nrControls.height<<','<<nrControls.passes<<','<<mailbox.sourceInterval.load()*1000<<','<<nrControls.economy<<','<<nrFps<<std::endl;lastStats=t;
   }
   if(t==lastStats){presentationWaitMs=0;loopGapMaxMs=0;presentCpuMs=0;}
   if(!legacyNrScheduling&&(!displayReady||(!newContent&&now()-lastDraw<1./60))){const auto waitStarted=now();gpuPollWait.wait();if(!displayReady)presentationWaitMs+=(now()-waitStarted)*1000;return;}
   // Fullscreen owns only its client cursor; never change the global ShowCursor counter.
   if(imageOnly)io.ConfigFlags|=ImGuiConfigFlags_NoMouseCursorChange;else io.ConfigFlags&=~ImGuiConfigFlags_NoMouseCursorChange;
   if(imageOnly&&GetForegroundWindow()==window){POINT p{};GetCursorPos(&p);if(WindowFromPoint(p)==window){
    const bool hide=hideOutputCursor();SetCursor(hide?nullptr:LoadCursor(nullptr,IDC_ARROW));
    if(hide!=reportedCursorHidden){CURSORINFO cursor{sizeof(cursor)};GetCursorInfo(&cursor);log(std::string("Fullscreen cursor ")+(hide?"idle / hidden":"moved / visible")+"; Windows flags="+std::to_string(cursor.flags));reportedCursorHidden=hide;}
   }}
   ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();ImVec2 size=io.DisplaySize;float panel=showPanel?std::min(480.f,size.x*.62f):0.f;
   auto toggleFreeze=[&]{if(!displayed)return;if(nrControls.enabled&&(!enhancer||!enhancer->hasResult))return;freezePreview=!freezePreview;if(freezePreview)frozenSubtitle=subtitles.snapshot();else{nrReset=true;nrDiscardPending=true;readyImages.clear();if(pipelineEco){havePrevious=false;}}log(std::string("NR comparison freeze=")+(freezePreview?"1":"0"));};
   if(imageOnly&&freezePreview)toggleFreeze(); // A frozen comparison must not freeze the viewing output.
   if(ImGui::IsKeyPressed(ImGuiKey_F9)){if(diagnostics.active())diagnostics.stop();else if(!diagnostics.busy())startDebug();}
   if(ImGui::IsKeyPressed(ImGuiKey_F10))fullscreenStats=!fullscreenStats;
   if(!imageOnly&&ImGui::IsKeyPressed(ImGuiKey_F8))toggleFreeze();
   if(FRAMELAB_WITH_NR&&nrControls.available(root)&&ImGui::IsKeyPressed(ImGuiKey_F6))nrControls.enabled=!nrControls.enabled;
   if(!imageOnly&&FRAMELAB_WITH_NR&&ImGui::IsKeyPressed(ImGuiKey_F7))nrControls.view=(nrControls.view+1)%3;
   if(nrControls.enabled!=nrWasEnabled){nrReset=true;if(!nrControls.enabled){nrDiscardPending=true;if(!freezePreview&&enhancer&&displayed==enhancer->original.view.Get())displayed=presentedImage.view.Get();}nrWasEnabled=nrControls.enabled;}
   const auto displayEnhancement=nrControls.enabled?(displayedEco?presentedEnhanced.view.Get():(enhancer&&enhancer->hasResult&&displayed==enhancer->original.view.Get()?enhancer->result.view.Get():nullptr)):nullptr;
   const auto visibleCaption=freezePreview?frozenSubtitle:subtitles.snapshot(isLocalVideo&&localSnapshot.paused);
   const double captionTime=(freezePreview||(isLocalVideo&&localSnapshot.paused))?visibleCaption.observedAt:t;
   if(imageOnly){
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(size);ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::Begin("Fullscreen image",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
    if(displayed&&w&&h){float scale=displayControls.scale(w,h,size.x,size.y);ImGui::SetCursorPos({(size.x-w*scale)/2,(size.y-h*scale)/2});auto p=ImGui::GetCursorScreenPos();nrControls.drawPair(displayed,{w*scale,h*scale},displayEnhancement,true);const auto& caption=visibleCaption;drawSubtitleOverlay(p,{w*scale,h*scale},subtitleSettings,caption,(freezePreview||(isLocalVideo&&localSnapshot.paused))?caption.observedAt:t,true);}ImGui::End();ImGui::PopStyleVar();
   }else{
   ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({size.x-panel,size.y});
   ImGui::Begin("Preview",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
   ui::brand();ui::nextButton(tr("Fullscreen image (F11)"));if(ImGui::Button(tr("Fullscreen image (F11)")))SendMessageW(window,WM_KEYDOWN,VK_F11,0);
   ui::nextButton("Freeze comparison (F8)");ImGui::BeginDisabled(!displayed||(nrControls.enabled&&(!enhancer||!enhancer->hasResult)));
   if(ImGui::Button(nrControls.text(freezePreview?"Resume preview (F8)":"Freeze comparison (F8)",freezePreview?"Pokračovat (F8)":"Zmrazit porovnání (F8)")))toggleFreeze();ImGui::EndDisabled();
   if(freezePreview)ImGui::TextColored(ui::accent,"%s",nrControls.text("FROZEN PAIR: adjust strength or press F7. Input continues in the background.","ZMRAZENÝ PÁR: měň sílu nebo stiskni F7. Vstup na pozadí běží dál."));
   if(!activeVideoName.empty())ImGui::TextUnformatted(activeVideoName.c_str());
   if(subtitleAutoPaused)ImGui::TextColored(ui::accent,"%s",nrControls.text("Video paused while the subtitle model loads. Playback resumes automatically.","Video čeká na načtení překladového modelu. Potom se samo rozběhne."));
   ui::eyebrow("01 CAPTURE   /   02 NVIDIA GPU   /   03 DISPLAY");
   if(isDemo&&!isLocalVideo)ImGui::TextColored(ui::orange,"%s",tr("TEST SOURCE"));
   ImGui::Separator();auto area=ImGui::GetContentRegionAvail();float aw=area.x;
   const float comparisonHeight=fg&&comparison.enabled?std::min(355.f,std::max(aw<680?310.f:270.f,area.y*.36f)):58.f;
   float ah=std::max(100.f,area.y-comparisonHeight-38);auto startPos=ImGui::GetCursorPos();
   if(displayed&&w&&h){auto clip=ImGui::GetCursorScreenPos();ImGui::GetWindowDrawList()->PushClipRect(clip,{clip.x+aw,clip.y+ah},true);float scale=displayControls.scale(w,h,aw,ah);float iw=w*scale,ih=h*scale;ImGui::SetCursorPos({startPos.x+(aw-iw)/2,startPos.y+(ah-ih)/2});auto p=ImGui::GetCursorScreenPos();nrControls.drawPair(displayed,{iw,ih},displayEnhancement,false,!subtitleSettings.editOverlay);const auto& caption=visibleCaption;drawSubtitleOverlay(p,{iw,ih},subtitleSettings,caption,(freezePreview||(isLocalVideo&&localSnapshot.paused))?caption.observedAt:t);ImGui::GetWindowDrawList()->PopClipRect();}
   else {ImGui::TextUnformatted(tr("Waiting for capture or test source..."));}
   ImGui::SetCursorPos({startPos.x,startPos.y+ah+4});
   if(fg)comparison.draw(g,root,comparisonHeight);
   else ImGui::TextWrapped("%s",FRAMELAB_WITH_NR?nrControls.text("F8: freeze preview. F7: original / enhanced / split. Sample videos are in Input.","F8: zmrazit náhled. F7: původní / upravený / dělený obraz. Ukázková videa jsou ve Vstupu."):nrControls.text("F8: freeze preview. F11: fullscreen. Sample videos are in Input; local Czech translation is in Subtitles.","F8: zmrazit náhled. F11: celá obrazovka. Ukázky jsou ve Vstupu, lokální český překlad v Titulcích."));
   ImGui::TextDisabled("%s | %u x %u | F1 dashboard | F11 fullscreen",fg&&vfgInitialized?vfg.name():tr("Native preview"),w,h);
   if(live&&!isLocalVideo&&now()-mailbox.lastArrival.load()>2)ImGui::TextColored({1,.65f,.3f,1},"Waiting for capture samples. Check signal / HDCP / other capture apps.");
   ImGui::End();
   if(showPanel){ImGui::SetNextWindowPos({size.x-panel,0});ImGui::SetNextWindowSize({panel,size.y});ImGui::Begin("Telemetry",nullptr,(ImGuiWindowFlags_NoDecoration&~ImGuiWindowFlags_NoScrollbar)|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ui::eyebrow(tr("CONTROL ROOM"));
    ImGui::SameLine(std::max(0.f,ImGui::GetWindowContentRegionMax().x-68));
    if(ui::button(czech?"CZ / EN":"EN / CZ")){czech=!czech;std::ofstream(root/L"language.txt")<<czech;}
    ImGui::TextWrapped("%s",gpu.name);
    if(ImGui::BeginTable("metrics",2)){ImGui::TableNextColumn();metric(tr("INPUT / capture"),inFps,"fps",ui::input);ImGui::TableNextColumn();metric(tr("CONTENT / estimated"),contentFps,"fps",ui::orange);ImGui::TableNextColumn();metric(tr("OUTPUT / content submitted"),outFps,"fps",ui::accent);ImGui::TableNextColumn();metric(tr("GENERATED / submitted"),genFps,"fps",ui::accent);ImGui::EndTable();}
    ui::section(tr("SETTINGS"));
    if(ImGui::BeginTabBar("Control tabs",ImGuiTabBarFlags_FittingPolicyScroll)){
     if(FRAMELAB_WITH_NR&&ImGui::BeginTabItem("AI / NR",nullptr,previewTab==5||(args.find(L"--nr")!=std::wstring::npos&&t-start<.5)?ImGuiTabItemFlags_SetSelected:0)){
      if(nrControls.panel(enhancer.get(),nrFailure,w,h,displayVerification.displayHeight,root)){freezePreview=false;if(pipelineEco||nrControls.economy){resetPipeline();pipelineEco=nrControls.enabled&&nrControls.economy;}else retireEnhancer();}
      if(nrControls.enabled)ImGui::Text(nrControls.text("AI evaluations: %.1f / s","Výpočty AI: %.1f / s"),nrFps);
      if(nrHistory.frames)ImGui::Text(nrControls.text("NR history resets: %llu / %llu frames","Resety historie NR: %llu / %llu snímků"),(unsigned long long)nrHistory.resets,(unsigned long long)nrHistory.frames);
      ImGui::EndTabItem();
     }
     if(ImGui::BeginTabItem(tr("Capture"),nullptr,previewTab==0?ImGuiTabItemFlags_SetSelected:0)){ImGui::Spacing();
    ui::field(isLocalVideo?nrControls.text("Capture device (not active)","Střihovka (není aktivní)"):tr("Source"));if(ImGui::BeginCombo("##capture-source",devices.empty()?tr("No capture devices"):devices[selected].friendlyName.c_str())){for(int i=0;i<int(devices.size());++i)if(ImGui::Selectable(devices[i].friendlyName.c_str(),selected==i)){if(selected!=i){stopSource();selected=i;resolution=0;refreshModes();}}ImGui::EndCombo();}
    ImGui::TextWrapped(tr("Delivered frames: %u x %u | %.1f FPS"),w,h,inFps);ImGui::TextDisabled(tr("HDMI signal: not reported by this API"));
    auto modeName=[&](int i){if(i==0)return std::string(tr("Automatic (device)"));auto m=resolutions.at(i-1);return std::to_string(m.width)+" x "+std::to_string(m.height)+"  ["+aspectLabel(m.width,m.height)+"]"+(m.width==3840&&m.height==2160?" 4K":"");};
    ui::field(tr("Resolution"));if(ImGui::BeginCombo("##capture-resolution",modeName(resolution).c_str())){for(int i=0;i<=int(resolutions.size());++i)if(ImGui::Selectable(modeName(i).c_str(),resolution==i)){resolution=i;requestedFps=0;}ImGui::EndCombo();}
    auto fpsLabel=[&](double f){if(f==0)return std::string(tr("Automatic (device)"));char text[48];snprintf(text,sizeof(text),"%.3f FPS",f);return std::string(text);};
    ImGui::BeginDisabled(resolution==0);
    ui::field(tr("Capture FPS"));if(ImGui::BeginCombo("##capture-fps",fpsLabel(requestedFps).c_str())){if(ImGui::Selectable(tr("Automatic (device)"),requestedFps==0))requestedFps=0;
     if(resolution>0){auto m=resolutions[resolution-1];for(double f:captureRates(modes,m.width,m.height))if(ImGui::Selectable(fpsLabel(f).c_str(),std::abs(requestedFps-f)<.005))requestedFps=f;}
     ImGui::EndCombo();}ImGui::EndDisabled();
    if(ImGui::CollapsingHeader(tr("Capture details"))){
    if(live&&!isDemo&&capture.negotiatedFrameRate()>0)ImGui::Text(tr("Driver negotiated: %.3f FPS"),capture.negotiatedFrameRate());
    if(w&&h)ImGui::Text(tr("Image aspect: %s (preserved)"),aspectLabel(w,h).c_str());
    ImGui::TextWrapped(tr("Capture FPS is not an output limit. Frame generation follows changing input frames."));
    ImGui::TextWrapped(tr("Apply with Start capture. Automatic leaves FPS to the driver."));
    if(ImGui::IsItemHovered())ImGui::SetTooltip(tr("Select a resolution to request FPS. Rates come from driver defaults and ranges; the driver may round them. Automatic is not HDMI signal detection."));
    }
    if(ui::button(tr("Start capture"),true))startCapture();ui::nextButton(tr("Test 30 FPS"));if(ImGui::Button(tr("Test 30 FPS"))){fg=true;startDemo();}ui::nextButton(tr("Stop"));if(ImGui::Button(tr("Stop"))){stopSource();status="Stopped";}
    ui::section(nrControls.text("YOUR VIDEO / MOVIE","VLASTNÍ VIDEO / FILM"));
    if(ui::button(nrControls.text("Open video...","Otevřít video..."),true))chooseLocalVideo();
    ImGui::TextWrapped("%s",nrControls.text("Local playback with audio, FG, enhancer and subtitles. The complete package includes codecs for MKV and other common formats. Protected streaming downloads are unsupported.","Místní přehrávání se zvukem, FG, enhancerem a titulky. Kompletní balíček obsahuje kodeky pro MKV a další běžné formáty. Chráněné filmy ze streamovacích služeb nepodporuje."));
    if(isLocalVideo){
     ImGui::TextWrapped("%s",activeVideoName.c_str());
     if(localSnapshot.opening)ImGui::TextUnformatted(nrControls.text("Opening...","Otevírání..."));
     else if(!localSnapshot.error.empty())ImGui::TextWrapped("%s",localSnapshot.error.c_str());
     else{
      ImGui::Text(nrControls.text("Native: %u x %u | %.3f FPS","Zdroj: %u x %u | %.3f FPS"),localSnapshot.width,localSnapshot.height,localSnapshot.fps);
      ImGui::TextDisabled("%s",localSnapshot.backend.c_str());
      auto trackName=[&](const std::vector<LocalVideoPlayer::Track>& tracks,int id){for(const auto& track:tracks)if(track.id==id)return track.id==-1?std::string(nrControls.text("Off","Vypnuto")):track.name;return std::string(nrControls.text("Off / unavailable","Vypnuto / nedostupné"));};
      if(!localSnapshot.audioTracks.empty()){
       ui::field(nrControls.text("Audio track","Zvuková stopa"));if(ImGui::BeginCombo("##file-audio-track",trackName(localSnapshot.audioTracks,localSnapshot.audioTrack).c_str())){for(auto& track:localSnapshot.audioTracks)if(ImGui::Selectable((track.id==-1?nrControls.text("Off","Vypnuto"):track.name.c_str()),track.id==localSnapshot.audioTrack))localVideo.selectAudioTrack(track.id);ImGui::EndCombo();}
      }
      if(!localSnapshot.subtitleTracks.empty()){
       ui::field(nrControls.text("Subtitles in this file","Titulky v tomto souboru"));if(ImGui::BeginCombo("##file-subtitle-track",trackName(localSnapshot.subtitleTracks,localSnapshot.subtitleTrack).c_str())){for(auto& track:localSnapshot.subtitleTracks)if(ImGui::Selectable((track.id==-1?nrControls.text("Off","Vypnuto"):track.name.c_str()),track.id==localSnapshot.subtitleTrack))localVideo.selectSubtitleTrack(track.id);ImGui::EndCombo();}
       ImGui::TextWrapped("%s",nrControls.text("Selected file subtitles are drawn into the source picture. AI translation is controlled separately in Subtitles.","Vybrané titulky ze souboru se vykreslí do vstupního obrazu. AI překlad se nastavuje zvlášť v Titulcích."));
      }
      if(!localSnapshot.trackStatus.empty())ImGui::TextWrapped("%s",localSnapshot.trackStatus.c_str());
      auto clockText=[](double value){int sec=int(std::max(0.,value));char buffer[32];snprintf(buffer,sizeof(buffer),"%02d:%02d:%02d",sec/3600,(sec/60)%60,sec%60);return std::string(buffer);};
      ImGui::Text("%s / %s",clockText(localSnapshot.position).c_str(),clockText(localSnapshot.duration).c_str());
      if(ImGui::Button(nrControls.text(localSnapshot.ended?"Play again":localSnapshot.paused?"Play":"Pause",localSnapshot.ended?"Přehrát znovu":localSnapshot.paused?"Přehrát":"Pozastavit"))){
       if(subtitleAutoPaused){subtitleAutoPaused=false;subtitleWarmupOptout=true;}
       freezePreview=false;if(localSnapshot.ended){localVideo.seek(0);localVideo.pause(false);}else localVideo.pause(!localSnapshot.paused);
      }
      ImGui::SameLine();bool loop=localSnapshot.loop;if(ImGui::Checkbox(nrControls.text("Loop","Opakovat"),&loop))localVideo.loop(loop);
      if(localSnapshot.duration>0){
       if(!editingSeek)seekPosition=float(localSnapshot.position);
       ImGui::SliderFloat("##file-position",&seekPosition,0,float(localSnapshot.duration),"%.1f s");editingSeek=ImGui::IsItemActive();
       if(ImGui::IsItemDeactivatedAfterEdit()){freezePreview=false;localVideo.seek(seekPosition);editingSeek=false;}
      }
      ImGui::TextWrapped("%s",nrControls.text("Sound output is in Audio. Decoded video and sound share one media clock; FG/NR and the display add video delay. This is not an A/V latency measurement.","Zvukový výstup vybereš ve Zvuku. Dekódovaný obraz a zvuk mají společné hodiny; FG/NR a monitor přidávají zpoždění obrazu. Nejde o měření A/V latence."));
     }
    }
    ui::section(nrControls.text("INCLUDED SAMPLE VIDEOS","PŘIBALENÁ UKÁZKOVÁ VIDEA"));
    if(ImGui::Button("GTA 720p / 30 FPS"))startSample(root/L"samples"/L"gta.mp4");ImGui::SameLine();if(ImGui::Button("CS:GO 1080p"))startSample(root/L"samples"/L"action.mp4");
    ImGui::TextDisabled("%s",nrControls.text("Both clips play locally, without downloading.","Oba klipy se přehrávají lokálně, bez stahování."));
    ImGui::BeginDisabled(!std::filesystem::is_regular_file(root/L"samples"/L"ocr-demo.mp4"));
    if(ImGui::Button(nrControls.text("Try English > Czech subtitles","Vyzkoušet titulky EN > CZ"))){startSample(root/L"samples"/L"ocr-demo.mp4");subtitleStartupPending=true;if(subtitleSettings.ocrEngine==1&&ocrSetup.snapshot().available==0)status=nrControls.text("Select bundled RapidOCR in Subtitles, or install Windows English OCR.","V Titulcích vyber přibalené RapidOCR, nebo doplň anglické OCR ve Windows.");}
    ImGui::EndDisabled();
    ui::field(tr("Next replay FPS"));int strideChoice=sampleStride-1;if(ImGui::Combo("##replay-fps",&strideChoice,"60\00030\00020\00015\000\000")){sampleStride=strideChoice+1;}
     ImGui::EndTabItem();}
     if(ImGui::BeginTabItem(tr("Frame gen"),nullptr,previewTab==1?ImGuiTabItemFlags_SetSelected:0)){ImGui::Spacing();
    int backend=vfg.useOptical?1:0;ui::field(tr("Backend"));bool change=ImGui::Combo("##backend",&backend,"NVIDIA VFG (RTX 40/50)\000D3D11 Optical Flow (experimental)\000\000");if(change){resetPipeline();vfg.useOptical=backend==1;}
    change|=ImGui::Checkbox(tr("Frame generation"),&fg);ui::nextButton(tr("Medium"));ImGui::SetNextItemWidth(125);ImGui::BeginDisabled(vfg.useOptical);const char* qualityLabels[]={tr("Low"),tr("Medium"),tr("High")};change|=ImGui::Combo("##quality",&quality,qualityLabels,3);ImGui::EndDisabled();ImGui::TextColored(ui::muted,"%s",tr("Multiplier"));float multiplierWidth=(ImGui::GetContentRegionAvail().x-2*ImGui::GetStyle().ItemSpacing.x)/3;
    for(int factor=2;factor<=4;++factor){if(factor>2)ImGui::SameLine();char label[8];snprintf(label,sizeof(label),"%dx",factor);if(ui::button(label,multiplier==factor,{multiplierWidth,38})){change|=multiplier!=factor;multiplier=factor;}}
    if(multiplier>2)ImGui::TextWrapped(tr("Experimental: 3x/4x can increase artifacts and latency."));ImGui::BeginDisabled(vfg.useOptical);change|=ImGui::Checkbox(tr("Automatic scene-cut detection"),&automaticCuts);ImGui::EndDisabled();if(vfg.useOptical)ImGui::TextWrapped(tr("Experimental optical flow: no AI model or automatic cut detection."));if(change){resetPipeline();status=fg?"VFG enabled. Waiting for a new source frame.":"Native preview; VFG disabled.";}
    if(multiplier>2&&!vfg.useOptical){
     ImGui::TextWrapped(tr("High is not always better. Medium favors detail; Low favors speed."));
     bool preset=false;
     if(ImGui::Button(tr("Balanced 3x"))){multiplier=3;quality=1;preset=true;}ImGui::SameLine();
     if(ImGui::Button(tr("Fast 4x"))){multiplier=4;quality=0;preset=true;}
     if(preset){fg=true;automaticCuts=true;resetPipeline();status="Preset applied. Waiting for a new source frame.";}
    }
    ImGui::Text(tr("Presentation queue: %u | discarded: %llu"),unsigned(readyImages.size()),(unsigned long long)presentationDrops);
    bool dd=mailbox.dedup; if(ImGui::Checkbox(tr("Remove repeated input frames"),&dd))mailbox.dedup=dd;
    if(ImGui::IsItemHovered())ImGui::SetTooltip(tr("Exact duplicate detection by default. Optional noise tolerance uses sparse RGB samples. Static scenes can read 0 FPS; this is not engine telemetry."));
    ImGui::Checkbox(tr("VSync (tear-free)"),&vsync);
    ImGui::TextWrapped("%s",pipelineEco?nrControls.text("Eco: input > enhancer > FG > captions > display. RAW/FG reference also runs for exact comparisons and strength blending.","Úsporně: vstup > enhancer > FG > titulky > monitor. Původní FG běží také kvůli přesnému porovnání a síle efektu."):nrControls.text("Pipeline: input > selected FG backend > enhancer (if enabled) > Czech subtitles > display. Output FPS is limited by the slowest stage.","Pořadí: vstup > vybrané FG > enhancer (je-li zapnutý) > české titulky > monitor. Výstupní FPS omezuje nejpomalejší část."));
     ImGui::EndTabItem();}
     if(ImGui::BeginTabItem(nrControls.text("Subtitles","Titulky"),nullptr,previewTab==6?ImGuiTabItemFlags_SetSelected:0)){
      if(subtitlePanel(subtitleSettings,subtitles.snapshot(isLocalVideo&&localSnapshot.paused),subtitleModels,gpu.freeMemoryMB,ocrSetup,window,rapidOcrReady,&subtitleStylePicker.requested,&subtitleProfiles,&subtitleInspector.open))subtitles.configure(subtitleSettings);
      ImGui::EndTabItem();
     }
     if(ImGui::BeginTabItem(nrControls.text("Display","Zobrazení"),nullptr,previewTab==7?ImGuiTabItemFlags_SetSelected:0)){auto previous=displayControls.fit;displayControls.panel(window,fullscreen);if(previous!=displayControls.fit)displayControls.save(root/L"display-settings.txt");ImGui::EndTabItem();}
     if(ImGui::BeginTabItem(tr("Audio"),nullptr,previewTab==2?ImGuiTabItemFlags_SetSelected:0)){ImGui::Spacing();
    if(isLocalVideo){
     ui::field(nrControls.text("Video file audio output","Zvukový výstup videa"));
     if(ImGui::BeginCombo("##file-audio-output",fileOutputs[fileAudioOutput].name.c_str())){
      for(int i=0;i<int(fileOutputs.size());++i)if(ImGui::Selectable(fileOutputs[i].name.c_str(),fileAudioOutput==i)&&i!=fileAudioOutput){
       fileAudioOutput=i;std::wofstream(root/L"file-audio-settings.txt")<<std::quoted(fileOutputs[i].id);
       startLocalVideo(localVideoPath,localSnapshot.position,localSnapshot.paused);localVideo.loop(localSnapshot.loop);
       if(!localSnapshot.audioTracks.empty())localVideo.selectAudioTrack(localSnapshot.audioTrack);
       if(!localSnapshot.subtitleTracks.empty())localVideo.selectSubtitleTrack(localSnapshot.subtitleTrack);
      }ImGui::EndCombo();
     }
     ImGui::TextWrapped("%s",localSnapshot.audio?nrControls.text("Video audio uses the selected Windows endpoint.","Zvuk videa používá vybrané zařízení Windows."):nrControls.text("No audio track / opening video.","Bez zvukové stopy / otevírání videa."));
    }else{
    ui::field(tr("Output"));if(ImGui::BeginCombo("##audio-output",outputs[audioOutput].friendlyName.c_str())){for(int i=0;i<int(outputs.size());++i)if(ImGui::Selectable(outputs[i].friendlyName.c_str(),audioOutput==i)){audioOutput=i;saveAudio();if(live&&!isDemo)startCapture();}ImGui::EndCombo();}
    }
    ui::field(tr("Volume"));bool volumeChanged=ImGui::SliderInt("##audio-volume",&audioVolume,0,100,"%d%%");volumeChanged|=ImGui::Checkbox(tr("Mute"),&audioMuted);if(volumeChanged){capture.setAudioVolume(audioVolume,audioMuted);localVideo.volume(audioVolume,audioMuted);saveAudio();}
    if(live&&!isDemo){ImGui::PushStyleColor(ImGuiCol_Text,capture.audioConnected()?ui::accent:ui::orange);ImGui::TextWrapped("%s",capture.audioStatus().c_str());ImGui::PopStyleColor();}
     ImGui::EndTabItem();}
     if(ImGui::BeginTabItem(tr("Recording"),nullptr,previewTab==3?ImGuiTabItemFlags_SetSelected:0)){ImGui::Spacing();
    ImGui::BeginDisabled(analysis.active||analysis.writing||analysisExporter);if(ui::button(tr("Record 15 s analysis"),true))startAnalysis();ImGui::EndDisabled();ui::nextButton(tr("Clips folder"));if(ImGui::Button(tr("Clips folder"))){auto folder=analysis.folder.empty()?(analysis.destination.empty()?root/L"recordings":analysis.destination):analysis.folder;std::filesystem::create_directories(folder);ShellExecuteW(window,L"open",folder.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}
    ImGui::TextWrapped("%s",nrControls.text("Includes RAW / FG, matched before / after enhancer, final video + Czech captions and SRT. Records the enabled effects; silent clips. Compressed originals stay on disk.","Uloží RAW / FG, stejný obraz před / po enhanceru, finální video s českými titulky a SRT. Zachytí zapnuté efekty; klipy jsou bez zvuku. Komprimované originály zůstanou na disku."));
    ImGui::TextWrapped("%s",analysis.message().c_str());
    recordingLocation.panel(window,root,analysis,analysis.active||analysis.writing||analysisExporter,czech);
    if(!analysis.folder.empty()){auto clipState=!std::filesystem::exists(analysis.folder)?"Recording folder no longer exists":std::filesystem::exists(analysis.folder/L"clips.json")?(analysis.dropped>0||analysis.stoppedTime-analysis.startTime<14.99?nrControls.text("Partial clips ready; inspect result.json","Částečné klipy hotové; viz result.json"):tr("Clips ready")):std::filesystem::exists(analysis.folder/L"export-error.txt")?"Clip export failed; see export-error.txt":analysis.active||analysis.writing?"Collecting source frames":"Creating clips...";ImGui::TextWrapped("%s | RAW %llu / VFG %llu",tr(clipState),(unsigned long long)analysis.rawCount.load(),(unsigned long long)analysis.genCount.load());}
     ImGui::EndTabItem();}
     if(ImGui::BeginTabItem(tr("Stats"),nullptr,previewTab==4?ImGuiTabItemFlags_SetSelected:0)){ImGui::Spacing();
    ui::section(tr("DISPLAY / TIMING"));
    ImGui::TextWrapped(tr("Display mode: %u x %u | %u Hz"),displayVerification.displayWidth,displayVerification.displayHeight,displayVerification.displayHz);
    if(ImGui::BeginTable("verified-display",2)){ImGui::TableNextColumn();metric(tr("WINDOWS / confirmed IDs"),dxgiRate,"fps",ui::accent);ImGui::TableNextColumn();metric(tr("GENERATED / confirmed"),confirmedGenFps,"fps",ui::accent);ImGui::EndTable();}
    ImGui::TextWrapped(tr("Observed display confirmations: lower bound, not all frames may be sampled. Repeated IDs count once; SDK pixel copies can still have new IDs."));
    if(dxgiRate<0)ImGui::TextWrapped(tr("Display verification unavailable. Submitted FPS alone does not prove displayed FPS."));
    if(displayVerification.haveImage)ImGui::Text(tr("Last OS-confirmed image: %s #%llu | present #%u"),displayVerification.lastGenerated?"G":"R",(unsigned long long)displayVerification.lastImage,displayVerification.lastConfirmed);
    chart("FPS",inputHistory,&uniqueHistory,&outputHistory,std::max(70.f,std::max(inFps,outFps)*1.2f),70);ImGui::TextWrapped(tr("Input / content / output | last 180 seconds"));
    if(ImGui::BeginTable("latency",2)){ImGui::TableNextColumn();metric(tr("CALLBACK > PRESENT / mean"),latency,"ms",ui::input);ImGui::TableNextColumn();metric(tr("VFG + GPU output copy"),gpuMs,"ms",ui::orange);ImGui::EndTable();}
    chart(tr("Latency"),latencyHistory,&gpuTimeHistory,nullptr,std::max(50.f,std::max(latency,gpuMs)*1.2f),50);
    ImGui::TextWrapped(tr("App latency P95: %.1f ms | excludes card + scanout"),p95);
    chart("GPU",utilHistory,nullptr,nullptr,100,35);ImGui::Text(tr("GPU total %.0f%% | VRAM %.0f MB | %.0f C | %.0f W"),gpu.utilization,gpu.memoryMB,gpu.temp,gpu.power);
    ImGui::TextWrapped(tr("Present calls %.1f/s | observed content %.1f/s"),presentFps,dxgiRate);ImGui::TextWrapped(tr("-1 = unavailable. Physical monitor FPS not measured."));
    ImGui::TextWrapped(tr("Mailbox drops %llu | invalid %llu"),(unsigned long long)mailbox.dropped.load(),(unsigned long long)mailbox.invalid.load());
    if(ImGui::CollapsingHeader(tr("Measurement details"))){float threshold=mailbox.threshold;if(ImGui::SliderFloat(tr("Dedup sensitivity"),&threshold,0,5,"%.2f"))mailbox.threshold=threshold;ImGui::TextWrapped(tr("Latency starts when the capture callback receives pixels and ends after Present returns. Generated frames use the newer source timestamp. GPU utilization is the whole GPU, including other apps. CSV is saved every second under app/logs. HDMI audio uses the selected DirectSound output. Audio latency and A/V offset are not measured."));}
     ImGui::EndTabItem();}
     if(ImGui::BeginTabItem(czech?"Debug a měření":"Debug & measurement",nullptr,previewTab==8?ImGuiTabItemFlags_SetSelected:0)){
      auto label=[&](const char* en,const char* cs){return czech?cs:en;};
      ui::title(label("DIAGNOSTICS","DIAGNOSTIKA"));
      if(ImGui::Checkbox(label("Fullscreen FPS + latency (F10)","FPS + odezva ve fullscreenu (F10)"),&fullscreenStats))saveLiveSession();
      ImGui::SameLine();ImGui::TextDisabled("(?)");
      if(ImGui::IsItemHovered()){ImGui::BeginTooltip();ImGui::PushTextWrapPos(ImGui::GetFontSize()*25.f);
       ImGui::TextUnformatted(label("Small top-right overlay; no debug recording needed. OS = OS-confirmed content FPS, OUT = submitted FPS when confirmation is unavailable. APP = capture callback to Present, including frame gen, enhancer and queues. 95% of sampled frames have latency at or below P95. Capture hardware, controller, audio and monitor latency are not measured.","Malý ukazatel vpravo nahoře, bez zapínání záznamu. OS = FPS obsahu potvrzené systémem, OUT = odeslané FPS při nedostupném potvrzení. APP = příjem snímku až po Present, včetně frame genu, enhanceru a front. 95 % měřených snímků mělo odezvu nejvýše P95. Latenci karty, herního ovladače, zvuku a monitoru neměříme."));
       ImGui::PopTextWrapPos();ImGui::EndTooltip();}
      ImGui::TextDisabled("%s",label("APP latency excludes capture hardware + monitor.","APP nezahrnuje zpoždění střihovky a monitoru."));
      ImGui::Separator();
      ImGui::TextWrapped("%s",label("Local performance recording: PC, GPU, frame pacing, enhancer, OCR and translation. No images or audio are captured.","Místní záznam výkonu: PC, GPU, časování snímků, enhancer, OCR a překlad. Neukládá obraz ani zvuk."));
      ImGui::BeginDisabled(diagnostics.busy());
      const char* durations[]={label("Until stopped","Do vypnutí"),"30 s","60 s","3 min","15 min"};const int seconds[]={0,30,60,180,900};int selectedDuration=debugDuration==30?1:debugDuration==60?2:debugDuration==180?3:debugDuration==900?4:0;
      if(ImGui::Combo(label("Duration","Délka"),&selectedDuration,durations,5))debugDuration=seconds[selectedDuration];
      ImGui::Checkbox(label("Frame-by-frame timing","Časování každého snímku"),&debugFrames);
      ImGui::Checkbox(label("Include OCR and translated text","Zahrnout text OCR a překladu"),&debugText);
      ImGui::TextWrapped("%s",label("Recording keeps writing to the folder until you press Stop / F9 or close the app. OCR and translated text can contain private information. Nothing is uploaded automatically.","Záznam bude průběžně ukládat do složky, dokud ho nevypneš tlačítkem Stop / F9 nebo nezavřeš aplikaci. OCR a překlady mohou obsahovat soukromé informace. Nic se automaticky neodesílá."));
      if(ImGui::Button(label("Start debug recording (F9)","Spustit diagnostický záznam (F9)")))startDebug();ImGui::EndDisabled();
      if(diagnostics.active()){ImGui::SameLine();if(ImGui::Button(label("Stop","Zastavit")))diagnostics.stop();}
      if(ImGui::Button(label("Open debug folder","Otevřít složku diagnostiky"))){auto folder=diagnostics.folder();if(folder.empty())folder=root/L"logs";ShellExecuteW(nullptr,L"open",folder.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}
      ImGui::Text("%s | %.1f s | %.2f MiB | dropped %llu",diagnostics.status().c_str(),diagnostics.elapsed(),diagnostics.bytes()/1048576.,(unsigned long long)diagnostics.dropped());
      ImGui::TextWrapped("%s",label("Background writing; files rotate at 64 MiB. Stops if free disk space falls below 256 MiB. UTC timestamps and session IDs link settings, metrics, frame timeline, OCR/translations and summary. Long-run percentiles are estimated.","Ukládání na pozadí, soubory se dělí po 64 MiB. Při méně než 256 MiB volného místa se záznam zastaví. Časy UTC a ID relace propojují nastavení, výkon, snímky, OCR/překlady a souhrn. Percentily dlouhých záznamů jsou odhad."));
      {auto folder=diagnostics.folder().empty()?root/L"logs":diagnostics.folder();auto u8=folder.u8string();ImGui::TextWrapped("%s",std::string(u8.begin(),u8.end()).c_str());}
      ui::section(label("LIVE STAGES","ŽIVÉ ČASY"));
      ImGui::Text("CPU %.1f%% | app %.1f%% | RAM %.0f / %.0f MiB",systemTelemetry.cpu,systemTelemetry.appCpu,systemTelemetry.ramUsedMB,systemTelemetry.ramTotalMB);
      ImGui::Text("GPU %.0f%% | %.0f W | %.0f C | VRAM %.0f MiB",gpu.utilization,gpu.power,gpu.temp,gpu.memoryMB);
      if(enhancer){auto nt=enhancer->timings();ImGui::Text("NR worker %.2f ms | submit > ready %.2f ms",enhancer->processingMs(),nt.submitToReadyMs);ImGui::Text("NR GPU: prep %.2f / AI %.2f / finish %.2f ms",nt.gpu.prepareGpuMs,nt.gpu.inferenceGpuMs,nt.gpu.finishGpuMs);ImGui::Text("NR ready > take %.2f ms | resets %llu",nt.readyToTakeMs,(unsigned long long)nrHistory.resets);}
      auto sub=subtitles.snapshot(isLocalVideo&&localSnapshot.paused);auto& sp=sub.performance;
      ImGui::Text("OCR %.1f ms | queue %.1f ms | busy %d",sub.ocrMilliseconds,sp.ocrQueueMs,sp.ocrBusy);
      ImGui::Text("MT %.1f ms | queue %.1f ms | busy %d",sp.translationMs,sp.translationQueueMs,sp.translationBusy);
      ImGui::Text("Caption delay %.1f ms | pending %d",sp.displayLatencyMs,sub.pending);
      ImGui::Text("OCR %llu | MT %llu | cache %llu | errors %llu",(unsigned long long)sp.ocrCompleted,(unsigned long long)sp.translations,(unsigned long long)sp.cacheHits,(unsigned long long)(sp.ocrErrors+sp.translationErrors));
      ImGui::TextWrapped("%s",label("Stage times show the last completed job, not total GPU usage. CPU app excludes helper processes. App latency excludes capture hardware, audio and monitor scanout; physical display FPS is not measured.","Časy etap patří poslední dokončené úloze, ne celkovému vytížení GPU. CPU aplikace nezahrnuje pomocné procesy. Latence aplikace nezahrnuje kartu, zvuk ani monitor; fyzické FPS monitoru neměříme."));
      ImGui::EndTabItem();
     }
     ImGui::EndTabBar();
    }
    ImGui::Spacing();ImGui::PushStyleColor(ImGuiCol_Text,ui::muted);ImGui::TextWrapped("%s",tr(status.c_str()));ImGui::PopStyleColor();
    if(analysis.active||analysis.writing){ImGui::TextColored(ui::orange,"%s",tr("Collecting source frames"));}
    ImGui::End();
   }
   }
   if(subtitleStylePicker.draw(g,subtitles,subtitleSettings,packet.data,packet.w,packet.h,imageOnly,root))subtitles.configure(subtitleSettings);
   if(subtitleInspector.draw(g,root,subtitles,subtitleSettings,packet.h,imageOnly,isLocalVideo&&localSnapshot.paused))subtitles.configure(subtitleSettings);
   if(!(savedSubtitleSettings==subtitleSettings)&&!ImGui::IsMouseDown(ImGuiMouseButton_Left)){subtitleSettings.save(root/L"subtitle-settings.txt");savedSubtitleSettings=subtitleSettings;}
   if(!(savedNrControls==nrControls)&&!ImGui::IsMouseDown(ImGuiMouseButton_Left)){nrControls.save(root/L"nr-settings.txt");savedNrControls=nrControls;}
   if(imageOnly&&fullscreenStats){
    const auto text=outputStatsText(live&&displayed,displayVerification.available,dxgiRate,outFps,latency,p95);
    auto font=ui::mono?ui::mono:ImGui::GetFont();const float fontSize=14.f;
    const auto bounds=font->CalcTextSizeA(fontSize,10000.f,0.f,text.c_str());
    const ImVec2 p{std::max(8.f,size.x-bounds.x-22.f),12.f};auto draw=ImGui::GetForegroundDrawList();
    draw->AddRectFilled({p.x-8,p.y-5},{p.x+bounds.x+8,p.y+bounds.y+5},IM_COL32(12,16,13,165),4.f);
    draw->AddText(font,fontSize,p,IM_COL32(235,240,230,225),text.c_str());
   }
   ImGui::Render();
   std::string recordedOutput;
   if(analysis.active&&displayed&&w&&h){try{
    const bool matched=displayEnhancement!=nullptr;
    const bool captionVisible=subtitleSettings.enabled&&!visibleCaption.translated.empty()&&captionTime<(visibleCaption.expiresAt>0?visibleCaption.expiresAt:visibleCaption.observedAt+4);
    const auto translated=captionVisible?visibleCaption.translated:std::string{};
    const auto source=captionVisible?(visibleCaption.displaySource.empty()?visibleCaption.source:visibleCaption.displaySource):std::string{};
    std::ostringstream signature;signature<<displayedGenerated<<':'<<displayedId<<':'<<matched<<':'<<nrControls.strength<<':'<<translated<<':'<<source<<':'<<visibleCaption.displayedAt
     <<':'<<subtitleSettings.fontPercent<<':'<<subtitleSettings.position<<':'<<subtitleSettings.horizontal<<':'<<subtitleSettings.placement<<':'<<subtitleSettings.background<<':'<<subtitleSettings.showOriginal<<':'<<subtitleSettings.coverSource;
    if(signature.str()!=analysis.lastOutputSignature){
     analysis.lastOutputSignature=signature.str();const auto id=++analysis.outputSerial;const double sampleTime=now();
     char bk=displayedGenerated?'G':'R';uint64_t bi=displayedId;
     if(!analysis.has(bk,bi)){ComPtr<ID3D11Resource> resource;ComPtr<ID3D11Texture2D> texture;displayed->GetResource(&resource);check(resource.As(&texture),"Recording source texture");bk='B';bi=id;analysis.image(g,texture.Get(),bk,w,h,bi,displayedA,displayedB,displayedPosition,sampleTime);}
     char ek=bk,fk=bk;uint64_t ei=bi,fi=bi;
     auto enhanced=displayEnhancement;
     if(matched&&nrControls.strength>0){recordingOutput.render(g,w,h,displayed,enhanced,nrControls.strength,nullptr,visibleCaption,captionTime);ek='E';ei=id;analysis.image(g,recordingOutput.image.texture.Get(),ek,w,h,id,displayedA,displayedB,displayedPosition,sampleTime);}
     fk=ek;fi=ei;
     if(captionVisible){recordingOutput.render(g,w,h,displayed,enhanced,nrControls.strength,&subtitleSettings,visibleCaption,captionTime);fk='F';fi=id;analysis.image(g,recordingOutput.image.texture.Get(),fk,w,h,id,displayedA,displayedB,displayedPosition,sampleTime);}
     auto ref=[](char k,uint64_t i){return "["+Diagnostics::quote(std::string(1,k))+","+std::to_string(i)+"]";};
     recordedOutput="\"output_id\":"+std::to_string(id)+",\"source_id\":"+std::to_string(displayedId)+",\"generated\":"+(displayedGenerated?"true":"false")
      +",\"source_A\":"+std::to_string(displayedA)+",\"source_B\":"+std::to_string(displayedB)+",\"phase\":"+Diagnostics::number(displayedPosition)
      +",\"before\":"+ref(bk,bi)+",\"after\":"+ref(ek,ei)+",\"final\":"+ref(fk,fi)+",\"nr_applied\":"+(matched?"true":"false")
      +",\"nr_strength\":"+Diagnostics::number(nrControls.strength)+",\"subtitle\":"+Diagnostics::quote(translated)+",\"subtitle_source\":"+Diagnostics::quote(source);
    }
   }catch(const std::exception& e){analysis.finish(std::string("Output recording failed: ")+e.what());log(e.what());}}
   g.begin();ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
   if(uiPreview&&t-start>=previewDelay){
    if(!previewPath.empty())saveUiPreview(g,previewPath);auto caption=subtitles.snapshot();
    std::ofstream(root/L"logs"/L"subtitle-preview.txt")<<caption.status<<'\n'<<caption.source<<'\n'<<caption.translated<<'\n';
    std::ofstream(root/L"logs"/L"pipeline-preview.txt")
     <<"local_video="<<isLocalVideo<<"\nlocal_video_audio="<<localSnapshot.audio<<"\nlocal_video_position="<<localSnapshot.position<<"\nlocal_video_error="<<localSnapshot.error<<"\n"<<"nr_passes="<<nrControls.passes<<"\n"<<"vfg_enabled="<<fg<<"\nvfg_initialized="<<vfgInitialized<<"\nnr_enabled="<<nrControls.enabled
     <<"\nnr_economy="<<nrControls.economy<<"\nnr_evaluations_fps="<<nrFps<<"\nnr_has_result="<<bool(enhancer&&enhancer->hasResult)<<"\nnr_processing_ms="<<(enhancer?enhancer->processingMs():0)
     <<"\nsubtitle_active_model="<<caption.activeModel<<"\nsubtitle_loading="<<caption.loading
     <<"\ninput_width="<<w<<"\ninput_height="<<h<<"\nclean_output="<<imageOnly<<'\n';
    running=false;
   }
   auto presentStarted=now();auto hr=g.present(inSizeMove?false:vsync);presentationPermit=false;
   lastDraw=now();presentCpuMs+=(lastDraw-presentStarted)*1000;if(hr==DXGI_STATUS_OCCLUDED)++occludedPresents;
   if(hr==S_OK&&!recordedOutput.empty())analysis.output(lastDraw,recordedOutput);
   if(hr==S_OK&&diagnostics.frames()){const double contentInterval=newContent&&lastContentAt>0?(lastDraw-lastContentAt)*1000:0;diagnostics.frame({{"new_content",double(newContent)},{"generated",double(displayedGenerated)},{"frame_id",double(displayedId)},{"source_a",double(displayedA)},{"source_b",double(displayedB)},{"phase",displayedPosition},{"content_interval_ms",contentInterval},{"present_cpu_ms",(lastDraw-presentStarted)*1000},{"callback_to_present_ms",displayed?(lastDraw-displaySourceTime)*1000:-1},{"queue",double(readyImages.size())}});if(newContent)lastContentAt=lastDraw;}
   if(hr==S_OK){if(newContent){pacer.presented(now());displayedAt=now();if(pacingLog)pacingLog<<std::fixed<<std::setprecision(6)<<displayedAt-start<<','<<newGenerated<<','<<displayedId<<','<<readyImages.size()<<','<<displayedA<<','<<displayedB<<','<<displayedPosition<<','<<(displayedAt-displaySourceTime)*1000<<'\n';}if(displayed)analysis.presented(displayedGenerated,displayedId,displayedA,displayedB,displayedPosition);++totalPresents;if(newContent){++totalContent;if(newGenerated)++totalGenerated;latencySamples.push_back(float((now()-displaySourceTime)*1000));}}
   else if(FAILED(hr))throw std::runtime_error("Present failed "+std::to_string(unsigned(hr)));
   if(hr==S_OK&&displayed){displayVerification.submitted(g.swap.Get(),displayedGenerated,displayedId);if(auto c=displayVerification.observe(g.swap.Get()))analysis.confirmed(c->submission.generated,c->submission.image,c->submission.present,c->refresh,c->syncRefresh,c->syncQpc);}
   newContent=false;newGenerated=false;
   if(!vsync&&!inSizeMove)gpuPollWait.wait();
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
  saveLiveSession();stopSource();if(analysisExporter)CloseHandle(analysisExporter);log("Finished: content="+std::to_string(totalContent)+" generated="+std::to_string(totalGenerated));ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();graphics=nullptr;DestroyWindow(window);
 }catch(const std::exception& e){log(std::string("FATAL ")+e.what());if(std::wstring(cmd).find(L"--replay-recording")==std::wstring::npos&&std::wstring(cmd).find(L"--ui-preview")==std::wstring::npos)MessageBoxA(nullptr,e.what(),"FrameLab",MB_ICONERROR);result=1;}
 timeEndPeriod(1);CoUninitialize();return result;
}
