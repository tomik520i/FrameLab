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
 void launch(const std::filesystem::path& app,int model){
  std::filesystem::create_directories(app/L"logs");
  const auto base=app/L"translation",python=base/L"python"/L"python.exe",script=base/L"translate_worker.py";
  if(!std::filesystem::is_regular_file(python)||!std::filesystem::is_regular_file(base/L"model"/L"model.bin"))throw std::runtime_error("Local translation files missing: translation/python and translation/model");
  SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};Handle childInput,childOutput;
  if(!CreatePipe(&childInput.value,&input.value,&sa,4096)||!CreatePipe(&output.value,&childOutput.value,&sa,4096))throw std::runtime_error("Subtitle pipe creation failed");
  SetHandleInformation(input,HANDLE_FLAG_INHERIT,0);SetHandleInformation(output,HANDLE_FLAG_INHERIT,0);
  errors.value=CreateFileW((app/L"logs"/L"translation-worker.log").c_str(),GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;si.hStdInput=childInput;si.hStdOutput=childOutput;si.hStdError=errors;
  PROCESS_INFORMATION pi{};std::wstring command=L"\""+python.wstring()+L"\" -I \""+script.wstring()+L"\" --model \""+(base/L"model").wstring()+L"\" --backend "+(model==1?L"cuni-q3km":model==2?L"cuni-q8":L"cpu");
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
}
struct SubtitleTranslator::Impl {
 struct Job{std::vector<unsigned char> pixels;unsigned w=0,h=0;double time=0;uint64_t generation=0;};
 struct TextJob{std::string source;uint64_t generation=0,revision=0;float ocrMs=0;};
 std::filesystem::path app;mutable std::mutex mutex;std::condition_variable wake,translateWake;std::atomic<bool> stop=false;
 std::atomic<uint64_t> modelEpoch=1;
 std::atomic<int> activeModel=-1;
 SubtitleSettings settings;SubtitleSnapshot result;Job pending;TextJob latest;
 bool available=false,textAvailable=false,loading=false;uint64_t generation=1,revision=0;double lastSubmitted=0;
 std::string modelError,ocrError;std::thread worker,translator;
 explicit Impl(std::filesystem::path p):app(std::move(p)),worker([this]{runOcr();}),translator([this]{runTranslation();}){}
 ~Impl(){stop=true;wake.notify_all();translateWake.notify_all();worker.join();translator.join();}
 void clearLocked(){++generation;++revision;pending={};latest={};available=textAvailable=false;lastSubmitted=0;result={};result.generation=generation;ocrError.clear();}
 void runOcr(){
  winrt::init_apartment(winrt::apartment_type::multi_threaded);
  winrt::Windows::Media::Ocr::OcrEngine engine{nullptr};SubtitleState state;uint64_t seenGeneration=0,failedGeneration=0;
  while(!stop){
   Job job;{std::unique_lock lock(mutex);wake.wait(lock,[&]{return stop||available;});if(stop)break;job=std::move(pending);available=false;}
   if(failedGeneration==job.generation)continue;
   try{
    const auto started=clockSeconds();
    if(seenGeneration!=job.generation){seenGeneration=job.generation;state.clear();}
    if(!engine){engine=winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(winrt::Windows::Globalization::Language(L"en-US"));if(!engine)throw std::runtime_error("Windows English OCR unavailable. Install English OCR in Windows language settings.");}
    winrt::Windows::Storage::Streams::DataWriter writer;writer.WriteBytes(job.pixels);
    auto bitmap=winrt::Windows::Graphics::Imaging::SoftwareBitmap::CreateCopyFromBuffer(writer.DetachBuffer(),winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,int(job.w),int(job.h),winrt::Windows::Graphics::Imaging::BitmapAlphaMode::Ignore);
    auto recognized=engine.RecognizeAsync(bitmap).get();const auto source=SubtitleState::normalize(winrt::to_string(recognized.Text()));bitmap.Close();writer.Close();
    const bool translate=state.observe(source);
    std::lock_guard lock(mutex);if(generation!=job.generation)continue;
    if(source!=result.source){++revision;result.translated.clear();latest={};textAvailable=false;}
    result.source=source;result.observedAt=job.time;result.generation=job.generation;
    if(!loading&&modelError.empty())result.status=source.empty()?"Waiting for English subtitles":"Local OCR + EN > CS ready";
    if(translate){latest={source,generation,revision,float((clockSeconds()-started)*1000)};textAvailable=true;translateWake.notify_one();}
   }catch(const winrt::hresult_error& e){std::lock_guard lock(mutex);if(generation==job.generation){ocrError="OCR: "+winrt::to_string(e.message());result.translated.clear();}failedGeneration=job.generation;}
    catch(const std::exception& e){std::lock_guard lock(mutex);if(generation==job.generation){ocrError=e.what();result.translated.clear();}failedGeneration=job.generation;}
  }
  winrt::uninit_apartment();
 }
 void runTranslation(){
  std::unique_ptr<TranslationProcess> process;uint64_t seenEpoch=0,failedEpoch=0;int loadedModel=-1;
  std::unordered_map<std::string,std::string> cache;std::deque<std::string> order;
  while(!stop){
   TextJob text;SubtitleSettings selected;uint64_t epoch;
   {std::unique_lock lock(mutex);translateWake.wait(lock,[&]{return stop||modelEpoch!=seenEpoch||textAvailable;});if(stop)break;epoch=modelEpoch;seenEpoch=epoch;selected=settings;text=std::move(latest);textAvailable=false;}
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
    {std::lock_guard lock(mutex);if(text.generation!=generation||text.revision!=revision)continue;}
    const auto start=clockSeconds();std::string translated;
    if(auto found=cache.find(text.source);found!=cache.end())translated=found->second;
    else{
     translated=process->translate(text.source,cancelled);
     if(cache.size()>=128){cache.erase(order.front());order.pop_front();}
     cache[text.source]=translated;order.push_back(text.source);
    }
    // OCR continues during inference. Never publish a result for a replaced line.
    {std::lock_guard lock(mutex);if(text.generation==generation&&text.revision==revision&&modelEpoch==epoch){result.translated=std::move(translated);result.milliseconds=text.ocrMs+float((clockSeconds()-start)*1000);result.status="Local OCR + EN > CS ready";}}
   }catch(const std::exception& e){process.reset();loadedModel=-1;activeModel=-1;std::lock_guard lock(mutex);loading=false;if(modelEpoch==epoch&&!stop){failedEpoch=epoch;modelError=e.what();result.status=modelError;result.translated.clear();}}
  }
 }
};
SubtitleTranslator::SubtitleTranslator(const std::filesystem::path& app):impl(std::make_unique<Impl>(app)){}
SubtitleTranslator::~SubtitleTranslator()=default;
void SubtitleTranslator::configure(SubtitleSettings settings){settings.sanitize();std::lock_guard lock(impl->mutex);if(settings.enabled!=impl->settings.enabled||settings.model!=impl->settings.model||!impl->modelError.empty()){++impl->modelEpoch;impl->modelError.clear();}impl->settings=settings;impl->clearLocked();impl->translateWake.notify_one();}
void SubtitleTranslator::reset(){std::lock_guard lock(impl->mutex);impl->clearLocked();}
SubtitleSnapshot SubtitleTranslator::snapshot()const{std::lock_guard lock(impl->mutex);auto value=impl->result;if(!impl->ocrError.empty())value.status=impl->ocrError;else if(!impl->modelError.empty())value.status=impl->modelError;value.activeModel=impl->activeModel;value.loading=impl->loading;return value;}
void SubtitleTranslator::submit(const unsigned char* bgra,unsigned w,unsigned h,double time){
 if(!bgra||!w||!h)return;std::lock_guard lock(impl->mutex);auto& s=impl->settings;
 if(!s.enabled||time-impl->lastSubmitted<s.intervalMs/1000.)return;
 impl->lastSubmitted=time;unsigned x=unsigned(w*s.left),y=unsigned(h*s.top),cw=std::max(1u,unsigned(w*s.right)-x),ch=std::max(1u,unsigned(h*s.bottom)-y);
 // Bound OCR work without touching the full-frame rendering path.
 unsigned factor=std::max(1u,(cw+1919)/1920);unsigned ow=cw/factor,oh=ch/factor;if(!ow||!oh)return;
 Impl::Job job;job.w=ow;job.h=oh;job.time=time;job.generation=impl->generation;job.pixels.resize(size_t(ow)*oh*4);
 for(unsigned row=0;row<oh;++row)for(unsigned col=0;col<ow;++col)memcpy(job.pixels.data()+(size_t(row)*ow+col)*4,bgra+(size_t(y+row*factor)*w+x+col*factor)*4,4);
 impl->pending=std::move(job);impl->available=true;impl->wake.notify_one();
}
