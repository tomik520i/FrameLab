#include "SubtitleTranslator.hpp"
#include "Diagnostics.hpp"
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>

double now(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
int wmain(int argc,wchar_t** argv){try{
 if(argc!=4)throw std::runtime_error("Usage: subtitle_dialogue_pipeline APP FIXTURES AUTHORIZED_ENDPOINT (sends fixture text to that server)");
 std::filesystem::path app=argv[1],fixtures=argv[2];
 SubtitleSettings settings;settings.load(app/L"subtitle-settings.txt");
 if(settings.provider!=2||settings.customUrl!=std::filesystem::path(argv[3]).string())throw std::runtime_error("The configured custom server must match the explicitly supplied endpoint");
 settings.enabled=true;settings.inspectOcr=true;settings.contextTrace=(app/L"logs"/L"dialogue-trace.jsonl").string();
 SubtitleTranslator worker(app);worker.configure(settings);
 for(auto name:{"two-speakers","changed-reply","main-wrap"}){
  std::vector<unsigned char> pixels(2560*1440*4);
  std::ifstream in(fixtures/(std::string(name)+".bgra"),std::ios::binary);
  if(!in.read(reinterpret_cast<char*>(pixels.data()),pixels.size()))throw std::runtime_error("Missing fixture");
  auto start=now();bool success=false;SubtitleSnapshot state;
  while(now()-start<45){
   worker.submit(pixels.data(),2560,1440,now());std::this_thread::sleep_for(std::chrono::milliseconds(90));state=worker.snapshot();
   const auto& source=state.displaySource;
   bool rightCase=std::string(name)=="two-speakers"?source.find("he has a gun")!=std::string::npos:std::string(name)=="changed-reply"?source.find("Run!")!=std::string::npos:source.find("not to shoot him")!=std::string::npos;
   if(rightCase&&std::count(source.begin(),source.end(),'\n')==1&&std::count(state.translated.begin(),state.translated.end(),'\n')==1){success=true;break;}
   if(state.performance.translationErrors||state.performance.ocrErrors)throw std::runtime_error(state.status);
  }
  std::cout<<"{\"case\":"<<Diagnostics::quote(name)<<",\"passed\":"<<(success?"true":"false")<<",\"source\":"<<Diagnostics::quote(state.displaySource)<<",\"translation\":"<<Diagnostics::quote(state.translated)<<",\"ocr_ms\":"<<state.ocrMilliseconds<<",\"translation_ms\":"<<state.performance.translationMs<<",\"display_ms\":"<<state.performance.displayLatencyMs<<",\"fixture_to_display_ms\":"<<(now()-start)*1000<<"}"<<std::endl;
  if(!success)throw std::runtime_error("Caption not displayed with two distinct speaker blocks: "+state.status+" | "+state.source);
 }
 settings.enabled=false;worker.configure(settings);
 std::cout<<"PASS: generated pixels > real OCR > STYLE4 IPC > stability > configured custom translator with history > display queue. No desktop capture.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
