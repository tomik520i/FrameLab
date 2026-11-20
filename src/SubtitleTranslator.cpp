#include "SubtitleTranslator.hpp"
#include "Diagnostics.hpp"
#include "SubtitleState.hpp"
#include "SubtitleSentence.hpp"
#include <Windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.Streams.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <deque>
#include <sstream>

namespace {
double clockSeconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
struct Handle {HANDLE value=nullptr;~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}operator HANDLE()const{return value;}};
struct TranslationProcess {
 Handle process,input,output,errors,job;
 ~TranslationProcess(){if(process&&WaitForSingleObject(process,0)==WAIT_TIMEOUT){TerminateProcess(process,0);WaitForSingleObject(process,2000);}}
 void launch(const std::filesystem::path& app,int model,const std::wstring& mapping=L""){
  std::filesystem::create_directories(app/L"logs");
  const bool ocr=!mapping.empty();const auto base=app/(ocr?L"ocr":L"translation"),python=app/L"translation"/L"python"/L"python.exe",script=base/(ocr?L"ocr_worker.py":L"translate_worker.py");
  if(!std::filesystem::is_regular_file(python)||!std::filesystem::is_regular_file(script)||(!ocr&&!std::filesystem::is_regular_file(base/L"model"/L"model.bin")))throw std::runtime_error("Local OCR / translation files missing. Extract the complete package.");
  SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};Handle childInput,childOutput;
  if(!CreatePipe(&childInput.value,&input.value,&sa,4096)||!CreatePipe(&output.value,&childOutput.value,&sa,4096))throw std::runtime_error("Subtitle pipe creation failed");
  SetHandleInformation(input,HANDLE_FLAG_INHERIT,0);SetHandleInformation(output,HANDLE_FLAG_INHERIT,0);
  errors.value=CreateFileW((app/L"logs"/(ocr?L"ocr-worker.log":L"translation-worker.log")).c_str(),GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;si.hStdInput=childInput;si.hStdOutput=childOutput;si.hStdError=errors;
  PROCESS_INFORMATION pi{};std::wstring command=L"\""+python.wstring()+L"\" -B -I \""+script.wstring()+L"\" ";
  command+=ocr?L"--mapping \""+mapping+L"\" --style-protocol":L"--model \""+(base/L"model").wstring()+L"\" --backend "+(model==1?L"cuni-q3km":model==2?L"cuni-q8":model==3?L"cuni-q5km":L"cpu");
  job.value=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if(!job||!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))throw std::runtime_error("Subtitle process isolation failed");
  if(!CreateProcessW(python.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,base.c_str(),&si,&pi))throw std::runtime_error("Local translation process failed to start");
  process.value=pi.hProcess;
  if(!AssignProcessToJobObject(job,process)){TerminateProcess(process,1);CloseHandle(pi.hThread);throw std::runtime_error("Subtitle child process isolation failed");}
  ResumeThread(pi.hThread);CloseHandle(pi.hThread);
 }
 void readExact(void* data,DWORD length,const std::function<bool()>& cancelled,double timeout){
  auto p=static_cast<unsigned char*>(data);const double deadline=clockSeconds()+timeout;
  while(length){if(cancelled()||clockSeconds()>deadline)throw std::runtime_error("Translation cancelled or timed out");DWORD available=0;if(!PeekNamedPipe(output,nullptr,0,nullptr,&available,nullptr))throw std::runtime_error("Translation process stopped; see translation-worker.log");if(!available){Sleep(5);continue;}DWORD got=0;if(!ReadFile(output,p,std::min(length,available),&got,nullptr)||!got)throw std::runtime_error("Translation response failed");p+=got;length-=got;}
 }
 std::string receive(const std::function<bool()>& cancelled,double timeout=30){uint32_t length=0;readExact(&length,4,cancelled,timeout);if(length>65536)throw std::runtime_error("Invalid subtitle response length");std::string value(length,'\0');if(length)readExact(value.data(),length,cancelled,timeout);return value;}
 std::string translate(const std::string& text,const std::function<bool()>& cancelled){
  uint32_t length=uint32_t(text.size());std::vector<unsigned char> message(length+4);memcpy(message.data(),&length,4);memcpy(message.data()+4,text.data(),length);DWORD sent=0;
  if(!WriteFile(input,message.data(),DWORD(message.size()),&sent,nullptr)||sent!=message.size())throw std::runtime_error("Translation request failed");
  auto result=receive(cancelled);if(result.starts_with("ERROR:"))throw std::runtime_error(result);return result;
 }
};
struct OcrProcess:TranslationProcess {
 static constexpr size_t capacity=16*1024*1024;
 Handle mapping;void* pixels=nullptr;
 ~OcrProcess(){if(pixels)UnmapViewOfFile(pixels);}
 void start(const std::filesystem::path& app,const std::function<bool()>& cancelled){
  static std::atomic<unsigned> serial=0;
  const auto name=L"Local\\FrameLab-OCR-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(++serial);
  mapping.value=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,DWORD(capacity),name.c_str());
  if(!mapping)throw std::runtime_error("OCR shared memory unavailable");
  pixels=MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,capacity);if(!pixels)throw std::runtime_error("OCR shared memory mapping failed");
  launch(app,0,name);const auto ready=receive(cancelled,60);if(ready!="READY")throw std::runtime_error(ready);
 }
 struct Reading {std::string text;float confidence=0;SubtitleStyleProfile profile;std::vector<SubtitleOcrLine> lines;};
 Reading recognize(const std::vector<unsigned char>& data,unsigned w,unsigned h,const std::string& config,const std::function<bool()>& cancelled){
  if(data.size()>capacity)throw std::runtime_error("OCR crop too large");
  memcpy(pixels,data.data(),data.size());uint32_t header[]{w,h,uint32_t(config.size())};std::vector<unsigned char> request(sizeof(header)+config.size());memcpy(request.data(),header,sizeof(header));memcpy(request.data()+sizeof(header),config.data(),config.size());DWORD sent=0;
  if(!WriteFile(input,request.data(),DWORD(request.size()),&sent,nullptr)||sent!=request.size())throw std::runtime_error("OCR request failed");
  const auto value=receive(cancelled,15);if(value.starts_with("ERROR:"))throw std::runtime_error(value);
  std::istringstream stream(value);stream.imbue(std::locale::classic());std::string headerLine,magic;std::getline(stream,headerLine);std::istringstream fields(headerLine);fields.imbue(std::locale::classic());Reading out;unsigned count=0;
  if(!(fields>>magic>>out.confidence>>out.profile.enabled>>out.profile.height>>out.profile.rgb[0]>>out.profile.rgb[1]>>out.profile.rgb[2]>>count)||(magic!="STYLE2"&&magic!="STYLE3")||count>64)throw std::runtime_error("Invalid OCR style response; update bundled OCR files");
  std::getline(stream,out.text);
  if(magic=="STYLE3")std::getline(stream,out.profile.sampleText);
  for(unsigned i=0;i<count;++i){std::string line;if(!std::getline(stream,line))throw std::runtime_error("Incomplete OCR style response");const auto tab=line.find('\t');if(tab==std::string::npos)throw std::runtime_error("Invalid OCR line");std::istringstream f(line.substr(0,tab));f.imbue(std::locale::classic());SubtitleOcrLine r;if(!(f>>r.rect[0]>>r.rect[1]>>r.rect[2]>>r.rect[3]>>r.confidence>>r.reason))throw std::runtime_error("Invalid OCR geometry");
   if(magic=="STYLE3"&&!(f>>r.measured>>r.height>>r.rgb[0]>>r.rgb[1]>>r.rgb[2]>>r.boxHeight))throw std::runtime_error("Invalid OCR measurements");
   if(!std::isfinite(r.height)||!std::isfinite(r.boxHeight)||!std::isfinite(r.confidence)||r.height<0||r.boxHeight<0)throw std::runtime_error("Invalid OCR measurement values");
   for(auto& c:r.rgb)if(!std::isfinite(c)||c<0||c>255)throw std::runtime_error("Invalid OCR colour");
   r.text=line.substr(tab+1);out.lines.push_back(std::move(r));}
  return out;
 }
};
}
struct SubtitleTranslator::Impl {
 struct Job{std::vector<unsigned char> pixels;unsigned w=0,h=0,sourceHeight=0;double time=0;uint64_t generation=0,id=0;int engine=0;float fullHeight=0;SubtitleStyleProfile style;bool preview=false,inspect=false;std::array<float,4> word{},area{0,0,1,1};};
 struct TextJob{std::string source;uint64_t generation=0;float ocrMs=0;double time=0,queuedAt=0;uint64_t id=0;};
 struct Caption{std::string source,translated;float ms=0;double time=0;std::string observedSource;uint64_t id=0;};
 std::filesystem::path app;mutable std::mutex mutex;std::condition_variable wake,translateWake;std::atomic<bool> stop=false;
 std::atomic<uint64_t> modelEpoch=1;
 std::atomic<uint64_t> ocrEpoch=1;
 std::atomic<uint64_t> sampleEpoch=1;
 std::atomic<int> activeModel=-1;
 SubtitlePerformance perf;std::atomic<Diagnostics*> diagnostics=nullptr;uint64_t nextJob=0;
 void event(const char* name,Diagnostics::Values fields={},const std::string& source={},const std::string& translated={}){if(auto d=diagnostics.load())d->event(name,fields,source,translated);}
 SubtitleSettings settings;SubtitleSnapshot result;Job pending;std::deque<TextJob> texts;std::deque<Caption> captions;
 Job samplePending;bool sampleAvailable=false;SubtitleStyleSample sampleResult;
 bool available=false,loading=false;uint64_t generation=1;double lastSubmitted=0;
 std::string modelError,ocrError,displayReading;std::thread worker,translator;
 explicit Impl(std::filesystem::path p):app(std::move(p)),worker([this]{runOcr();}),translator([this]{runTranslation();}){}
 ~Impl(){stop=true;wake.notify_all();translateWake.notify_all();worker.join();translator.join();}
 void clearLocked(){perf.ocrSuperseded+=available?1:0;perf.translationDrops+=texts.size();perf.captionDrops+=captions.size();event("subtitle_reset",{{"generation",double(generation+1)}});++generation;pending={};texts.clear();captions.clear();available=false;lastSubmitted=0;result={};result.generation=generation;ocrError.clear();displayReading.clear();++sampleEpoch;samplePending={};sampleAvailable=false;sampleResult={};}
 void displayLocked(double now,bool paused){
  // Never build an unlimited delay: retain at most three pending lines and
  // expire work older than six seconds. Source/seek/model resets still flush all.
  while(!captions.empty()&&now-captions.front().time>6){++perf.captionDrops;event("caption_expired",{{"job_id",double(captions.front().id)}});captions.pop_front();}
  if(!captions.empty()&&(result.translated.empty()||(!paused&&now-result.displayedAt>=settings.holdSeconds))){
   auto caption=std::move(captions.front());captions.pop_front();
   ++perf.shown;perf.displayLatencyMs=(now-caption.time)*1000;event("caption_display",{{"job_id",double(caption.id)},{"capture_to_display_ms",perf.displayLatencyMs}},caption.source,caption.translated);
   result.displaySource=std::move(caption.source);result.translated=std::move(caption.translated);result.milliseconds=caption.ms;result.displayedAt=now;displayReading=std::move(caption.observedSource);
   size_t letters=0;for(unsigned char c:result.translated)if((c&0xc0)!=0x80)++letters;
   result.expiresAt=now+std::max(double(settings.lingerSeconds),std::min(7.5,letters/25.0));
  }
  if(!paused&&!result.translated.empty()&&now>=result.expiresAt){result.translated.clear();result.displaySource.clear();}
  result.pending=int(texts.size()+captions.size());
 }
 void runOcr(){
  winrt::init_apartment(winrt::apartment_type::multi_threaded);
  winrt::Windows::Media::Ocr::OcrEngine engine{nullptr};std::unique_ptr<OcrProcess> neural;SubtitleState state;uint64_t seenGeneration=0,failedGeneration=0,seenEpoch=0;
  while(!stop){
   Job job;uint64_t epoch;{std::unique_lock lock(mutex);wake.wait(lock,[&]{return stop||available||sampleAvailable||ocrEpoch!=seenEpoch;});if(stop)break;epoch=ocrEpoch;if(epoch!=seenEpoch){neural.reset();seenEpoch=epoch;}if(sampleAvailable){job=std::move(samplePending);sampleAvailable=false;}else{if(!available)continue;job=std::move(pending);available=false;perf.ocrBusy=true;perf.ocrQueueMs=(clockSeconds()-job.time)*1000;}}
   if(!job.preview&&failedGeneration==job.generation){std::lock_guard lock(mutex);perf.ocrBusy=false;++perf.ocrSuperseded;continue;}
   try{
    const auto started=clockSeconds();
    if(seenGeneration!=job.generation){seenGeneration=job.generation;state.clear();}
    std::string source;float confidence=0;std::vector<SubtitleOcrLine> lines;
    if(job.engine==0){
     const auto cancelled=[&]{return stop||ocrEpoch!=epoch||(job.preview&&sampleEpoch!=job.id);};
     if(!neural){neural=std::make_unique<OcrProcess>();neural->start(app,cancelled);}
     auto n=[](float v){return Diagnostics::number(v);};
     const auto& s=job.style;std::string config="{\"full_height\":"+n(job.fullHeight)+",\"profile\":{\"enabled\":"+(s.enabled?std::string("true"):std::string("false"))+",\"height\":"+n(s.height)+",\"rgb\":["+n(s.rgb[0])+","+n(s.rgb[1])+","+n(s.rgb[2])+"],\"height_tolerance\":"+n(s.heightTolerance)+",\"color_tolerance\":"+n(s.colorTolerance)+"}";
     if(job.preview){config+=",\"sample\":["+n(job.word[0])+","+n(job.word[1])+","+n(job.word[2])+","+n(job.word[3])+"],\"area\":["+n(job.area[0])+","+n(job.area[1])+","+n(job.area[2])+","+n(job.area[3])+"]";}
     config+=",\"inspect\":"+std::string(job.inspect?"true":"false")+'}';auto reading=neural->recognize(job.pixels,job.w,job.h,config,cancelled);
     if(job.preview)reading.profile.sampleFrameHeight=job.sourceHeight;
     if(job.preview){std::lock_guard lock(mutex);if(sampleEpoch==job.id){sampleResult={job.id,false,reading.profile,std::move(reading.lines),reading.text,{},float((clockSeconds()-started)*1000)};event("ocr_style_sample",{{"sample_id",double(job.id)},{"height_ratio",reading.profile.height},{"r",reading.profile.rgb[0]},{"g",reading.profile.rgb[1]},{"b",reading.profile.rgb[2]}},reading.text);}continue;}
     source=SubtitleState::normalize(reading.text);confidence=reading.confidence;lines=std::move(reading.lines);
    }else{
     if(!engine){engine=winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(winrt::Windows::Globalization::Language(L"en-US"));if(!engine)throw std::runtime_error("Windows English OCR unavailable. Select bundled RapidOCR or install English OCR.");}
     winrt::Windows::Storage::Streams::DataWriter writer;writer.WriteBytes(job.pixels);
     auto bitmap=winrt::Windows::Graphics::Imaging::SoftwareBitmap::CreateCopyFromBuffer(writer.DetachBuffer(),winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,int(job.w),int(job.h),winrt::Windows::Graphics::Imaging::BitmapAlphaMode::Ignore);
     auto recognized=engine.RecognizeAsync(bitmap).get();source=SubtitleState::normalize(winrt::to_string(recognized.Text()));bitmap.Close();writer.Close();
    }
    const bool translate=state.observe(source,confidence);
    std::lock_guard lock(mutex);perf.ocrBusy=false;++perf.ocrCompleted;if(generation!=job.generation)continue;
    result.source=source;result.observedAt=job.time;result.generation=job.generation;
    result.ocrLines=std::move(lines);
    result.sourceHeight=job.sourceHeight;result.inspected=job.inspect;
    if(job.style.enabled||job.inspect)for(const auto& r:result.ocrLines)event("ocr_style_line",{{"job_id",double(job.id)},{"reason",double(r.reason)},{"left",r.rect[0]},{"top",r.rect[1]},{"right",r.rect[2]},{"bottom",r.rect[3]},{"confidence",r.confidence},{"measured",double(r.measured)},{"glyph_height_ratio",r.height},{"glyph_height_source_px",r.height*job.sourceHeight},{"box_height_source_px",r.boxHeight*job.sourceHeight},{"r",r.rgb[0]},{"g",r.rgb[1]},{"b",r.rgb[2]}},r.text);
    result.ocrMilliseconds=float((clockSeconds()-started)*1000);result.confidence=confidence;
    event("ocr_complete",{{"job_id",double(job.id)},{"generation",double(job.generation)},{"ocr_ms",result.ocrMilliseconds},{"queue_ms",perf.ocrQueueMs},{"confidence",confidence},{"translate",double(translate)}},source);
    // Keep a caption while its source remains visible. The current OCR reading
    // is only the final fragment of a joined sentence, not its full displaySource.
    if(!source.empty()&&SubtitleState::equivalent(source,displayReading))result.expiresAt=std::max(result.expiresAt,clockSeconds()+std::max(1.0,2*settings.intervalMs/1000.));
    if(!loading&&modelError.empty())result.status=source.empty()?"Waiting for English subtitles":translate?"Subtitle queued for translation":"Reading English subtitles";
    if(translate){if(texts.size()>=3){++perf.translationDrops;event("translation_dropped",{{"job_id",double(texts.front().id)}});texts.pop_front();}texts.push_back({state.committed,generation,result.ocrMilliseconds,job.time,clockSeconds(),job.id});translateWake.notify_one();}
   }catch(const winrt::hresult_error& e){neural.reset();std::lock_guard lock(mutex);perf.ocrBusy=false;++perf.ocrErrors;event("ocr_error",{{"job_id",double(job.id)}});if(generation==job.generation)ocrError="OCR: "+winrt::to_string(e.message());failedGeneration=job.generation;}
    catch(const std::exception& e){neural.reset();std::lock_guard lock(mutex);if(job.preview){if(sampleEpoch==job.id){sampleResult.busy=false;sampleResult.error=e.what();}continue;}perf.ocrBusy=false;++perf.ocrErrors;event("ocr_error",{{"job_id",double(job.id)}});if(generation==job.generation)ocrError=e.what();failedGeneration=job.generation;}
  }
  winrt::uninit_apartment();
 }
 void runTranslation(){
  std::unique_ptr<TranslationProcess> process;uint64_t seenEpoch=0,failedEpoch=0;int loadedModel=-1;
  std::unordered_map<std::string,std::string> cache;std::deque<std::string> order;SubtitleSentence sentences;
  while(!stop){
   TextJob text;SubtitleSettings selected;uint64_t epoch;
   {std::unique_lock lock(mutex);translateWake.wait(lock,[&]{return stop||modelEpoch!=seenEpoch||!texts.empty();});if(stop)break;epoch=modelEpoch;seenEpoch=epoch;selected=settings;if(!texts.empty()){text=std::move(texts.front());texts.pop_front();}}
   if(!selected.enabled){process.reset();loadedModel=-1;activeModel=-1;std::lock_guard lock(mutex);loading=false;result.status="Translation off; model unloaded";continue;}
   if(failedEpoch==epoch)continue;
   const auto cancelled=[&]{return stop||modelEpoch!=epoch;};
   try{
    if(!process||loadedModel!=selected.model){
     process.reset();activeModel=-1;cache.clear();order.clear();sentences.clear();
     {std::lock_guard lock(mutex);loading=true;modelError.clear();result.status="Loading and warming local translation model...";}
     const auto loadingStarted=clockSeconds();process=std::make_unique<TranslationProcess>();process->launch(app,selected.model);
     const auto ready=process->receive(cancelled,120);if(ready!="READY")throw std::runtime_error(ready);
     loadedModel=selected.model;
     activeModel=loadedModel;
     {std::lock_guard lock(mutex);if(modelEpoch==epoch){loading=false;perf.modelLoadMs=(clockSeconds()-loadingStarted)*1000;event("model_ready",{{"model",double(loadedModel)},{"load_ms",perf.modelLoadMs}});result.status="Local model ready";}}
    }
    if(text.source.empty())continue;
    {std::lock_guard lock(mutex);if(text.generation!=generation||clockSeconds()-text.time>6){++perf.translationDrops;event("translation_expired",{{"job_id",double(text.id)}});continue;}perf.translationBusy=true;perf.translationQueueMs=(clockSeconds()-text.queuedAt)*1000;}
    const auto input=sentences.prepare(text.source,text.time,text.generation,selected.joinFragments);
    const auto start=clockSeconds();std::string translated;bool cacheHit=false;
    if(auto found=cache.find(input.request);found!=cache.end()){translated=found->second;cacheHit=true;}
    else{
     translated=process->translate(input.request,cancelled);
     if(cache.size()>=128){cache.erase(order.front());order.pop_front();}
     cache[input.request]=translated;order.push_back(input.request);
    }
    // Continue OCR and translation while the previous Czech caption is readable.
    {std::lock_guard lock(mutex);perf.translationBusy=false;++perf.translations;if(cacheHit)++perf.cacheHits;perf.translationMs=(clockSeconds()-start)*1000;event("translation_complete",{{"job_id",double(text.id)},{"model",double(loadedModel)},{"translation_ms",perf.translationMs},{"queue_ms",perf.translationQueueMs},{"cache_hit",double(cacheHit)}},input.source,translated);if(text.generation==generation&&modelEpoch==epoch&&clockSeconds()-text.time<=6){if(captions.size()>=3){++perf.captionDrops;captions.pop_front();}captions.push_back({input.source,std::move(translated),text.ocrMs+float(perf.translationMs),text.time,text.source,text.id});result.status=input.joined?"Translated sentence continuation":"Local OCR + EN > CS ready";}}
   }catch(const std::exception& e){process.reset();loadedModel=-1;activeModel=-1;std::lock_guard lock(mutex);loading=false;perf.translationBusy=false;if(modelEpoch==epoch&&!stop){++perf.translationErrors;event("translation_error",{{"job_id",double(text.id)}});failedEpoch=epoch;modelError=e.what();result.status=modelError;result.translated.clear();}}
  }
 }
};
SubtitleTranslator::SubtitleTranslator(const std::filesystem::path& app):impl(std::make_unique<Impl>(app)){}
SubtitleTranslator::~SubtitleTranslator()=default;
void SubtitleTranslator::configure(SubtitleSettings settings){
 settings.sanitize();std::lock_guard lock(impl->mutex);const auto& previous=impl->settings;
 const bool modelChanged=settings.enabled!=previous.enabled||settings.model!=previous.model||!impl->modelError.empty();
 const bool ocrChanged=settings.enabled!=previous.enabled||settings.ocrEngine!=previous.ocrEngine||!impl->ocrError.empty();
 const bool sourceChanged=modelChanged||ocrChanged||settings.joinFragments!=previous.joinFragments||settings.left!=previous.left||settings.top!=previous.top||settings.right!=previous.right||settings.bottom!=previous.bottom||settings.intervalMs!=previous.intervalMs||settings.style!=previous.style;
 if(modelChanged){++impl->modelEpoch;impl->modelError.clear();}if(ocrChanged)++impl->ocrEpoch;
 impl->settings=settings;if(sourceChanged)impl->clearLocked();impl->translateWake.notify_one();impl->wake.notify_one();
}
void SubtitleTranslator::setDiagnostics(Diagnostics* recorder){impl->diagnostics=recorder;}
void SubtitleTranslator::reset(){std::lock_guard lock(impl->mutex);impl->clearLocked();}
SubtitleSnapshot SubtitleTranslator::snapshot(bool paused)const{std::lock_guard lock(impl->mutex);impl->displayLocked(clockSeconds(),paused);auto value=impl->result;value.performance=impl->perf;if(!impl->ocrError.empty())value.status=impl->ocrError;else if(!impl->modelError.empty())value.status=impl->modelError;value.activeModel=impl->activeModel;value.loading=impl->loading;return value;}
bool SubtitleTranslator::rapidOcrAvailable(const std::filesystem::path& app){return std::filesystem::is_regular_file(app/L"ocr"/L"ocr_worker.py")&&std::filesystem::is_regular_file(app/L"ocr"/L"models"/L"en_PP-OCRv5_rec_mobile.onnx")&&std::filesystem::is_directory(app/L"ocr"/L"deps"/L"rapidocr_onnxruntime");}
SubtitleStyleSample SubtitleTranslator::styleSample()const{std::lock_guard lock(impl->mutex);return impl->sampleResult;}
void SubtitleTranslator::cancelStyleSample(){std::lock_guard lock(impl->mutex);++impl->sampleEpoch;impl->sampleAvailable=false;impl->samplePending={};impl->sampleResult={};}
uint64_t SubtitleTranslator::requestStyleSample(const unsigned char* bgra,unsigned w,unsigned h,std::array<float,4> word,std::array<float,4> area){
 if(!bgra||!w||!h)return 0;std::lock_guard lock(impl->mutex);Impl::Job job;job.preview=true;job.sourceHeight=h;job.id=++impl->sampleEpoch;job.generation=impl->generation;job.time=clockSeconds();job.word=word;job.area=area;
 const unsigned factor=std::max({1u,(w+1919)/1920,(h+1919)/1920});job.w=w/factor;job.h=h/factor;job.fullHeight=float(h)/factor;job.pixels.resize(size_t(job.w)*job.h*4);
 for(unsigned row=0;row<job.h;++row)for(unsigned col=0;col<job.w;++col)memcpy(job.pixels.data()+(size_t(row)*job.w+col)*4,bgra+(size_t(row*factor)*w+col*factor)*4,4);
 impl->sampleResult={};impl->sampleResult.id=job.id;impl->sampleResult.busy=true;impl->samplePending=std::move(job);impl->sampleAvailable=true;impl->wake.notify_one();return impl->sampleResult.id;
}
void SubtitleTranslator::submit(const unsigned char* bgra,unsigned w,unsigned h,double time){
 if(!bgra||!w||!h)return;std::lock_guard lock(impl->mutex);auto& s=impl->settings;
 if(!s.enabled||time-impl->lastSubmitted<s.intervalMs/1000.)return;
 impl->lastSubmitted=time;unsigned x=unsigned(w*s.left),y=unsigned(h*s.top),cw=std::max(1u,unsigned(w*s.right)-x),ch=std::max(1u,unsigned(h*s.bottom)-y);
 // Bound OCR work without touching the full-frame rendering path.
 unsigned factor=std::max({1u,(cw+1919)/1920,(ch+1919)/1920});unsigned ow=cw/factor,oh=ch/factor;if(!ow||!oh)return;
 Impl::Job job;job.w=ow;job.h=oh;job.sourceHeight=h;job.inspect=s.inspectOcr;job.time=time;job.generation=impl->generation;job.engine=s.ocrEngine;job.style=s.style;job.fullHeight=float(h)/factor;job.pixels.resize(size_t(ow)*oh*4);
 for(unsigned row=0;row<oh;++row)for(unsigned col=0;col<ow;++col)memcpy(job.pixels.data()+(size_t(row)*ow+col)*4,bgra+(size_t(y+row*factor)*w+x+col*factor)*4,4);
 job.id=++impl->nextJob;++impl->perf.submitted;if(impl->available){++impl->perf.ocrSuperseded;impl->event("ocr_superseded",{{"job_id",double(impl->pending.id)}});}impl->event("ocr_submit",{{"job_id",double(job.id)},{"width",double(ow)},{"height",double(oh)}});
 impl->pending=std::move(job);impl->available=true;impl->wake.notify_one();
}
