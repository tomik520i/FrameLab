#include "SubtitleTranslator.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <stdexcept>
double tick(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
int wmain(int argc,wchar_t** argv){try{
 if(argc!=2)throw std::runtime_error("Usage: subtitle_recovery_test ISOLATED_FAULT_FIXTURE");
 const std::filesystem::path app=argv[1];if(std::filesystem::exists(app/L"translation"/L"attempt.txt"))throw std::runtime_error("Use a fresh fixture");
 SubtitleTranslator worker(app);SubtitleSettings s;s.enabled=true;s.model=0;s.ocrEngine=0;s.left=s.top=0;s.right=s.bottom=1;s.intervalMs=150;
 std::vector<unsigned char> pixels(32*32*4,255);worker.configure(s);
 auto wait=[&](auto ready,double seconds){const auto end=tick()+seconds;while(tick()<end){worker.submit(pixels.data(),32,32,tick());if(ready())return;std::this_thread::sleep_for(std::chrono::milliseconds(20));}throw std::runtime_error("Timed out: "+worker.snapshot().status);};
 // Fixture fails the first startup, then exits during its first translation.
 // The unchanged OCR line must be translated again without a user toggle.
 wait([&]{return worker.snapshot().translated=="RECOVERED CAPTION";},35);
 auto result=worker.snapshot();if(result.performance.translationErrors!=2||result.activeModel!=0||result.loading)throw std::runtime_error("Recovery did not clear failed state");
 std::cout<<"PASS: failed startup + dead worker recovered automatically; unchanged OCR line requeued\n";
 s.enabled=false;worker.configure(s);wait([&]{return worker.snapshot().activeModel==-1;},5);
 std::ofstream(app/L"translation"/L"permanent.txt")<<1;
 s.enabled=true;worker.configure(s);wait([&]{return worker.snapshot().performance.translationErrors==3;},5);
 // User disable must cancel the scheduled retry, including in-flight loading.
 s.enabled=false;worker.configure(s);wait([&]{return worker.snapshot().activeModel==-1&&!worker.snapshot().loading;},5);
 auto count=[&]{int n=0;std::ifstream(app/L"translation"/L"attempt.txt")>>n;return n;};const int before=count();
 std::this_thread::sleep_for(std::chrono::seconds(3));if(count()!=before||worker.snapshot().activeModel!=-1)throw std::runtime_error("Disabled subtitles restarted a model");
 std::cout<<"PASS: disabling subtitles cancels automatic retry and unloads model\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
