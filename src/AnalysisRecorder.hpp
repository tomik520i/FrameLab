#pragma once
#include "Graphics.hpp"
#include "LosslessRamBuffer.hpp"
#include "Diagnostics.hpp"
#include <map>
#include <set>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <array>
#include <cmath>
#include <sstream>

// Fifteen seconds buffered in RAM; disk writing begins only after capture ends. Output timeline references raw/generated pixels,
// avoiding duplicate readback for every Present. All timestamps use steady_clock.
struct AnalysisRecorder {
 std::string backendName="NVIDIA VFG";
 std::string outputScheduler="independent-gpu-pump-content-deadline";
 std::string pipelineSettings="{}";
 std::filesystem::path destination; // Empty = app/recordings; otherwise an existing user-selected folder.
 std::vector<std::string> outputRows,settingRows;
 std::set<std::pair<char,uint64_t>> recorded;
 uint64_t outputSerial=0;std::string lastOutputSignature;
 struct Item {char kind='R';unsigned w=0,h=0;uint64_t id=0,a=0,b=0;double time=0,t=0;std::vector<unsigned char> pixels;LosslessRamBuffer::Handle block;UINT presentId=0,refreshId=0,syncRefreshId=0;int64_t syncQpc=0;};
 std::atomic<bool> active=false,writing=false;std::atomic<uint64_t> dropped=0,rawCount=0,genCount=0,presentCount=0;
 std::mutex mutex;std::condition_variable cv;std::deque<Item> queue;size_t queueBudget=512ull*1024*1024;bool closing=false;
 LosslessRamBuffer ram;
 std::jthread worker;std::atomic<double> startTime=0,endTime=0;double stoppedTime=0;std::filesystem::path folder;std::string error,reason;
 struct Readback {ComPtr<ID3D11Texture2D> texture;Item item;bool pending=false;};
 std::array<Readback,16> readbacks;std::atomic<unsigned> pendingReadbacks=0,pendingRaw=0;unsigned stageW=0,stageH=0;
 void closeIfDrained(){std::lock_guard lock(mutex);if(!active&&(pendingReadbacks==0&&pendingRaw==0)){closing=true;cv.notify_one();}}
 void cancelPending(){for(auto& slot:readbacks)if(slot.pending){slot.pending=false;++dropped;--pendingReadbacks;}closeIfDrained();}
 bool capacity(size_t bytes){std::lock_guard lock(mutex);if(!active)return false;if(ram.canFit(bytes))return true;
  ++dropped;active=false;stoppedTime=std::min(clock(),endTime.load());reason="Recording stopped: recording buffer limit (RAM / free disk); live playback takes priority";if((pendingReadbacks==0&&pendingRaw==0))closing=true;cv.notify_one();return false;}

 static double clock(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
 ~AnalysisRecorder(){finish("Application closed");cancelPending();if(worker.joinable())worker.join();}
 void startError(const std::string& why){std::lock_guard lock(mutex);error="Could not start: "+why;}
 static std::string gib(uint64_t bytes){std::ostringstream s;s<<std::fixed<<std::setprecision(1)<<double(bytes)/(1024.*1024*1024);return s.str()+" GiB";}
 void start(const std::filesystem::path& root,const std::string& source,unsigned w,unsigned h,int quality,int multiplier,bool automaticCuts,bool vsync,bool dedup,float threshold,const std::string& gpu,bool fg,double captureFps,double contentFps=0){
  if(active||writing)return;if(worker.joinable())worker.join();
  const size_t frameBytes=size_t(w)*h*4;
  if(!frameBytes||frameBytes>LZ4_MAX_INPUT_SIZE||multiplier<1||multiplier>4)throw std::runtime_error("Invalid recording dimensions or multiplier");
  MEMORYSTATUSEX memory{sizeof(memory)};if(!GlobalMemoryStatusEx(&memory))throw std::runtime_error("Cannot query free RAM");
  const double rawRate=std::isfinite(captureFps)&&captureFps>0?std::clamp(captureFps,30.,1000.):60.;
  const double changedRate=std::isfinite(contentFps)&&contentFps>0?std::min(rawRate,contentFps):rawRate;
  const size_t expected=frameBytes*size_t(std::ceil((rawRate+(fg?changedRate*(multiplier-1):0))*15*1.15));
  const size_t reserve=12ull*1024*1024*1024;
  const size_t minimum=std::max(2ull*1024*1024*1024,frameBytes*16ull);
  if(memory.ullAvailPhys<reserve+minimum)throw std::runtime_error("Free RAM "+gib(memory.ullAvailPhys)+"; need at least "+gib(reserve+minimum)+" including system reserve. Close memory-heavy apps.");
  const auto disk=std::filesystem::space(destination.empty()?root:destination).available;
  if(disk<4ull*1024*1024*1024)throw std::runtime_error("At least 4 GiB free disk is required for recording and previews");
  // Stored blocks remain compressed on disk. Bound the worst case and reserve
  // equal space for playable exports; incompressible footage stops gracefully.
  queueBudget=std::min({size_t(memory.ullAvailPhys-reserve),size_t(16ull*1024*1024*1024),size_t((disk-2ull*1024*1024*1024)/2)});
  ram.reset(queueBudget,4);
  // Do not create a take or replace the last successful folder on preflight failure.
  auto nextFolder=(destination.empty()?root/L"recordings":destination)/("analysis-15s-"+std::to_string(GetTickCount64()));std::filesystem::create_directories(nextFolder);folder=nextFolder;
  std::ofstream extra(folder/L"capture-settings.json");extra<<"{\"recording_strategy\":\"LZ4 lossless RAM first, compressed FLP archive after capture; no BGRA files\",\"ram_compression_workers\":4,\"fg_enabled\":"<<(fg?"true":"false")<<",\"capture_fps_at_start\":"<<rawRate<<",\"content_fps_at_start\":"<<changedRate<<",\"estimated_uncompressed_bytes\":"<<expected<<",\"queue_budget_bytes\":"<<queueBudget<<",\"system_reserve_bytes\":"<<reserve<<",\"quality_name\":"<<std::quoted(quality==0?"Low":quality==1?"Medium":"High")<<",\"utc_epoch_ms\":"<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()<<"}\n";if(!extra)throw std::runtime_error("Cannot write capture settings");
  std::ofstream meta(folder/L"session.json");meta<<"{\"recording_id\":"<<std::quoted(folder.filename().string())<<",\"settings_id\":"<<std::quoted(folder.filename().string()+"_settings")<<",\"backend\":"<<std::quoted(backendName)<<",\"pacing_version\":3,\"output_scheduler\":"<<std::quoted(outputScheduler)<<",\"gpu\":"<<std::quoted(gpu)<<",\"vsync\":"<<(vsync?"true":"false")<<",\"dedup\":"<<(dedup?"true":"false")<<",\"dedup_threshold\":"<<threshold<<",\"requested_seconds\":15,\"source\":"<<std::quoted(source)<<",\"width\":"<<w<<",\"height\":"<<h<<",\"quality\":"<<quality<<",\"multiplier\":"<<multiplier<<",\"automatic_cut_detection\":"<<(automaticCuts?"true":"false")<<",\"pixel_format\":\"BGRA8 opaque, top-down\",\"hdmi_signal_resolution\":\"unknown; dimensions describe delivered capture frames\"}\n";if(!meta)throw std::runtime_error("Cannot write analysis metadata");
  {std::lock_guard lock(mutex);queue.clear();outputRows.clear();settingRows.clear();recorded.clear();outputSerial=0;lastOutputSignature.clear();closing=false;error.clear();reason.clear();}
  std::ofstream(folder/L"pipeline-settings.json")<<pipelineSettings;
  dropped=0;rawCount=0;genCount=0;presentCount=0;startTime=clock();endTime=startTime+15;stoppedTime=endTime;writing=true;active=true;
  worker=std::jthread([this]{
   SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
   try{
    {std::unique_lock lock(mutex);cv.wait(lock,[&]{return closing;});}
    ram.waitIdle();const auto storedBytes=ram.used(),rawBytes=ram.rawTotal(),peakBytes=ram.peak();
    if(std::filesystem::space(folder).available<storedBytes+1024ull*1024*1024)throw std::runtime_error("Not enough disk to save compressed recording with 1 GiB reserve");
    std::map<char,std::ofstream> files;std::map<char,uint64_t> offsets;
    for(char k:std::string("RGBEF")){auto& file=files[k];file.open(folder/(std::string(1,k)+".flp"),std::ios::binary);file.exceptions(std::ios::badbit|std::ios::failbit);}
    std::ofstream frames(folder/L"frames.csv"),presents(folder/L"presents.csv"),confirmed(folder/L"display-confirmations.csv");
    frames.exceptions(std::ios::badbit|std::ios::failbit);presents.exceptions(std::ios::badbit|std::ios::failbit);
    frames<<"kind,id,source_A,source_B,time_s,timestep,width,height,byte_offset,byte_count,stored_bytes\n"<<std::fixed<<std::setprecision(9);
    presents<<"present_time_s,kind,image_id,source_A,source_B,timestep\n"<<std::fixed<<std::setprecision(9);
    confirmed.exceptions(std::ios::badbit|std::ios::failbit);confirmed<<"observed_time_s,kind,image_id,dxgi_present_id,present_refresh_count,sync_refresh_count,sync_qpc\n"<<std::fixed<<std::setprecision(9);
    for(;;){Item item;{std::unique_lock lock(mutex);cv.wait(lock,[&]{return closing||!queue.empty();});if(queue.empty()&&closing)break;item=std::move(queue.front());queue.pop_front();}
     if(item.kind=='r'||item.kind=='g'){presents<<item.time-startTime<<','<<item.kind<<','<<item.id<<','<<item.a<<','<<item.b<<','<<item.t<<'\n';continue;}
     if(item.kind=='u'||item.kind=='v'){confirmed<<item.time-startTime<<','<<(item.kind=='v'?'g':'r')<<','<<item.id<<','<<item.presentId<<','<<item.refreshId<<','<<item.syncRefreshId<<','<<item.syncQpc<<'\n';continue;}
     auto& file=files.at(item.kind);auto& offset=offsets[item.kind];const auto& block=*item.block;
     const uint32_t header[]={0x31504c46,uint32_t(block.rawBytes),uint32_t(block.bytes.size()),uint32_t(block.compressed)|(uint32_t(block.rgbDelta)<<1),block.alpha};
     if(std::filesystem::space(folder).available<block.bytes.size()+1024ull*1024*1024)throw std::runtime_error("Recording save stopped: low disk space");
     file.write(reinterpret_cast<const char*>(header),sizeof(header));file.write(reinterpret_cast<const char*>(block.bytes.data()),block.bytes.size());
     const auto packed=sizeof(header)+block.bytes.size();
     frames<<item.kind<<','<<item.id<<','<<item.a<<','<<item.b<<','<<item.time-startTime<<','<<item.t<<','<<item.w<<','<<item.h<<','<<offset<<','<<block.rawBytes<<','<<packed<<'\n';offset+=packed;ram.release(item.block);
    }
    for(auto& [kind,file]:files)file.close();frames.close();presents.close();confirmed.close();
    {std::ofstream out(folder/L"output-frames.jsonl");out.exceptions(std::ios::badbit|std::ios::failbit);for(auto& row:outputRows)out<<row<<'\n';}
    {std::ofstream out(folder/L"settings-timeline.jsonl");out.exceptions(std::ios::badbit|std::ios::failbit);for(auto& row:settingRows)out<<row<<'\n';}
    std::ofstream done(folder/L"result.json");done<<"{\"duration_seconds\":"<<stoppedTime-startTime<<",\"raw_frames\":"<<rawCount<<",\"generated_frames\":"<<genCount<<",\"present_events\":"<<presentCount<<",\"recorder_drops\":"<<dropped<<",\"ram_raw_bytes\":"<<rawBytes<<",\"ram_stored_bytes\":"<<storedBytes<<",\"ram_peak_bytes\":"<<peakBytes<<",\"reason\":"<<std::quoted(reason)<<",\"note\":\"Lossless RAM compression preserves every BGRA byte. Diagnostic readback/compression may affect performance. Presents reference image IDs; pre-start images may be absent. Matched output before/after enhancement and burned Czech captions are recorded in source resolution. Subtitle timing is in output-frames.jsonl. Audio, UI chrome and physical monitor scanout are not recorded.\"}\n";if(!done)throw std::runtime_error("Cannot finalize analysis metadata");
   }catch(const std::exception& e){std::lock_guard lock(mutex);error=e.what();queue.clear();active=false;}
   writing=false;
  });
 }
 void finish(const std::string& why="15 seconds completed"){
  std::lock_guard lock(mutex);if(!active.exchange(false))return;stoppedTime=std::min(clock(),endTime.load());reason=why;closing=(pendingReadbacks==0&&pendingRaw==0);cv.notify_one();
 }
 void tick(){if(active&&clock()>=endTime)finish();}
 void poll(Graphics& g){
  for(auto& slot:readbacks){if(!slot.pending)continue;
   D3D11_MAPPED_SUBRESOURCE map{};auto hr=g.context->Map(slot.texture.Get(),0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&map);
   // A late CPU poll does not mean the GPU copy failed. Map first: a ready
   // frame must survive an OCR/model-loading or scheduling pause.
   if(hr==DXGI_ERROR_WAS_STILL_DRAWING){if(clock()-slot.item.time>2.){slot.pending=false;--pendingReadbacks;++dropped;finish("Recording stopped: GPU copy still pending after 2 seconds");}continue;}
   if(FAILED(hr)){slot.pending=false;--pendingReadbacks;++dropped;finish("Recording stopped: GPU readback error");continue;}
   const size_t bytes=size_t(slot.item.w)*slot.item.h*4;
   slot.item.pixels.resize(bytes);for(unsigned y=0;y<slot.item.h;++y)memcpy(slot.item.pixels.data()+size_t(y)*slot.item.w*4,(char*)map.pData+size_t(y)*map.RowPitch,slot.item.w*4);
   g.context->Unmap(slot.texture.Get(),0);slot.pending=false;--pendingReadbacks;push(std::move(slot.item));
  }closeIfDrained();
 }

 void push(Item item){std::lock_guard lock(mutex);if((!active&&item.pixels.empty())||!writing||closing||item.time<startTime||item.time>=endTime)return;
  if(!item.pixels.empty())item.block=ram.submit(std::move(item.pixels));
  if((std::string("RGBEF").find(item.kind)!=std::string::npos&&!item.block)||queue.size()>20000){++dropped;active=false;stoppedTime=std::min(clock(),endTime.load());reason="Recording stopped: recording buffer limit (RAM / free disk); choose more space or reduce capture resolution";if((pendingReadbacks==0&&pendingRaw==0))closing=true;cv.notify_one();return;}if(item.kind=='R')++rawCount;else if(item.kind=='G')++genCount;else if(item.kind=='r'||item.kind=='g')++presentCount;if(item.block)recorded.insert({item.kind,item.id});queue.push_back(std::move(item));cv.notify_one();
 }
 void raw(const unsigned char* pixels,unsigned w,unsigned h,uint64_t id,double time){
  {std::lock_guard lock(mutex);if(!active||time<startTime||time>=endTime)return;++pendingRaw;}
  if(capacity(size_t(w)*h*4)){Item item;item.w=w;item.h=h;item.id=id;item.a=item.b=id;item.time=time;item.pixels.assign(pixels,pixels+size_t(w)*h*4);push(std::move(item));}
  --pendingRaw;closeIfDrained();
 }
 void generated(Graphics& g,Texture& texture,unsigned w,unsigned h,uint64_t id,uint64_t a,uint64_t b,double position){
  image(g,texture.texture.Get(),'G',w,h,id,a,b,position,clock());
 }
 bool has(char kind,uint64_t id){std::lock_guard lock(mutex);return recorded.contains({kind,id});}
 void image(Graphics& g,ID3D11Texture2D* texture,char kind,unsigned w,unsigned h,uint64_t id,uint64_t a,uint64_t b,double position,double time){
  if(!active||!capacity(size_t(w)*h*4))return;
  Readback* slot=nullptr;for(auto& candidate:readbacks)if(!candidate.pending){slot=&candidate;break;}
  if(!slot){++dropped;finish("Recording stopped: GPU readback RAM buffer full");return;}
  if(stageW!=w||stageH!=h){cancelPending();for(auto& candidate:readbacks)candidate.texture.Reset();stageW=w;stageH=h;}
  if(!slot->texture){D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;check(g.device->CreateTexture2D(&desc,nullptr,&slot->texture),"Analysis staging");}
  slot->item=Item{};slot->item.kind=kind;slot->item.w=w;slot->item.h=h;slot->item.id=id;slot->item.a=a;slot->item.b=b;slot->item.t=position;slot->item.time=time;
  g.context->CopyResource(slot->texture.Get(),texture);slot->pending=true;++pendingReadbacks;
 }
 void output(double time,const std::string& values){std::lock_guard lock(mutex);if(active&&time<endTime)outputRows.push_back("{\"time_s\":"+Diagnostics::number(time-startTime)+","+values+"}");}
 void settings(const std::string& json){std::lock_guard lock(mutex);if(active)settingRows.push_back("{\"time_s\":"+Diagnostics::number(clock()-startTime)+",\"settings\":"+json+"}");}
 void presented(bool generated,uint64_t id,uint64_t a,uint64_t b,double position){if(!active)return;Item item;item.kind=generated?'g':'r';item.id=id;item.a=a;item.b=b;item.t=position;item.time=clock();push(std::move(item));}
 void confirmed(bool generated,uint64_t id,UINT present,UINT refresh,UINT syncRefresh,int64_t qpc){if(!active)return;Item item;item.kind=generated?'v':'u';item.id=id;item.presentId=present;item.refreshId=refresh;item.syncRefreshId=syncRefresh;item.syncQpc=qpc;item.time=clock();push(std::move(item));}
 std::string message(){std::lock_guard lock(mutex);if(!error.empty())return "Recording error: "+error;if(active)return "Recording to lossless RAM: "+std::to_string(std::max(0.,endTime-clock())).substr(0,4)+" s left | "+gib(ram.used())+" / "+gib(queueBudget);if(writing)return "Recording finished. Saving lossless RAM to disk: "+gib(ram.used())+" remaining";if(!folder.empty())return (dropped.load()?"INCOMPLETE: ":"Saved: ")+folder.filename().string()+" | dropped "+std::to_string(dropped.load())+" | "+reason;return "15 s: RAW / FG + before / after enhancer + subtitles (compressed)";}
};
