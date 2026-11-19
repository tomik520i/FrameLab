#include "SubtitleTranslator.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <stdexcept>
#include <iomanip>

double tick(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
int wmain(int argc,wchar_t** argv){try{
 if(argc<3||argc>4)throw std::runtime_error("Usage: subtitle_pipeline_test APP_DIR FIXTURE_BGRA_1280x720 [MODEL_ID 0/1/2]");
 constexpr unsigned w=1280,h=720;std::vector<unsigned char> pixels(w*h*4);std::ifstream f(argv[2],std::ios::binary);if(!f.read(reinterpret_cast<char*>(pixels.data()),pixels.size()))throw std::runtime_error("Missing fixture");
 SubtitleTranslator worker(argv[1]);SubtitleSettings settings;settings.enabled=true;settings.intervalMs=200;settings.model=argc==4?std::stoi(argv[3]):0;worker.configure(settings);
 const auto deadline=tick()+180;SubtitleSnapshot value;
 while(tick()<deadline){worker.submit(pixels.data(),w,h,tick());std::this_thread::sleep_for(std::chrono::milliseconds(100));value=worker.snapshot();if(!value.translated.empty())break;}
 if(value.source!="The door is locked."||value.translated!="Dveře jsou zamčené.")throw std::runtime_error("Unexpected OCR / MT: "+value.status+" | "+value.source+" | "+value.translated);
 std::cout<<"{\"model\":"<<settings.model<<",\"source\":"<<std::quoted(value.source)<<",\"translated\":"<<std::quoted(value.translated)<<",\"milliseconds\":"<<value.milliseconds<<"}\n";
 const auto staticUntil=tick()+4;
 while(tick()<staticUntil){worker.submit(pixels.data(),w,h,tick());std::this_thread::sleep_for(std::chrono::milliseconds(100));if(worker.snapshot().translated!="Dveře jsou zamčené.")throw std::runtime_error("Visible source lost its Czech caption");}
 // Blank input must not flash away an already displayed translation. Its
 // reading timer begins on display, then it expires even without further OCR.
 for(size_t i=0;i<pixels.size();i+=4){pixels[i]=pixels[i+1]=pixels[i+2]=0;pixels[i+3]=255;}
 const auto firstDisplay=value.displayedAt;const auto blankDeadline=tick()+9;bool cleared=false;
 while(tick()<blankDeadline){worker.submit(pixels.data(),w,h,tick());std::this_thread::sleep_for(std::chrono::milliseconds(100));auto state=worker.snapshot();if(state.source.empty()&&state.translated.empty()){cleared=true;break;}}
 if(!cleared||tick()-firstDisplay<settings.holdSeconds)throw std::runtime_error("Caption reading lifetime incorrect");
 worker.reset();if(!worker.snapshot().translated.empty())throw std::runtime_error("Reset retained a caption");settings.enabled=false;worker.configure(settings);worker.submit(pixels.data(),w,h,tick());std::this_thread::sleep_for(std::chrono::milliseconds(300));if(!worker.snapshot().source.empty())throw std::runtime_error("Disabled worker produced OCR");
 std::cout<<"PASS: actual neural OCR, offline EN-CS model, readable lifetime, expiry, reset and disabled state\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
