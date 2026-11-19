#include "SubtitleTranslator.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <stdexcept>
using Clock=std::chrono::steady_clock;
int wmain(int argc,wchar_t** argv){try{
 if(argc!=3)throw std::runtime_error("Usage: subtitle_model_lifecycle_test APP_DIR MODEL_ID(0/1/2)");
 SubtitleTranslator worker(argv[1]);SubtitleSettings settings;settings.model=std::stoi(argv[2]);settings.enabled=true;
 auto start=Clock::now();worker.configure(settings);
 if(Clock::now()-start>std::chrono::milliseconds(100))throw std::runtime_error("Configure blocks the caller");
 // Cancelling a load is asynchronous, including an in-flight model warmup.
 std::this_thread::sleep_for(std::chrono::milliseconds(250));settings.enabled=false;worker.configure(settings);
 std::this_thread::sleep_for(std::chrono::seconds(1));settings.enabled=true;worker.configure(settings);
 auto deadline=Clock::now()+std::chrono::seconds(180);bool ready=false;
 while(Clock::now()<deadline){auto state=worker.snapshot();if(state.activeModel==settings.model&&!state.loading){ready=true;break;}if(state.status.starts_with("ERROR:")||state.status.find("stopped")!=std::string::npos)throw std::runtime_error(state.status);std::this_thread::sleep_for(std::chrono::milliseconds(100));}
 if(!ready)throw std::runtime_error("Model failed to become ready: "+worker.snapshot().status);
 std::cout<<"READY model="<<settings.model<<" (functional lifecycle test, not benchmark)\n";
 settings.enabled=false;start=Clock::now();worker.configure(settings);
 if(Clock::now()-start>std::chrono::milliseconds(100))throw std::runtime_error("Unload blocks caller");
 deadline=Clock::now()+std::chrono::seconds(8);
 while(Clock::now()<deadline&&worker.snapshot().activeModel!=-1)std::this_thread::sleep_for(std::chrono::milliseconds(50));
 if(worker.snapshot().activeModel!=-1)throw std::runtime_error("Model did not unload");
 std::cout<<"PASS: async loading, cancelled warmup, reload and unload\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
