#pragma once
#include "SubtitleTranslator.hpp"
#include <Windows.h>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <limits>

// Named appearance + reading-area presets. Translation/audio/model choices are
// deliberately independent. Current custom settings also persist in settings v7.
class SubtitleProfiles {
public:
 struct Profile {uint64_t id=0;std::string name;std::array<float,4> area{};SubtitleStyleProfile style;};
 std::vector<Profile> entries;uint64_t selected=0;std::string error;
 void load(const std::filesystem::path& file){
  path=file;entries.clear();selected=0;error.clear();std::ifstream in(path);if(!in)return;
  unsigned version=0,count=0;if(!(in>>version>>selected>>count)||(version!=1&&version!=2)||count>128){error="Cannot read subtitle profiles";return;}
  for(unsigned i=0;i<count;++i){Profile p;auto& s=p.style;if(!(in>>p.id>>std::quoted(p.name)>>p.area[0]>>p.area[1]>>p.area[2]>>p.area[3]>>s.enabled>>s.height>>s.rgb[0]>>s.rgb[1]>>s.rgb[2]>>s.heightTolerance>>s.colorTolerance)||p.name.size()>160||!p.id){entries.clear();selected=0;error="Cannot read subtitle profiles";return;}
   if(version>=2&&!(in>>std::quoted(s.sampleText)>>s.sampleFrameHeight>>s.sampleId)){entries.clear();selected=0;error="Cannot read subtitle sample";return;}
   SubtitleSettings check;apply(p,check);p.area={check.left,check.top,check.right,check.bottom};p.style=check.style;entries.push_back(std::move(p));}
  if(!find(selected))selected=0;
 }
 const Profile* find(uint64_t id)const{for(auto& p:entries)if(p.id==id)return &p;return nullptr;}
 static void apply(const Profile& p,SubtitleSettings& settings){settings.style=p.style;settings.left=p.area[0];settings.top=p.area[1];settings.right=p.area[2];settings.bottom=p.area[3];settings.sanitize();}
 static bool matches(const Profile& p,const SubtitleSettings& s){return p.style==s.style&&p.area==std::array<float,4>{s.left,s.top,s.right,s.bottom};}
 bool choose(uint64_t id,SubtitleSettings& settings){auto p=find(id);if(!p)return false;apply(*p,settings);selected=id;return save();}
 bool add(std::string name,const SubtitleSettings& settings){
  name.erase(0,name.find_first_not_of(" \t\r\n"));auto last=name.find_last_not_of(" \t\r\n");if(last!=std::string::npos)name.resize(last+1);
  if(name.empty()||name.size()>160||entries.size()>=128){error="Enter a profile name (maximum 128 profiles)";return false;}
  const auto id=uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
  Profile p{id,std::move(name),{settings.left,settings.top,settings.right,settings.bottom},settings.style};while(find(p.id))++p.id;entries.push_back(p);selected=p.id;return save();
 }
 bool update(const SubtitleSettings& settings){for(auto& p:entries)if(p.id==selected){p.area={settings.left,settings.top,settings.right,settings.bottom};p.style=settings.style;return save();}return false;}
 bool removeSelected(){auto old=entries.size();std::erase_if(entries,[&](auto& p){return p.id==selected;});selected=0;return old!=entries.size()&&save();}
private:
 std::filesystem::path path;
 bool save(){
  if(path.empty()){error="Profile folder is unavailable";return false;}
  auto temporary=path;temporary+=L".tmp";std::ofstream out(temporary,std::ios::trunc);out<<std::setprecision(std::numeric_limits<float>::max_digits10)<<2<<' '<<selected<<' '<<entries.size()<<'\n';
  for(auto& p:entries){auto& s=p.style;out<<p.id<<' '<<std::quoted(p.name)<<' '<<p.area[0]<<' '<<p.area[1]<<' '<<p.area[2]<<' '<<p.area[3]<<' '<<s.enabled<<' '<<s.height<<' '<<s.rgb[0]<<' '<<s.rgb[1]<<' '<<s.rgb[2]<<' '<<s.heightTolerance<<' '<<s.colorTolerance<<' '<<std::quoted(s.sampleText)<<' '<<s.sampleFrameHeight<<' '<<s.sampleId<<'\n';}
  out.close();if(!out||!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){error="Could not save subtitle profiles";return false;}error.clear();return true;
 }
};
