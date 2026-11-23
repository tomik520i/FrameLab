#include "SubtitleTranslator.hpp"
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <iomanip>
double tick(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
int wmain(int argc,wchar_t** argv){try{
 if(argc!=3)throw std::runtime_error("Usage: subtitle_language_switch_test APP FIXTURES");
 SubtitleTranslator worker(argv[1]);SubtitleSettings s;s.enabled=true;s.model=4;s.ocrEngine=0;s.left=0;s.right=1;s.top=.65f;s.bottom=1;s.sourceLanguage="de";s.targetLanguage="en";
 std::vector<unsigned char> pixels(1280*720*4);
 auto run=[&](const char* source,const char* target){
  std::ifstream input(std::filesystem::path(argv[2])/(std::string(source)+"-36.bgra"),std::ios::binary);if(!input.read((char*)pixels.data(),pixels.size()))throw std::runtime_error("Missing source fixture");
  s.sourceLanguage=source;s.targetLanguage=target;auto start=tick();worker.configure(s);if(tick()-start>.1)throw std::runtime_error("Language switch blocked caller");
  if(!worker.snapshot().translated.empty())throw std::runtime_error("Old target caption survived language switch");
  const auto deadline=tick()+150;SubtitleSnapshot state;
  while(tick()<deadline){worker.submit(pixels.data(),1280,720,tick());std::this_thread::sleep_for(std::chrono::milliseconds(80));state=worker.snapshot();if(!state.translated.empty())break;}
  if(state.translated.empty())throw std::runtime_error("No caption: "+state.status);
  std::cout<<source<<" > "<<target<<" OCR="<<std::quoted(state.source)<<" MT="<<std::quoted(state.translated)<<" OCR_ms="<<state.ocrMilliseconds<<" total_ms="<<state.milliseconds<<"\n"<<std::flush;
  if(std::string(target)=="en"&&state.translated.find("door")==std::string::npos)throw std::runtime_error("Unexpected English output");
  if(std::string(target)=="cs"&&state.translated.find("dve")==std::string::npos)throw std::runtime_error("Unexpected Czech output");
 };
 run("de","en");run("de","cs");run("ja","en");
 s.enabled=false;worker.configure(s);auto deadline=tick()+8;while(tick()<deadline&&worker.snapshot().activeModel!=-1)std::this_thread::sleep_for(std::chrono::milliseconds(50));
 if(worker.snapshot().activeModel!=-1)throw std::runtime_error("Unload failed");
 std::cout<<"PASS: actual OCR + GPU MT, target switch on identical source, source alphabet switch, flush and unload\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
