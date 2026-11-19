#include "SubtitleTranslator.hpp"
#include "SubtitleState.hpp"
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
  PROCESS_INFORMATION pi{};std::wstring command=L"\""+python.wstring()+L"\" -I \""+script.wstring()+L"\" ";
  command+=ocr?L"--mapping \""+mapping+L"\"":L"--model \""+(base/L"model").wstring()+L"\" --backend "+(model==1?L"cuni-q3km":model==2?L"cuni-q8":L"cpu");
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
 std::string receive(const std::function<bool()>& cancelled,double timeout=30){uint32_t length=0;readExact(&length,4,cancelled,timeout);if(length>16384)throw std::runtime_error("Invalid translation response length");std::string value(length,'\0');if(length)readExact(value.data(),length,cancelled,timeout);return value;}
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
 std::pair<std::string,float> recognize(const std::vector<unsigned char>& data,unsigned w,unsigned h,const std::function<bool()>& cancelled){
  if(data.size()>capacity)throw std::runtime_error("OCR crop too large");
  memcpy(pixels,data.data(),data.size());uint32_t header[]{w,h};DWORD sent=0;
  if(!WriteFile(input,header,sizeof(header),&sent,nullptr)||sent!=sizeof(header))throw std::runtime_error("OCR request failed");
  const auto value=receive(cancelled,15);if(value.starts_with("ERROR:"))throw std::runtime_error(value);
  const auto tab=value.find('\t');if(tab==std::string::npos)throw std::runtime_error("Invalid OCR response");
  return {value.substr(tab+1),std::clamp(std::stof(value.substr(0,tab)),0.f,1.f)};
 }
};
}
struct SubtitleTranslator::Impl {
 struct Job{std::vector<unsigned char> pixels;unsigned w=0,h=0;double time=0;uint64_t generation=0;int engine=0;};
 struct TextJob{std::string source;uint64_t generation=0;float ocrMs=0;double time=0;};
 struct Caption{std::string source,translated;float ms=0;double time=0;};
 std::filesystem::path app;mutable std::mutex mutex;std::condition_variable wake,translateWake;std::atomic<bool> stop=false;
 std::atomic<uint64_t> modelEpoch=1;
 std::atomic<uint64_t> ocrEpoch=1;
 std::atomic<int> activeModel=-1;
 SubtitleSettings settings;SubtitleSnapshot result;Job pending;std::deque<TextJob> texts;std::deque<Caption> captions;
 bool available=false,loading=false;uint64_t generation=1;double lastSubmitted=0;
 std::string modelError,ocrError;std::thread worker,translator;
 explicit Impl(std::filesystem::path p):app(std::move(p)),worker([this]{runOcr();}),translator([this]{runTranslation();}){}
 ~Impl(){stop=true;wake.notify_all();translateWake.notify_all();worker.join();translator.join();}
 void clearLocked(){++generation;pending={};texts.clear();captions.clear();available=false;lastSubmitted=0;result={};result.generation=generation;ocrError.clear();}
 void displayLocked(double now,bool paused){
  // Never build an unlimited delay: retain at most three pending lines and
  // expire work older than six seconds. Source/seek/model resets still flush all.
  while(!captions.empty()&&now-captions.front().time>6)captions.pop_front();
  if(!captions.empty()&&(result.translated.empty()||(!paused&&now-result.displayedAt>=settings.holdSeconds))){
   auto caption=std::move(captions.front());captions.pop_front();
   result.displaySource=std::move(caption.source);result.translated=std::move(caption.translated);result.milliseconds=caption.ms;result.displayedAt=now;
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
   Job job;uint64_t epoch;{std::unique_lock lock(mutex);wake.wait(lock,[&]{return stop||available||ocrEpoch!=seenEpoch;});if(stop)break;epoch=ocrEpoch;if(epoch!=seenEpoch){neural.reset();seenEpoch=epoch;}if(!available)continue;job=std::move(pending);available=false;}
   if(failedGeneration==job.generation)continue;
   try{
    const auto started=clockSeconds();
    if(seenGeneration!=job.generation){seenGeneration=job.generation;state.clear();}
    std::string source;float confidence=0;
    if(job.engine==0){
     const auto cancelled=[&]{return stop||ocrEpoch!=epoch;};
     if(!neural){neural=std::make_unique<OcrProcess>();neural->start(app,cancelled);}
     auto reading=neural->recognize(job.pixels,job.w,job.h,cancelled);source=SubtitleState::normalize(reading.first);confidence=reading.second;
    }else{
     if(!engine){engine=winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(winrt::Windows::Globalization::Language(L"en-US"));if(!engine)throw std::runtime_error("Windows English OCR unavailable. Select bundled RapidOCR or install English OCR.");}
     winrt::Windows::Storage::Streams::DataWriter writer;writer.WriteBytes(job.pixels);
     auto bitmap=winrt::Windows::Graphics::Imaging::SoftwareBitmap::CreateCopyFromBuffer(writer.DetachBuffer(),winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,int(job.w),int(job.h),winrt::Windows::Graphics::Imaging::BitmapAlphaMode::Ignore);
     auto recognized=engine.RecognizeAsync(bitmap).get();source=SubtitleState::normalize(winrt::to_string(recognized.Text()));bitmap.Close();writer.Close();
    }
    const bool translate=state.observe(source,confidence);
    std::lock_guard lock(mutex);if(generation!=job.generation)continue;
    result.source=source;result.observedAt=job.time;result.generation=job.generation;
    result.ocrMilliseconds=float((clockSeconds()-started)*1000);result.confidence=confidence;
    if(!source.empty()&&SubtitleState::equivalent(source,result.displaySource))result.expiresAt=std::max(result.expiresAt,clockSeconds()+.5);
    if(!loading&&modelError.empty())result.status=source.empty()?"Waiting for English subtitles":translate?"Subtitle queued for translation":"Reading English subtitles";
    if(translate){if(texts.size()>=3)texts.pop_front();texts.push_back({state.committed,generation,result.ocrMilliseconds,job.time});translateWake.notify_one();}
   }catch(const winrt::hresult_error& e){neural.reset();std::lock_guard lock(mutex);if(generation==job.generation)ocrError="OCR: "+winrt::to_string(e.message());failedGeneration=job.generation;}
    catch(const std::exception& e){neural.reset();std::lock_guard lock(mutex);if(generation==job.generation)ocrError=e.what();failedGeneration=job.generation;}
  }
  winrt::uninit_apartment();
 }
 void runTranslation(){
  std::unique_ptr<TranslationProcess> process;uint64_t seenEpoch=0,failedEpoch=0;int loadedModel=-1;
  std::unordered_map<std::string,std::string> cache;std::deque<std::string> order;
  while(!stop){
   TextJob text;SubtitleSettings selected;uint64_t epoch;
   {std::unique_lock lock(mutex);translateWake.wait(lock,[&]{return stop||modelEpoch!=seenEpoch||!texts.empty();});if(stop)break;epoch=modelEpoch;seenEpoch=epoch;selected=settings;if(!texts.empty()){text=std::move(texts.front());texts.pop_front();}}
   if(!selected.enabled){process.reset();loadedModel=-1;activeModel=-1;std::lock_guard lock(mutex);loading=false;result.status="Translation off; model unloaded";continue;}
   if(failedEpoch==epoch)continue;
   const auto cancelled=[&]{return stop||modelEpoch!=epoch;};
   try{
    if(!process||loadedModel!=selected.model){
     process.reset();activeModel=-1;cache.clear();order.clear();
     {std::lock_guard lock(mutex);loading=true;modelError.clear();result.status="Loading and warming local translation model...";}
     process=std::make_unique<TranslationProcess>();process->launch(app,selected.model);
     const auto ready=process->receive(cancelled,120);if(ready!="READY")throw std::runtime_error(ready);
     loadedModel=selected.model;
     activeModel=loadedModel;
     {std::lock_guard lock(mutex);if(modelEpoch==epoch){loading=false;result.status="Local model ready";}}
    }
    if(text.source.empty())continue;
    {std::lock_guard lock(mutex);if(text.generation!=generation||clockSeconds()-text.time>6)continue;}
    const auto start=clockSeconds();std::string translated;
    if(auto found=cache.find(text.source);found!=cache.end())translated=found->second;
    else{
     translated=process->translate(text.source,cancelled);
     if(cache.size()>=128){cache.erase(order.front());order.pop_front();}
     cache[text.source]=translated;order.push_back(text.source);
    }
    // Continue OCR and translation while the previous Czech caption is readable.
    {std::lock_guard lock(mutex);if(text.generation==generation&&modelEpoch==epoch&&clockSeconds()-text.time<=6){if(captions.size()>=3)captions.pop_front();captions.push_back({text.source,std::move(translated),text.ocrMs+float((clockSeconds()-start)*1000),text.time});result.status="Local OCR + EN > CS ready";}}
   }catch(const std::exception& e){process.reset();loadedModel=-1;activeModel=-1;std::lock_guard lock(mutex);loading=false;if(modelEpoch==epoch&&!stop){failedEpoch=epoch;modelError=e.what();result.status=modelError;result.translated.clear();}}
  }
 }
};
SubtitleTranslator::SubtitleTranslator(const std::filesystem::path& app):impl(std::make_unique<Impl>(app)){}
SubtitleTranslator::~SubtitleTranslator()=default;
void SubtitleTranslator::configure(SubtitleSettings settings){
 settings.sanitize();std::lock_guard lock(impl->mutex);const auto& previous=impl->settings;
 const bool modelChanged=settings.enabled!=previous.enabled||settings.model!=previous.model||!impl->modelError.empty();
 const bool ocrChanged=settings.enabled!=previous.enabled||settings.ocrEngine!=previous.ocrEngine||!impl->ocrError.empty();
 const bool sourceChanged=modelChanged||ocrChanged||settings.left!=previous.left||settings.top!=previous.top||settings.right!=previous.right||settings.bottom!=previous.bottom||settings.intervalMs!=previous.intervalMs;
 if(modelChanged){++impl->modelEpoch;impl->modelError.clear();}if(ocrChanged)++impl->ocrEpoch;
 impl->settings=settings;if(sourceChanged)impl->clearLocked();impl->translateWake.notify_one();impl->wake.notify_one();
}
void SubtitleTranslator::reset(){std::lock_guard lock(impl->mutex);impl->clearLocked();}
SubtitleSnapshot SubtitleTranslator::snapshot(bool paused)const{std::lock_guard lock(impl->mutex);impl->displayLocked(clockSeconds(),paused);auto value=impl->result;if(!impl->ocrError.empty())value.status=impl->ocrError;else if(!impl->modelError.empty())value.status=impl->modelError;value.activeModel=impl->activeModel;value.loading=impl->loading;return value;}
bool SubtitleTranslator::rapidOcrAvailable(const std::filesystem::path& app){return std::filesystem::is_regular_file(app/L"ocr"/L"ocr_worker.py")&&std::filesystem::is_regular_file(app/L"ocr"/L"models"/L"en_PP-OCRv5_rec_mobile.onnx")&&std::filesystem::is_directory(app/L"ocr"/L"deps"/L"rapidocr_onnxruntime");}
void SubtitleTranslator::submit(const unsigned char* bgra,unsigned w,unsigned h,double time){
 if(!bgra||!w||!h)return;std::lock_guard lock(impl->mutex);auto& s=impl->settings;
 if(!s.enabled||time-impl->lastSubmitted<s.intervalMs/1000.)return;
 impl->lastSubmitted=time;unsigned x=unsigned(w*s.left),y=unsigned(h*s.top),cw=std::max(1u,unsigned(w*s.right)-x),ch=std::max(1u,unsigned(h*s.bottom)-y);
 // Bound OCR work without touching the full-frame rendering path.
 unsigned factor=std::max({1u,(cw+1919)/1920,(ch+1919)/1920});unsigned ow=cw/factor,oh=ch/factor;if(!ow||!oh)return;
 Impl::Job job;job.w=ow;job.h=oh;job.time=time;job.generation=impl->generation;job.engine=s.ocrEngine;job.pixels.resize(size_t(ow)*oh*4);
 for(unsigned row=0;row<oh;++row)for(unsigned col=0;col<ow;++col)memcpy(job.pixels.data()+(size_t(row)*ow+col)*4,bgra+(size_t(y+row*factor)*w+x+col*factor)*4,4);
 impl->pending=std::move(job);impl->available=true;impl->wake.notify_one();
}
