#include "SubtitleTranslator.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <stdexcept>

double tick(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
int wmain(int argc,wchar_t** argv){try{
 if(argc!=4)throw std::runtime_error("Usage: subtitle_stale_test ISOLATED_DELAYED_WORKER_APP DOOR_BGRA_1280x720 FOLLOW_BGRA_1280x720");
 const std::filesystem::path app=argv[1],ipc=app/L"translation";
 if(std::filesystem::exists(ipc/L"started-1"))throw std::runtime_error("Use a fresh isolated fixture directory");
 constexpr unsigned w=1280,h=720;std::vector<unsigned char> pixels(w*h*4),follow(w*h*4),blank(w*h*4,0);
 for(size_t i=3;i<blank.size();i+=4)blank[i]=255;
 std::ifstream file(argv[2],std::ios::binary);if(!file.read(reinterpret_cast<char*>(pixels.data()),pixels.size()))throw std::runtime_error("Missing fixture");
 std::ifstream second(argv[3],std::ios::binary);if(!second.read(reinterpret_cast<char*>(follow.data()),follow.size()))throw std::runtime_error("Missing second fixture");
 SubtitleTranslator worker(app);SubtitleSettings settings;settings.enabled=true;settings.intervalMs=200;worker.configure(settings);
 auto wait=[&](auto ready,const std::vector<unsigned char>* input,double seconds){
  const auto end=tick()+seconds;
  while(tick()<end){if(input)worker.submit(input->data(),w,h,tick());if(ready())return;std::this_thread::sleep_for(std::chrono::milliseconds(10));}
  throw std::runtime_error("Condition timed out: "+worker.snapshot().status);
 };
 wait([&]{return std::filesystem::exists(ipc/L"started-1");},&pixels,20);
 if(worker.snapshot().source!="The door is locked.")throw std::runtime_error("Real OCR did not read fixture");
 // OCR must keep running while IPC translation is deliberately held in flight.
 wait([&]{auto s=worker.snapshot();return s.source.empty()&&s.translated.empty();},&blank,3);
 if(std::filesystem::exists(ipc/L"completed-1"))throw std::runtime_error("Delayed translation was not in flight");
 std::ofstream(ipc/L"release-1")<<"release";
 wait([&]{return std::filesystem::exists(ipc/L"completed-1");},&blank,3);
 const auto until=tick()+.6;
 while(tick()<until){if(!worker.snapshot().translated.empty())throw std::runtime_error("Stale result reappeared after blank OCR");std::this_thread::sleep_for(std::chrono::milliseconds(10));}
 // Reset while another real OCR line is awaiting translation.
 wait([&]{return std::filesystem::exists(ipc/L"started-2");},&follow,5);
 if(worker.snapshot().source!="Follow me.")throw std::runtime_error("Second OCR fixture was not read");
 worker.reset();if(!worker.snapshot().translated.empty())throw std::runtime_error("Source reset retained old text");
 std::ofstream(ipc/L"release-2")<<"release";
 wait([&]{return std::filesystem::exists(ipc/L"completed-2");},&blank,3);
 const auto resetUntil=tick()+.6;
 while(tick()<resetUntil){if(!worker.snapshot().translated.empty())throw std::runtime_error("Previous source generation published a caption");std::this_thread::sleep_for(std::chrono::milliseconds(10));}
 std::cout<<"PASS: actual OCR continues during delayed IPC; blank and reset clear before completion; stale results rejected\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
