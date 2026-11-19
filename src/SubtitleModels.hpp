#pragma once
#include <Windows.h>
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <thread>

struct SubtitleModelInfo {
 const char* id;const char* name;double downloadGB,requiredGiB;
};
inline constexpr std::array<SubtitleModelInfo,4> subtitleModelCatalog{{
 {"cpu","OPUS-MT / CPU",0,0},
 {"cuni-q3km","CUNI-MH 7B / Q3_K_M",3.52,4.4},
 {"cuni-q8","CUNI-MH 7B / Q8_0",7.70,8.5},
 {"cuni-q5km","CUNI-MH 7B / Q5_K_M",5.13,6.0}
}};
struct SubtitleModelDownload {std::string phase="starting",detail;uint64_t done=0,total=0;};
// Filesystem and helper lifecycle live on a background thread, never in render().
class SubtitleModels {
 std::filesystem::path app,session;std::mutex mutex;std::array<SubtitleModelDownload,subtitleModelCatalog.size()> states;
 std::string requested;std::atomic<bool> stop=false;std::thread worker;
 void run(){
  HANDLE process=nullptr,job=nullptr;
  try{
   std::filesystem::create_directories(session);
   const auto python=app/L"translation"/L"python"/L"python.exe",script=app/L"translation"/L"subtitle_model_manager.py";
   if(!std::filesystem::exists(python)||!std::filesystem::exists(script))throw std::runtime_error("Subtitle model manager files missing. Use the complete portable package.");
   job=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
   if(!job||!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))throw std::runtime_error("Cannot create download helper job");
   std::wstring command=L"\""+python.wstring()+L"\" \""+script.wstring()+L"\" --root \""+(app/L"translation").wstring()+L"\" --session \""+session.wstring()+L"\"";
   STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION pi{};
   if(!CreateProcessW(python.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,nullptr,&startup,&pi))throw std::runtime_error("Cannot start subtitle download helper");
   process=pi.hProcess;
   if(!AssignProcessToJobObject(job,process)){TerminateProcess(process,1);CloseHandle(pi.hThread);throw std::runtime_error("Cannot isolate subtitle helper");}
   ResumeThread(pi.hThread);CloseHandle(pi.hThread);uint64_t sequence=0;
   while(!stop){
    if(WaitForSingleObject(process,0)!=WAIT_TIMEOUT)throw std::runtime_error("Subtitle model helper stopped; reinstall translation tools");
    std::string command;{std::lock_guard lock(mutex);command=std::move(requested);requested.clear();}
    if(!command.empty()){
     const auto temporary=session/L"command.tmp",target=session/L"command.txt";
     {std::ofstream f(temporary);f<<++sequence<<' '<<command;}
     if(!MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING))throw std::runtime_error("Cannot write subtitle model command");
    }
    std::ifstream f(session/L"status.txt");std::string id,phase,detail;uint64_t done,total;
    while(f>>std::quoted(id)>>std::quoted(phase)>>done>>total>>std::quoted(detail)){
     for(size_t i=1;i<states.size();++i)if(id==subtitleModelCatalog[i].id){std::lock_guard lock(mutex);states[i]={phase,detail,done,total};}
    }
    for(int i=0;i<5&&!stop;++i)Sleep(50);
   }
  }catch(const std::exception& e){std::lock_guard lock(mutex);for(size_t i=1;i<states.size();++i)states[i]={"error",e.what(),0,0};}
  if(job)CloseHandle(job);if(process){WaitForSingleObject(process,2000);CloseHandle(process);}
 }
public:
 explicit SubtitleModels(const std::filesystem::path& p):app(p),session(p/L"logs"/(L"subtitle-models-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()))),worker([this]{run();}){}
 ~SubtitleModels(){stop=true;worker.join();}
 SubtitleModelDownload snapshot(int i){std::lock_guard lock(mutex);return states.at(i);}
 void download(int i){if(i<1||i>=int(states.size()))return;std::lock_guard lock(mutex);requested=std::string("download ")+subtitleModelCatalog[i].id;}
 void cancel(){std::lock_guard lock(mutex);requested="cancel";}
};
