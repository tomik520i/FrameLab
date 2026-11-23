#include "SubtitleWindowOverlay.hpp"
#include "OpenAiSettings.hpp"
#include <iostream>
#include <filesystem>

void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int wmain(int argc,wchar_t** argv){
 try{
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  SubtitleRaster raster;auto image=raster.get("Příliš žluťoučký kůň\n日本語字幕",42,900,"cs",false,1,true);
  require(image&&!image->texture.texture,"CPU text requires no GPU texture");
  require(!image->premultipliedBgra.empty()&&image->height>40,"Missing physical-pixel subtitle bitmap");
  unsigned white=0,transparent=0;
  for(size_t i=0;i<image->premultipliedBgra.size();i+=4){auto* p=image->premultipliedBgra.data()+i;require(p[0]<=p[3]&&p[1]<=p[3]&&p[2]<=p[3],"Non-premultiplied pixel");if(!p[3])++transparent;if(p[0]>200&&p[1]>200&&p[2]>200)++white;}
  require(white>300&&transparent>300,"Text or transparency missing");
  auto smallImage=raster.get("Stejný text",32,900,"cs",false,2,true);auto h=smallImage->height;
  auto big=raster.get("Stejný text",64,1800,"cs",false,3,true);require(big->height>h*1.8,"Output-size text rerasterization failed");
  SubtitleSettings s;s.provider=1;s.model=0;s.sourceLanguage="ja";s.targetLanguage="de";s.enabled=true;s.sanitize();require(s.enabled,"API language pair incorrectly blocked by retained local model");
  s.apiModel="bad\" --injection";s.sanitize();require(s.apiModel=="gpt-6-luna","Unsafe model ID accepted");
  s.computeBackend="vulkan";s.computeDevice="bad\" --injection";s.sanitize();require(s.computeDevice.empty(),"Unsafe GPU identifier accepted");
  s.computeDevice="8680a8a7040000000002000000000000";s.allowCpuOffload=true;
  auto file=std::filesystem::temp_directory_path()/(L"framelab-cloud-test-"+std::to_wstring(GetCurrentProcessId())+L".txt");s.apiModel="gpt-6.1-terra";s.apiEffort="low";s.apiTemperature=.2f;s.save(file);SubtitleSettings loaded;loaded.load(file);require(!loaded.enabled&&loaded.provider==1&&loaded.apiModel==s.apiModel&&loaded.sourceLanguage=="ja","Cloud settings round trip failed");std::filesystem::remove(file);
  if(argc>1){OpenAiSettings api(argv[1]);require(api.error.empty()&&api.profiles.size()>=3,"Native capability catalog missing");require(api.profile("gpt-6-luna")&&api.profile("gpt-6-luna")->temperature("none"),"Default profile invalid");require(!api.profile("gpt-6-luna-fake"),"Unknown snapshot inherits capability");}
  require(loaded.computeBackend=="vulkan"&&loaded.computeDevice==s.computeDevice&&loaded.allowCpuOffload,"Translation GPU settings did not survive restart");
  // Own test windows only; no synthetic input or automation of another app.
  HWND owner=CreateWindowExW(0,L"STATIC",L"FrameLab overlay test settings",WS_OVERLAPPEDWINDOW,30,30,300,150,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
  HWND source=CreateWindowExW(0,L"STATIC",L"FrameLab overlay test source",WS_OVERLAPPEDWINDOW|WS_VISIBLE,100,100,960,600,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
  require(owner&&source,"Fixture creation failed");SetForegroundWindow(source);
  SubtitleWindowOverlay overlay;require(overlay.start(owner,source),"Overlay creation/hotkey failed");
  auto ex=GetWindowLongPtrW(overlay.handle(),GWL_EXSTYLE);require((ex&(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW))==(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW),"Click-through style missing");
  require(SendMessageW(overlay.handle(),WM_NCHITTEST,0,0)==HTTRANSPARENT,"Overlay intercepts mouse hit test");
  s={};s.enabled=true;SubtitleSnapshot caption;caption.translated="Johne, počkej! Kód je 1234.";caption.expiresAt=100;
  overlay.update(s,caption,1);if(GetForegroundWindow()==source){require(overlay.visible()&&overlay.updates()==1,"Caption not shown");overlay.update(s,caption,2);require(overlay.updates()==1,"Unchanged caption rerasterized");ShowWindow(owner,SW_MINIMIZE);overlay.update(s,caption,3);require(overlay.visible(),"Minimized settings hide game subtitles");overlay.update(s,caption,101);require(!overlay.visible(),"Expired caption visible");}
  overlay.update(s,caption,1,true);require(!overlay.visible(),"Paused source caption visible");DestroyWindow(source);overlay.update(s,caption,1);require(!overlay.active(),"Closed source leaves stale overlay");DestroyWindow(owner);
  std::cout<<"PASS: CPU Unicode pixels, alpha, native output scale, cloud settings, profiles, overlay lifecycle / click-through\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
