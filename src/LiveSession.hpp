#pragma once
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <cmath>
#include <vector>

// Complements the existing audio/NR/subtitle settings with on/off and source
// selections. Normal double-click and --resume restore the last session;
// explicit diagnostic/test commands do not inherit it.
struct LiveSession {
 static bool shouldResume(const std::vector<std::wstring>& arguments){
  if(arguments.empty())return true;
  for(const auto& arg:arguments)if(arg==L"--resume")return true;
  return false;
 }
 std::map<std::string,std::string> values;bool valid=false;
 void load(const std::filesystem::path& file){values.clear();valid=false;std::ifstream in(file);int version=0;if(!(in>>version)||version!=1)return;std::string k,v;while(in>>std::quoted(k)>>std::quoted(v))values[k]=v;valid=!values.empty();}
 template<class T> void put(const std::string& key,T value){std::ostringstream s;s<<std::setprecision(17)<<value;values[key]=s.str();}
 std::string text(const std::string& key,const std::string& fallback={})const{auto it=values.find(key);return it==values.end()?fallback:it->second;}
 double number(const std::string& key,double fallback=0)const{try{auto s=text(key);size_t n=0;double v=std::stod(s,&n);return n==s.size()&&std::isfinite(v)?v:fallback;}catch(...){return fallback;}}
 void save(const std::filesystem::path& file)const{auto temp=file;temp+=L".tmp";{std::ofstream out(temp);out<<1<<'\n';for(auto& [key,v]:values)out<<std::quoted(key)<<' '<<std::quoted(v)<<'\n';if(!out)return;}MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}
};
