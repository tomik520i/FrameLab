#include "SubtitleProfiles.hpp"
#include <iostream>
#include <stdexcept>
void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
int main(){try{
 auto folder=std::filesystem::temp_directory_path()/("framelab-profile-test-"+std::to_string(GetCurrentProcessId()));std::filesystem::create_directories(folder);auto path=folder/L"profiles.txt";
 SubtitleProfiles p;p.load(path);SubtitleSettings white;white.style.enabled=true;white.style.height=.033333333f;white.style.rgb={245,245,245};white.top=.65f;
 white.style.sampleText="Follow \"me\" / vzor";white.style.sampleFrameHeight=1440;white.style.sampleId=1795190000000;
 check(p.add("GTA bílé titulky",white),"Save white");auto whiteId=p.selected;
 SubtitleSettings yellow=white;yellow.style.rgb={255,220,35};yellow.style.height=.04861111f;yellow.top=.2f;
 check(p.add("Film / žluté titulky",yellow),"Save yellow");auto yellowId=p.selected;
 SubtitleProfiles reloaded;reloaded.load(path);check(reloaded.entries.size()==2&&reloaded.selected==yellowId,"Restart lost profile selection");
 SubtitleSettings restored;restored.model=3;restored.enabled=true;check(reloaded.choose(whiteId,restored),"Select white");check(restored.style==white.style&&restored.top==white.top&&restored.model==3&&restored.enabled,"Wrong selected style or changed translation backend");
 restored.style.height=.041234567f;check(!SubtitleProfiles::matches(*reloaded.find(whiteId),restored),"Modified state not detected");check(reloaded.update(restored),"Update failed");
 SubtitleProfiles again;again.load(path);check(again.find(whiteId)->style==restored.style,"Saved modification lost precision");check(again.removeSelected(),"Delete failed");SubtitleProfiles final;final.load(path);check(final.entries.size()==1&&final.find(yellowId),"Delete removed other profile");
 check(!final.add("   ",restored),"Blank name accepted");
 {std::ofstream legacy(path);legacy<<"1 42 1\n42 \"Old profile\" 0.1 0.7 0.9 0.97 1 0.03 245 245 245 0.42 35\n";}
 SubtitleProfiles legacy;legacy.load(path);check(legacy.entries.size()==1&&legacy.find(42)->style.sampleText.empty()&&legacy.find(42)->style.enabled,"Legacy profile no longer loads");
 auto settingsPath=folder/L"settings.txt";white.inspectOcr=true;white.save(settingsPath);SubtitleSettings saved;saved.load(settingsPath);check(saved.style==white.style&&!saved.inspectOcr,"Sample persistence or transient inspector state incorrect");std::filesystem::remove(settingsPath);
 std::filesystem::remove(path);std::filesystem::remove(folder);std::cout<<"PASS: named profiles, Czech names, restart, last selection, independent backend, modifications, delete\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
