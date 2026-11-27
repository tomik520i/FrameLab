#include "SubtitleTranslator.hpp"
#include <windows.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <stdexcept>
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
double seconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
int wmain(int argc,wchar_t** argv){try{
 require(argc==3,"Usage: custom_deadline_pipeline APP PORT");std::filesystem::path app=argv[1];
 SubtitleSettings s;s.provider=2;s.customUrl="http://127.0.0.1:"+std::to_string(std::stoi(argv[2]))+"/v1";s.customModel="test-model-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64());s.enabled=true;s.conversationMemory=true;s.selectedMemory=false;s.contentFilter=0;s.intervalMs=200;
 std::cout<<"RUN_ID="<<s.customModel<<std::endl;
 require(s.customTimeout==2&&s.customMaxTokens==1000,"Wrong defaults");s.save(app/L"deadline-settings.txt");SubtitleSettings saved;saved.load(app/L"deadline-settings.txt");require(saved.customTimeout==2&&saved.customMaxTokens==1000,"Deadline not persisted");
 SubtitleTranslator worker(app);worker.configure(s);std::vector<unsigned char> pixels(640*360*4,0);
 auto source=[&](const char* text){std::ofstream out(app/L"test-source.txt");out<<text;out.flush();require(out.good(),"Fixture write failed");};
 auto pump=[&]{worker.submit(pixels.data(),640,360,seconds());std::this_thread::sleep_for(std::chrono::milliseconds(40));return worker.snapshot();};
 auto until=[&](auto predicate,double limit){auto start=seconds();while(seconds()-start<limit){if(predicate(pump()))return;}throw std::runtime_error("Caption pipeline timed out");};
 auto hold=[&](double duration){auto start=seconds();while(seconds()-start<duration)pump();};
 source("Jason: Before the delay.");until([](auto state){return state.translated=="Jason: Před čekáním.";},5);
 source("Jason: Slow subtitle.");until([](auto state){return state.performance.translationBusy;},3);auto busyStart=seconds();
 hold(.25);source("Jason: Old pending one.");hold(.32);source("Jason: Old pending two.");hold(.32);source("Jason: Latest subtitle.");
 until([](auto state){return state.translated=="Jason: Nejnovější titulek.";},4);
 auto result=worker.snapshot();require(result.performance.translationErrors==1,"Expected exactly one timeout");require(result.activeModel>=0,"Worker unloaded after HTTP deadline");require(seconds()-busyStart<3.6,"Latest caption was held behind obsolete work");
 source("Lucia: Keep translating.");until([](auto state){return state.translated=="Lucia: Překlad pokračuje.";},4);
 s.enabled=false;worker.configure(s);
 std::cout<<"PASS: two-second timeout, obsolete queue skipped, latest caption displayed, history retained, continued without toggle\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
