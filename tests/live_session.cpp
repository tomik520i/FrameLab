#include "LiveSession.hpp"
#include <stdexcept>
#include <iostream>
void require(bool ok){if(!ok)throw std::runtime_error("Session persistence regression");}
int main(int argc,char** argv){try{
 require(LiveSession::shouldResume({}));require(LiveSession::shouldResume({L"--resume",L"--fg",L"--welcome"}));
 require(!LiveSession::shouldResume({L"--demo",L"--fg"}));require(!LiveSession::shouldResume({L"--file",L"movie.mkv"}));require(!LiveSession::shouldResume({L"--not--resume"}));
 const auto path=argc>1?std::filesystem::path(argv[1]):std::filesystem::temp_directory_path()/(L"framelab-session-"+std::to_wstring(GetCurrentProcessId())+L".txt");
 LiveSession s;s.put("nr_enabled",1);s.put("fg",1);s.put("multiplier",2);s.put("quality",0);s.put("subtitles_enabled",1);s.put("debug_active",0);s.put("source_kind","capture");s.put("capture_device","AVerMedia GC573");if(!s.save(path)){std::cerr<<"Save failed: "<<GetLastError()<<" path="<<path.string()<<std::endl;return 1;}
 s.put("fullscreen_stats",1);s.save(path);
 LiveSession loaded;loaded.load(path);require(loaded.valid&&loaded.values==s.values&&loaded.number("fullscreen_stats")==1);loaded.put("stale",1);loaded.load(path);require(loaded.values==s.values);
 // A reader that denies replacement must not lose the latest complete save.
 auto locked=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);require(locked!=INVALID_HANDLE_VALUE);
 s.put("fullscreen_stats",0);require(s.save(path));loaded.load(path);require(loaded.values==s.values);CloseHandle(locked);
 auto sidecar=path;sidecar+=L".tmp";
 {std::ofstream interrupted(sidecar,std::ios::binary);interrupted<<"FRAMELAB_SETTINGS_V1 100\npartial";}
 loaded.load(path);require(loaded.number("fullscreen_stats")==1); // Last intact primary survives.
 require(s.save(path));loaded.load(path);require(loaded.values==s.values);
 std::filesystem::remove(sidecar);std::filesystem::remove(path);loaded.load(path);require(!loaded.valid&&loaded.values.empty());
 std::cout<<"PASS: direct EXE resume, explicit test isolation, exact session round trip, clean reload\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}
