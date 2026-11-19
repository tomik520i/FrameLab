#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <ctime>

// Bounded, asynchronous metadata recording. Never captures pixels or audio.
// Producers only enqueue; slow disks lose diagnostic rows, not video frames.
class Diagnostics {
public:
 using Values=std::vector<std::pair<std::string,double>>;
 static double clock(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
 static std::string quote(const std::string& s){std::ostringstream out;out<<'"';for(unsigned char c:s){switch(c){case '"':out<<"\\\"";break;case '\\':out<<"\\\\";break;case '\n':out<<"\\n";break;case '\r':out<<"\\r";break;case '\t':out<<"\\t";break;default:if(c<32)out<<"\\u00"<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c)<<std::dec;else out<<char(c);}}return out.str()+'"';}
 static std::string utcNow(){auto now=std::chrono::system_clock::now();auto seconds=std::chrono::system_clock::to_time_t(now);std::tm tm{};gmtime_s(&tm,&seconds);auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()%1000;std::ostringstream out;out<<std::put_time(&tm,"%Y-%m-%dT%H:%M:%S")<<'.'<<std::setw(3)<<std::setfill('0')<<ms<<'Z';return out.str();}
 static std::string number(double v){if(!std::isfinite(v))return "null";std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(10)<<v;return out.str();}
 static std::string object(const Values& values){std::string out="{";for(auto& [key,v]:values){if(out.size()>1)out+=',';out+=quote(key)+':'+number(v);}return out+'}';}
 ~Diagnostics(){stop();if(writer.joinable())writer.join();}
 bool active()const{return recording.load();}
 bool busy()const{return writing.load();}
 bool texts()const{return active()&&includeText;}
 bool frames()const{return active()&&includeFrames;}
 uint64_t dropped()const{return lost.load();}
 uint64_t bytes()const{return written.load();}
 double elapsed()const{return active()?clock()-started:duration.load();}
 std::filesystem::path folder()const{std::lock_guard lock(mutex);return path;}
 std::string status()const{std::lock_guard lock(mutex);return message;}
 bool start(const std::filesystem::path& root,const std::string& metadata,bool frameDetail,bool textDetail,double seconds){
  if(busy())return false;if(writer.joinable())writer.join();
  auto id=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  {std::lock_guard lock(mutex);path=root/("debug-"+std::to_string(id));queue.clear();queuedBytes=0;message="Recording";}
  includeFrames=frameDetail;includeText=textDetail;limit=seconds<=0?0:std::clamp(seconds,5.,900.);started=clock();startedUtc=utcNow();duration=0;lost=written=0;writing=recording=true;
  writer=std::thread([this,metadata]{run(metadata);});return true;
 }
 void stop(){recording=false;wake.notify_all();}
 void tick(){if(active()&&limit>0&&clock()-started>=limit)stop();}
 void sample(const Values& values){if(active())enqueue({0,clock()-started,{},values});}
 void frame(const Values& values){if(frames())enqueue({1,clock()-started,{},values});}
 void event(const std::string& kind,const Values& values={},const std::string& source={},const std::string& translated={}){
  if(!active())return;
  auto fields=object(values);fields.pop_back();
  std::string line="{\"utc\":"+quote(utcNow())+",\"time_s\":"+number(clock()-started)+",\"event\":"+quote(kind);
  if(fields.size()>1)line+=','+fields.substr(1);
  if(includeText){if(!source.empty())line+=",\"source\":"+quote(source);if(!translated.empty())line+=",\"translation\":"+quote(translated);}
  enqueue({2,0,line+'}',{}});
 }
 void settings(const std::string& json){if(active())enqueue({2,0,"{\"utc\":"+quote(utcNow())+",\"time_s\":"+number(clock()-started)+",\"event\":\"settings\",\"settings\":"+json+"}",{}});}
private:
 struct Item{int kind;double time;std::string line;Values values;std::string utc=utcNow();};
 struct Series{uint64_t count=0;double sum=0,maximum=0;std::vector<double> values;void add(double v){++count;sum+=v;maximum=std::max(maximum,v);if(values.size()<4096)values.push_back(v);else{uint64_t hash=count+0x9e3779b97f4a7c15ull;hash=(hash^(hash>>30))*0xbf58476d1ce4e5b9ull;hash=(hash^(hash>>27))*0x94d049bb133111ebull;hash^=hash>>31;auto slot=hash%count;if(slot<values.size())values[size_t(slot)]=v;}}};
 std::atomic<bool> recording=false,writing=false;std::atomic<bool> includeText=false,includeFrames=false;
 std::atomic<uint64_t> lost=0,written=0;std::atomic<double> duration=0;
 std::atomic<double> started=0;double limit=60;mutable std::mutex mutex;std::condition_variable wake;std::deque<Item> queue;size_t queuedBytes=0;
 std::filesystem::path path;std::string message,startedUtc;std::thread writer;
 static size_t cost(const Item& i){size_t n=i.line.size()+64;for(auto& [k,v]:i.values)n+=k.size()+32;return n;}
 void enqueue(Item item){std::lock_guard lock(mutex);if(!recording)return;auto n=cost(item);if(queue.size()>=4096||queuedBytes+n>4*1024*1024){++lost;return;}queuedBytes+=n;queue.push_back(std::move(item));wake.notify_one();}
 void run(const std::string& metadata){
  try{
   std::filesystem::create_directories(path);
   std::ofstream meta(path/"session.json");meta<<"{\"schema\":1,\"session_id\":"<<quote(path.filename().string())<<",\"started_utc\":"<<quote(startedUtc)<<",\"frames\":"<<includeFrames<<",\"subtitle_text\":"<<includeText<<",\"duration_limit_s\":"<<limit<<",\"metadata\":"<<metadata<<"}\n";meta.close();if(!meta)throw std::runtime_error("Cannot write diagnostic session");
   std::ofstream metrics(path/"metrics.csv"),framesFile(path/"frames.csv"),events(path/"events.jsonl");
   if(!metrics||!framesFile||!events)throw std::runtime_error("Cannot open diagnostic logs");
   bool headers[2]={false,false};std::map<std::string,Series> summary;uint64_t fileBytes[3]={};unsigned parts[3]={};uint64_t rows=0,frameRows=0;double flushed=clock();
   for(;;){if(recording&&limit>0&&clock()-started>=limit)recording=false;Item item;bool have=false;{
    std::unique_lock lock(mutex);wake.wait_for(lock,std::chrono::milliseconds(500),[&]{return !queue.empty()||!recording;});
    if(!queue.empty()){item=std::move(queue.front());queue.pop_front();queuedBytes-=cost(item);have=true;}else if(!recording)break;
   }
    if(have){std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(9);
     auto& file=item.kind==0?metrics:item.kind==1?framesFile:events;
     if(fileBytes[item.kind]>=64ull*1024*1024){file.close();++parts[item.kind];const char* names[]={"metrics","frames","events"};file.open(path/(std::string(names[item.kind])+"-"+std::to_string(parts[item.kind])+(item.kind==2?".jsonl":".csv")));fileBytes[item.kind]=0;if(item.kind<2)headers[item.kind]=false;if(!file)throw std::runtime_error("Cannot rotate diagnostic log");}
     if(item.kind<2){if(!headers[item.kind]){out<<"utc,time_s";for(auto& [key,v]:item.values)out<<','<<key;out<<'\n';headers[item.kind]=true;}
      out<<item.utc<<','<<item.time;for(auto& [key,v]:item.values){out<<','<<number(v);if(std::isfinite(v)&&v>=0&&(item.kind==0||(key=="content_interval_ms"&&v>0)||key=="present_cpu_ms")){summary[(item.kind==1?"frame_":"")+key].add(v);}}out<<'\n';
      if(item.kind==0)++rows;else ++frameRows;
     }else out<<item.line<<'\n';
     const auto line=out.str();file<<line;written+=line.size();fileBytes[item.kind]+=line.size();if(!file)throw std::runtime_error("Diagnostic write failed (disk full?)");
    }
    if(clock()-flushed>=1){metrics.flush();framesFile.flush();events.flush();if(!metrics||!framesFile||!events)throw std::runtime_error("Diagnostic flush failed");std::error_code ec;auto free=std::filesystem::space(path,ec);if(!ec&&free.available<256ull*1024*1024){recording=false;std::lock_guard lock(mutex);message="Stopped: less than 256 MiB disk space";}flushed=clock();}
   }
   duration=clock()-started;std::ofstream result(path/"summary.json");result<<"{\"started_utc\":"<<quote(startedUtc)<<",\"finished_utc\":"<<quote(utcNow())<<",\"percentiles\":\"Exact up to 4096 values, uniform reservoir estimate thereafter\",\"duration_s\":"<<duration.load()<<",\"samples\":"<<rows<<",\"frame_rows\":"<<frameRows<<",\"dropped_log_rows\":"<<lost<<",\"metrics\":{";bool first=true;
   for(auto& [key,series]:summary){auto& v=series.values;if(v.empty())continue;std::sort(v.begin(),v.end());double sum=0;for(auto x:v)sum+=x;auto percentile=[&](double p){return v[std::min(v.size()-1,size_t(std::ceil(v.size()*p)-1))];};if(!first)result<<',';first=false;result<<quote(key)<<":{\"count\":"<<series.count<<",\"mean\":"<<series.sum/series.count<<",\"p50\":"<<percentile(.5)<<",\"p95\":"<<percentile(.95)<<",\"max\":"<<series.maximum<<'}';}result<<"}}\n";result.close();if(!result)throw std::runtime_error("Cannot write diagnostic summary");
   std::ofstream readme(path/"README.txt");readme<<"FrameLab local diagnostics\nManual recording runs until Stop (F9), app exit, write failure or disk space below 256 MiB. Files rotate at 64 MiB; none are deleted automatically.\nUTC timestamps use ISO 8601; time_s is monotonic seconds since recording start. All files share the debug folder/session ID.\nSummary percentiles use a bounded 4096-value reservoir after the first 4096 values; mean/max/count cover every recorded value.\nmetrics.csv: one sample per second; -1 = unavailable.\nframes.csv: Present calls (optional); content_interval_ms only for a new image.\nevents.jsonl: OCR, translation, NR stages and setting changes. Subtitle text is opt-in.\nsummary.json: descriptive statistics, including warmup. Counters are cumulative, not rates.\nsession.json: initial settings and measurement definitions. No video, audio or desktop capture.\nGPU/CPU system load includes other applications. App CPU/RAM excludes helper processes; use system totals alongside stage timings.\nApp latency excludes capture hardware and monitor scanout. OS confirmations are a lower bound, not physical display FPS.\n";
   std::lock_guard lock(mutex);if(message=="Recording")message="Saved";
  }catch(const std::exception& e){std::lock_guard lock(mutex);message=std::string("Error: ")+e.what();}
  duration=clock()-started;recording=false;writing=false;
 }
};
