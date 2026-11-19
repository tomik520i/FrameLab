#include "SubtitleTranslator.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>
#include <stdexcept>
double clockSeconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
int wmain(int argc,wchar_t** argv){try{
 if(argc!=9)throw std::runtime_error("Usage: subtitle_clip_test APP CROP_BGRA WIDTH HEIGHT FPS MODEL OCR OUTPUT_JSONL");
 const unsigned w=std::stoul(argv[3]),h=std::stoul(argv[4]);const double fps=std::stod(argv[5]);
 if(!w||!h||w>1920||h>1920||fps<1||fps>120)throw std::runtime_error("Invalid replay dimensions/rate");
 SubtitleTranslator worker(argv[1]);SubtitleSettings settings;settings.enabled=true;settings.left=settings.top=0;settings.right=settings.bottom=1;settings.model=std::stoi(argv[6]);settings.ocrEngine=std::stoi(argv[7]);settings.intervalMs=200;worker.configure(settings);
 std::vector<unsigned char> frame(size_t(w)*h*4,0);for(size_t p=3;p<frame.size();p+=4)frame[p]=255;
 auto deadline=clockSeconds()+180;
 while(clockSeconds()<deadline){worker.submit(frame.data(),w,h,clockSeconds());auto state=worker.snapshot();if(state.activeModel==settings.model&&!state.loading&&state.observedAt>0)break;if(state.status.starts_with("ERROR:")||state.status.find("failed")!=std::string::npos||state.status.find("missing")!=std::string::npos)throw std::runtime_error(state.status);std::this_thread::sleep_for(std::chrono::milliseconds(20));}
 if(clockSeconds()>=deadline)throw std::runtime_error("Warmup timed out: "+worker.snapshot().status);
 worker.reset();std::ifstream input(argv[2],std::ios::binary);std::ofstream output(argv[8]);if(!input||!output)throw std::runtime_error("Replay IO failed");
 const double start=clockSeconds();double lastObserved=-1;std::string lastDisplay;size_t index=0,shown=0;
 auto sample=[&]{auto state=worker.snapshot();if(state.observedAt!=lastObserved||state.translated!=lastDisplay){
  output<<"{\"time\":"<<clockSeconds()-start<<",\"source\":"<<std::quoted(state.source)<<",\"display_source\":"<<std::quoted(state.displaySource)<<",\"cs\":"<<std::quoted(state.translated)<<",\"ocr_ms\":"<<state.ocrMilliseconds<<",\"score\":"<<state.confidence<<",\"ocr_mt_ms\":"<<state.milliseconds<<",\"queue\":"<<state.pending<<"}\n";output.flush();
  if(!state.translated.empty()&&state.translated!=lastDisplay)++shown;lastObserved=state.observedAt;lastDisplay=state.translated;
 }};
 while(input.read(reinterpret_cast<char*>(frame.data()),frame.size())){
  const double due=start+index/fps;while(clockSeconds()<due){sample();std::this_thread::sleep_for(std::chrono::milliseconds(10));}
  worker.submit(frame.data(),w,h,clockSeconds());++index;sample();
 }
 const double tail=clockSeconds()+9;while(clockSeconds()<tail){sample();std::this_thread::sleep_for(std::chrono::milliseconds(10));}
 if(!worker.snapshot().translated.empty())throw std::runtime_error("Caption failed to expire");
 if(shown<5)throw std::runtime_error("Too few translations: "+std::to_string(shown)+" | "+worker.snapshot().status);
 worker.reset();if(!worker.snapshot().translated.empty())throw std::runtime_error("Reset retained text");
 std::cout<<"PASS frames="<<index<<" displayed="<<shown<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
