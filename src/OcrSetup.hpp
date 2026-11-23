#pragma once
#include <Windows.h>
#include "SubtitleLanguages.hpp"
#include <shellapi.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Media.Ocr.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <string>

// Detection and Windows servicing stay off the rendering thread. Only a user's
// Install click queues elevation; closing FrameLab never kills Windows servicing.
class OcrSetup {
public:
 struct State {int available=-1;bool installing=false;std::string outcome,language="en";};
private:
 std::mutex mutex;std::condition_variable wake;std::atomic<bool> stop=false;
 State state;std::string language="en";HWND requested=nullptr;bool recheck=true;std::thread worker;
 void check(const std::string& source){
  bool ready=false;try{ready=bool(winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(winrt::Windows::Globalization::Language(subtitleLanguage(source)->windows)));}catch(...){ready=false;}
  std::lock_guard lock(mutex);if(language==source){state.available=ready?1:0;state.language=source;}
 }
 void run(){
  try{
   winrt::init_apartment(winrt::apartment_type::multi_threaded);
   while(!stop){
    HWND parent=nullptr;bool refresh=false;std::string source;
    {std::unique_lock lock(mutex);wake.wait_for(lock,std::chrono::seconds(5),[&]{return stop||requested||recheck;});if(stop)break;parent=requested;requested=nullptr;refresh=recheck||state.available!=1;recheck=false;source=language;}
    if(parent){
     wchar_t system[MAX_PATH]{};GetSystemDirectoryW(system,MAX_PATH);
     const std::wstring exe=std::wstring(system)+L"\\WindowsPowerShell\\v1.0\\powershell.exe";
     // Constant command: no user text, model paths or downloaded scripts execute elevated.
     const wchar_t* args=L"-NoProfile -NonInteractive -Command \"try { Add-WindowsCapability -Online -Name 'Language.OCR~~~en-US~0.0.1.0' -ErrorAction Stop | Out-Null; if ((Get-WindowsCapability -Online -Name 'Language.OCR~~~en-US~0.0.1.0' -ErrorAction Stop).State -eq 'Installed') { exit 0 }; exit 2 } catch { exit 1 }\"";
     SHELLEXECUTEINFOW info{sizeof(info)};info.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC;info.hwnd=parent;info.lpVerb=L"runas";info.lpFile=exe.c_str();info.lpParameters=args;info.nShow=SW_HIDE;
     DWORD code=1;std::string outcome;
     if(!ShellExecuteExW(&info))outcome=GetLastError()==ERROR_CANCELLED?"cancelled":"launch-failed";
     else if(info.hProcess){
      while(!stop&&WaitForSingleObject(info.hProcess,200)==WAIT_TIMEOUT){}
      if(!stop)GetExitCodeProcess(info.hProcess,&code);
      CloseHandle(info.hProcess);outcome=code==0?"installed":"failed";
     }
     {std::lock_guard lock(mutex);state.installing=false;state.outcome=outcome;}
     refresh=true;
    }
    if(refresh&&!stop)check(source);
   }
   winrt::uninit_apartment();
  }catch(...){std::lock_guard lock(mutex);state.available=0;state.installing=false;state.outcome="check-failed";}
 }
public:
 OcrSetup():worker([this]{run();}){}
 ~OcrSetup(){stop=true;wake.notify_all();worker.join();}
 void select(const std::string& source){std::lock_guard lock(mutex);if(!subtitleLanguage(source)||source==language)return;language=source;state.available=-1;state.language=source;state.outcome.clear();recheck=true;wake.notify_one();}
 State snapshot(){std::lock_guard lock(mutex);return state;}
 void install(HWND parent){std::lock_guard lock(mutex);if(language!="en"||!parent||state.installing||state.available==1)return;state.installing=true;state.outcome.clear();requested=parent;wake.notify_one();}
 void refresh(){std::lock_guard lock(mutex);recheck=true;wake.notify_one();}
};
