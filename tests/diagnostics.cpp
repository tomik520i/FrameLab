#include "Diagnostics.hpp"
#include <iostream>
#include <iterator>
#include <stdexcept>
static std::string read(const std::filesystem::path& p){std::ifstream f(p);return {std::istreambuf_iterator<char>(f),{}};}
int main(){try{
 auto root=std::filesystem::temp_directory_path()/"framelab-diagnostics-tests";
 Diagnostics d;
 if(!d.start(root,"{\"test\":true}",true,false,10))throw std::runtime_error("start");
 auto folder=d.folder();d.event("ocr_complete",{{"job_id",7},{"ms",2.5}},"PRIVATE SOURCE","PRIVATE TRANSLATION");
 for(int i=0;i<100;++i){d.sample({{"fps",60},{"unavailable",-1}});d.frame({{"new_content",1},{"content_interval_ms",16.667},{"present_cpu_ms",.1}});}
 d.stop();while(d.busy())std::this_thread::sleep_for(std::chrono::milliseconds(2));
 if(d.status()!="Saved"||d.dropped())throw std::runtime_error("recording failed: "+d.status());
 auto events=read(folder/"events.jsonl");if(events.find("PRIVATE")!=std::string::npos||events.find("ocr_complete")==std::string::npos)throw std::runtime_error("text opt-in broken");
 auto summary=read(folder/"summary.json");if(summary.find("\"samples\":100")==std::string::npos||summary.find("\"frame_rows\":100")==std::string::npos||summary.find("\"p95\":60")==std::string::npos)throw std::runtime_error("summary/flush failed");
 if(!d.start(root,"{}",false,true,0))throw std::runtime_error("restart");folder=d.folder();d.tick();if(!d.active())throw std::runtime_error("manual recording stopped");d.event("text",{},"a\"b\n\\c","český text");d.stop();while(d.busy())std::this_thread::sleep_for(std::chrono::milliseconds(2));
 if(read(folder/"events.jsonl").find("a\\\"b\\n\\\\c")==std::string::npos)throw std::runtime_error("JSON escaping failed");
 if(read(folder/"session.json").find("started_utc")==std::string::npos||read(folder/"events.jsonl").find("\"utc\":")==std::string::npos)throw std::runtime_error("timestamp missing");
 std::cout<<"PASS bounded async diagnostics, drain, summary, opt-in text and restart\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
