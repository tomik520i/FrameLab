#pragma once
#include "AnalysisRecorder.hpp"
#include "LiveSession.hpp"
#include "imgui.h"
#include <shobjidl.h>
#include <thread>
#include <atomic>
#include <mutex>

struct RecordingLocation {
 std::filesystem::path config;std::string error;
 std::atomic<bool> choosing=false;std::mutex resultMutex;
 std::filesystem::path selectedPath;std::string pickerError;
 // Join before the worker's result storage and synchronization are destroyed.
 std::jthread picker;
 static std::string utf8(const std::filesystem::path& p){auto u=p.u8string();return {u.begin(),u.end()};}
 void load(const std::filesystem::path& root,AnalysisRecorder& recorder){
  config=root/L"recording-folder.txt";LiveSession s;s.load(config);auto value=s.text("folder");
  if(!value.empty())recorder.destination=std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(value.data()),value.size()));
 }
 void panel(HWND owner,const std::filesystem::path& root,AnalysisRecorder& recorder,bool busy,bool cs){
  if(!choosing&&picker.joinable()){
   picker.join();std::lock_guard lock(resultMutex);error=std::move(pickerError);
   if(!selectedPath.empty()){recorder.destination=std::move(selectedPath);selectedPath.clear();LiveSession s;s.put("folder",utf8(recorder.destination));s.save(config);}
  }
  const auto target=recorder.destination.empty()?root/L"recordings":recorder.destination;
  ImGui::TextWrapped("%s: %s",cs?"Složka záznamů":"Recording folder",utf8(target).c_str());
  std::error_code ec;const auto disk=std::filesystem::space(recorder.destination.empty()?root:target,ec);
  if(ec)ImGui::TextWrapped("%s",cs?"Složka není dostupná. Vyber jinou.":"Folder unavailable. Choose another location.");
  else {ImGui::Text("%s: %.1f GiB",cs?"Volné místo":"Free space",double(disk.available)/1073741824.);
   if(disk.available<36ull*1024*1024*1024)ImGui::TextWrapped("%s",cs?"Málo místa může zkrátit záznam. Pro 1440p se všemi efekty doporučujeme alespoň 36 GiB; záleží na obsahu.":"Low disk space can shorten a take. For 1440p with all effects, allow at least 36 GiB; actual requirements depend on the footage.");}
  ImGui::BeginDisabled(busy||choosing);
  if(ImGui::Button(cs?"Změnit složku...":"Choose folder...")){
   // The render/capture thread is MTA. Shell dialogs require their own STA;
   // never block that render thread on COM activation or filesystem browsing.
   choosing=true;picker=std::jthread([this,owner,cs]{
    const HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);HRESULT hr=initialized;std::filesystem::path chosen;
    if(SUCCEEDED(hr)){
     {ComPtr<IFileOpenDialog> dialog;hr=CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog));
      if(SUCCEEDED(hr)){DWORD options=0;dialog->GetOptions(&options);dialog->SetOptions(options|FOS_PICKFOLDERS|FOS_FORCEFILESYSTEM|FOS_PATHMUSTEXIST);dialog->SetTitle(cs?L"Složka pro záznamy FrameLab":L"FrameLab recording folder");hr=dialog->Show(owner);
       if(SUCCEEDED(hr)){ComPtr<IShellItem> selected;hr=dialog->GetResult(&selected);PWSTR path=nullptr;if(SUCCEEDED(hr))hr=selected->GetDisplayName(SIGDN_FILESYSPATH,&path);if(SUCCEEDED(hr)){chosen=path;CoTaskMemFree(path);}}}}
     CoUninitialize();
    }
    {std::lock_guard lock(resultMutex);selectedPath=std::move(chosen);pickerError=FAILED(hr)&&hr!=HRESULT_FROM_WIN32(ERROR_CANCELLED)?(cs?"Výběr složky se nezdařil.":"Could not select a recording folder."):"";}
    choosing=false;
   });
  }
  ImGui::EndDisabled();if(!error.empty())ImGui::TextWrapped("%s",error.c_str());
 }
};
