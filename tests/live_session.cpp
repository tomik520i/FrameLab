#include "LiveSession.hpp"
#include <stdexcept>
#include <iostream>
void require(bool ok){if(!ok)throw std::runtime_error("Session persistence regression");}
int main(){
 require(LiveSession::shouldResume({}));require(LiveSession::shouldResume({L"--resume",L"--fg",L"--welcome"}));
 require(!LiveSession::shouldResume({L"--demo",L"--fg"}));require(!LiveSession::shouldResume({L"--file",L"movie.mkv"}));require(!LiveSession::shouldResume({L"--not--resume"}));
 const auto path=std::filesystem::temp_directory_path()/(L"framelab-session-"+std::to_wstring(GetCurrentProcessId())+L".txt");
 LiveSession s;s.put("nr_enabled",1);s.put("fg",1);s.put("multiplier",2);s.put("quality",0);s.put("subtitles_enabled",1);s.put("debug_active",0);s.put("source_kind","capture");s.put("capture_device","AVerMedia GC573");s.save(path);
 s.put("fullscreen_stats",1);s.save(path);
 LiveSession loaded;loaded.load(path);require(loaded.valid&&loaded.values==s.values&&loaded.number("fullscreen_stats")==1);loaded.put("stale",1);loaded.load(path);require(loaded.values==s.values);
 std::filesystem::remove(path);loaded.load(path);require(!loaded.valid&&loaded.values.empty());
 std::cout<<"PASS: direct EXE resume, explicit test isolation, exact session round trip, clean reload\n";
}
