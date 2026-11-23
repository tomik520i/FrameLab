#pragma once
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <iterator>

// A completed sidecar remains readable when Windows temporarily locks the
// destination against replacement. Length framing rejects interrupted writes.
inline std::string readPortableSettings(const std::filesystem::path& file){
 auto read=[](const std::filesystem::path& path,bool framedOnly,std::string& value){
  std::error_code ec;auto size=std::filesystem::file_size(path,ec);if(ec||size>1024*1024)return false;
  std::ifstream in(path,std::ios::binary);if(!in)return false;std::string data((std::istreambuf_iterator<char>(in)),{});if(in.bad())return false;
  const std::string prefix="FRAMELAB_SETTINGS_V1 ";
  if(!data.starts_with(prefix)){if(framedOnly)return false;value=std::move(data);return true;}
  auto newline=data.find('\n');if(newline==std::string::npos)return false;
  size_t length=0;std::istringstream header(data.substr(prefix.size(),newline-prefix.size()));char extra;
  if(!(header>>length)||(header>>extra)||length!=data.size()-newline-1)return false;
  value=data.substr(newline+1);return true;
 };
 std::string primary,pending;const bool valid=read(file,false,primary);auto temporary=file;temporary+=L".tmp";
 if(read(temporary,true,pending)){
  std::error_code a,b;auto pendingTime=std::filesystem::last_write_time(temporary,a),primaryTime=std::filesystem::last_write_time(file,b);
  if(!valid||(!a&&(b||pendingTime>=primaryTime)))return pending;
 }
 return primary;
}

inline bool writePortableSettings(const std::filesystem::path& file,const std::string& contents){
 auto temporary=file;temporary+=L".tmp";
 {std::ofstream out(temporary,std::ios::binary);out<<"FRAMELAB_SETTINGS_V1 "<<contents.size()<<'\n';out.write(contents.data(),std::streamsize(contents.size()));out.close();if(!out)return false;}
 if(MoveFileExW(temporary.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return true;
 const auto error=GetLastError();if(error!=ERROR_ACCESS_DENIED&&error!=ERROR_SHARING_VIOLATION)return false;
 // The full framed sidecar has been closed successfully. The matching reader
 // selects it over the older destination; never truncate the previous copy.
 return true;
}
