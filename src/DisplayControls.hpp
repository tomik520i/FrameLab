#pragma once
#include <Windows.h>
#include "UiTheme.hpp"
#include "Localization.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <filesystem>

struct DisplayControls {
 int fit=0;
 struct Monitor {HMONITOR handle;MONITORINFOEXW info;std::string label;};
 std::vector<Monitor> monitors;
 void refresh(){monitors.clear();EnumDisplayMonitors(nullptr,nullptr,[](HMONITOR h,HDC,LPRECT,LPARAM p)->BOOL{
  auto self=reinterpret_cast<DisplayControls*>(p);MONITORINFOEXW m{};m.cbSize=sizeof(m);if(GetMonitorInfoW(h,&m)){DEVMODEW dm{};dm.dmSize=sizeof(dm);EnumDisplaySettingsW(m.szDevice,ENUM_CURRENT_SETTINGS,&dm);self->monitors.push_back({h,m,std::string("Monitor ")+std::to_string(self->monitors.size()+1)+" | "+std::to_string(dm.dmPelsWidth)+" x "+std::to_string(dm.dmPelsHeight)+" | "+std::to_string(dm.dmDisplayFrequency)+" Hz"});}return TRUE;},reinterpret_cast<LPARAM>(this));}
 void load(const std::filesystem::path& path){std::ifstream(path)>>fit;fit=std::clamp(fit,0,2);refresh();}
 void save(const std::filesystem::path& path)const{std::ofstream(path)<<fit;}
 float scale(unsigned w,unsigned h,float areaW,float areaH)const{if(fit==2)return 1;return fit==1?std::max(areaW/w,areaH/h):std::min(areaW/w,areaH/h);}
 void panel(HWND window,bool fullscreen){
  const char* names[]={czech?"Přizpůsobit / zachovat celý obraz":"Fit / show whole image",czech?"Vyplnit / oříznout okraje":"Fill / crop edges",czech?"1:1 / zdrojové pixely":"1:1 / source pixels"};
  ui::title(czech?"ZOBRAZENÍ":"DISPLAY");ImGui::Combo(czech?"Měřítko obrazu":"Image fit",&fit,names,3);
  auto current=MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST);auto selected=std::find_if(monitors.begin(),monitors.end(),[&](const auto& m){return m.handle==current;});
  if(ImGui::BeginCombo(czech?"Monitor":"Monitor",selected==monitors.end()?"--":selected->label.c_str())){
   for(const auto& m:monitors)if(ImGui::Selectable(m.label.c_str(),m.handle==current)){RECT r{};GetWindowRect(window,&r);auto bounds=fullscreen?m.info.rcMonitor:m.info.rcWork;int width=fullscreen?bounds.right-bounds.left:std::min(r.right-r.left,bounds.right-bounds.left),height=fullscreen?bounds.bottom-bounds.top:std::min(r.bottom-r.top,bounds.bottom-bounds.top);SetWindowPos(window,nullptr,bounds.left+(bounds.right-bounds.left-width)/2,bounds.top+(bounds.bottom-bounds.top-height)/2,width,height,SWP_NOZORDER);}
   ImGui::EndCombo();
  }
  if(ImGui::Button(czech?"Obnovit monitory":"Refresh monitors"))refresh();
  ImGui::BeginDisabled(fullscreen);
  if(ImGui::BeginCombo(czech?"Velikost okna":"Window size",czech?"Vybrat…":"Choose…")){
   MONITORINFOEXW m{};m.cbSize=sizeof(m);GetMonitorInfoW(current,&m);std::vector<std::pair<int,int>> dimensions{{960,540},{1280,720},{1480,900},{1600,900},{1920,1080},{2560,1440},{3440,1440},{3840,2160}};
   DEVMODEW dm{};dm.dmSize=sizeof(dm);for(DWORD i=0;EnumDisplaySettingsW(m.szDevice,i,&dm);++i)if(dm.dmPelsWidth>=640&&dm.dmPelsHeight>=480)dimensions.emplace_back(int(dm.dmPelsWidth),int(dm.dmPelsHeight));
   std::sort(dimensions.begin(),dimensions.end());dimensions.erase(std::unique(dimensions.begin(),dimensions.end()),dimensions.end());
   for(auto [w,h]:dimensions){if(w>m.rcWork.right-m.rcWork.left||h>m.rcWork.bottom-m.rcWork.top)continue;std::string label=std::to_string(w)+" x "+std::to_string(h);if(ImGui::Selectable(label.c_str()))SetWindowPos(window,nullptr,m.rcWork.left+(m.rcWork.right-m.rcWork.left-w)/2,m.rcWork.top+(m.rcWork.bottom-m.rcWork.top-h)/2,w,h,SWP_NOZORDER);}
   ImGui::EndCombo();
  }ImGui::EndDisabled();
  if(ImGui::Button(czech?"Celá obrazovka (F11)":"Fullscreen (F11)"))SendMessageW(window,WM_KEYDOWN,VK_F11,0);
  ImGui::TextWrapped("%s",czech?"Okno a režim celé obrazovky používají aktuální rozlišení a frekvenci Windows. Měřítko zachovává poměr stran; Vyplnit a 1:1 mohou oříznout okraje. Rozlišení vstupu nastav ve Vstupu.":"The window and fullscreen use the current Windows resolution and refresh rate. Scaling keeps aspect ratio; Fill and 1:1 can crop edges. Choose capture resolution in Input.");
 }
};
