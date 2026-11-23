#include "SubtitleTranslator.hpp"
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <iomanip>
double tick(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
int wmain(int argc,wchar_t** argv){try{
 if(argc<4||argc>6)throw std::runtime_error("Usage: subtitle_filter_pipeline APP FIXTURES MODEL_ID [COMPUTE [DEVICE_UUID]]");
 SubtitleTranslator worker(argv[1]);SubtitleSettings s;s.enabled=true;if(std::wstring(argv[3])==L"openai")s.provider=1;else s.model=std::stoi(argv[3]);s.ocrEngine=0;s.contentFilter=2;s.left=0;s.top=.5f;s.right=1;s.bottom=1;s.intervalMs=200;
 if(argc>4){auto value=std::wstring(argv[4]);s.computeBackend.assign(value.begin(),value.end());}if(argc>5){auto value=std::wstring(argv[5]);s.computeDevice.assign(value.begin(),value.end());}s.sanitize();worker.configure(s);
 struct Case{const char* file;bool skip;};
 const Case cases[]={{"dialogue-name",false},{"name-female",true},{"dialogue-code",false},{"name-male",true},{"dialogue-short",false},{"dialogue-no-punctuation",false}};
 for(const auto& c:cases){
  worker.reset();auto baseline=worker.snapshot().performance.filtered;std::vector<unsigned char> pixels(1280*720*4);std::ifstream in(std::filesystem::path(argv[2])/(std::string(c.file)+".bgra"),std::ios::binary);if(!in.read((char*)pixels.data(),pixels.size()))throw std::runtime_error("Missing fixture");
  auto started=tick(),deadline=started+120;SubtitleSnapshot state;bool success=false;
  while(tick()<deadline){worker.submit(pixels.data(),1280,720,tick());std::this_thread::sleep_for(std::chrono::milliseconds(80));state=worker.snapshot();
   if(c.skip&&!state.translated.empty())throw std::runtime_error(std::string("Name leaked: ")+c.file+" | "+state.source+" | "+state.translated);
   if(c.skip?state.performance.filtered>baseline:!state.translated.empty()){success=true;break;}
  }
  if(!success)throw std::runtime_error(std::string("No result: ")+c.file+" | "+state.status+" | OCR="+state.source);
  if(state.filterBackend!=(s.provider?"openai":"ner"))throw std::runtime_error("Requested filter backend did not load in real worker");
  if(s.computeBackend=="vulkan"&&!state.computeDescription.starts_with("VULKAN /"))throw std::runtime_error("Vulkan selection not confirmed by real worker");
  std::cout<<"COMPUTE "<<state.computeDescription<<std::endl;
  std::cout<<"{\"case\":"<<std::quoted(c.file)<<",\"model\":"<<s.model<<",\"filtered\":"<<(c.skip?"true":"false")<<",\"ocr\":"<<std::quoted(state.source)<<",\"translation\":"<<std::quoted(state.translated)<<",\"ms\":"<<(tick()-started)*1000<<",\"passed\":true}"<<std::endl;
 }
 s.enabled=false;worker.configure(s);std::cout<<"PASS: real OCR + filter + translation + IPC + subtitle queue\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
