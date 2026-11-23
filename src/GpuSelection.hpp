#pragma once
#include "Graphics.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include "SettingsFile.hpp"
#include <sstream>
struct GpuSelection {
 struct Item{int index;LUID luid;std::string name;size_t memory;};
 std::vector<Item> items;int selected=-1;bool missing=false;
 void load(const std::filesystem::path& file){
  ComPtr<IDXGIFactory1> factory;check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"Enumerate processing GPUs");
  for(unsigned i=0;;++i){ComPtr<IDXGIAdapter1> adapter;if(factory->EnumAdapters1(i,&adapter)==DXGI_ERROR_NOT_FOUND)break;DXGI_ADAPTER_DESC1 d{};if(!adapter||FAILED(adapter->GetDesc1(&d))||(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE))continue;
   char name[512]{};WideCharToMultiByte(CP_UTF8,0,d.Description,-1,name,sizeof(name),nullptr,nullptr);items.push_back({int(i),d.AdapterLuid,name,d.DedicatedVideoMemory});}
  long high=0;unsigned long low=0;std::istringstream f(readPortableSettings(file));if(f>>high>>low){missing=true;for(const auto& item:items)if(item.luid.HighPart==high&&item.luid.LowPart==low){selected=item.index;missing=false;break;}}
 }
 bool save(const std::filesystem::path& file)const{
  std::ostringstream f;if(selected<0)f<<"auto\n";else{bool found=false;for(const auto& i:items)if(i.index==selected){f<<i.luid.HighPart<<' '<<i.luid.LowPart<<'\n';found=true;break;}if(!found)return false;}return writePortableSettings(file,f.str());
 }
};
