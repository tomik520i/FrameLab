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

namespace {
double clockSeconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
struct Handle {HANDLE value=nullptr;~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}operator HANDLE()const{return value;}};
struct TranslationProcess {
 Handle process,input,output,errors;
 ~TranslationProcess(){if(process&&WaitForSingleObject(process,0)==WAIT_TIMEOUT){TerminateProcess(process,0);WaitForSingleObject(process,2000);}}
 void launch(const std::filesystem::path& app){
  const auto base=app/L"translation",python=base/L"python"/L"python.exe",script=base/L"translate_worker.py";
  if(!std::filesystem::is_regular_file(python)||!std::filesystem::is_regular_file(base/L"model"/L"model.bin"))throw std::runtime_error("Local translation files missing: translation/python and translation/model");
  SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};Handle childInput,childOutput;
  if(!CreatePipe(&childInput.value,&input.value,&sa,4096)||!CreatePipe(&output.value,&childOutput.value,&sa,4096))throw std::runtime_error("Subtitle pipe creation failed");
  SetHandleInformation(input,HANDLE_FLAG_INHERIT,0);SetHandleInformation(output,HANDLE_FLAG_INHERIT,0);
  errors.value=CreateFileW((app/L"logs"/L"translation-worker.log").c_str(),GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;si.hStdInput=childInput;si.hStdOutput=childOutput;si.hStdError=errors;
  PROCESS_INFORMATION pi{};std::wstring command=L"\""+python.wstring()+L"\" -I \""+script.wstring()+L"\" --model \""+(base/L"model").wstring()+L"\"";
  if(!CreateProcessW(python.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,base.c_str(),&si,&pi))throw std::runtime_error("Local translation process failed to start");
  process.value=pi.hProcess;CloseHandle(pi.hThread);
 }
 void readExact(void* data,DWORD length,const std::atomic<bool>& stop){
  auto p=static_cast<unsigned char*>(data);const double deadline=clockSeconds()+30;
  while(length){if(stop||clockSeconds()>deadline)throw std::runtime_error("Translation cancelled or timed out");DWORD available=0;if(!PeekNamedPipe(output,nullptr,0,nullptr,&available,nullptr))throw std::runtime_error("Translation process stopped; see translation-worker.log");if(!available){Sleep(5);continue;}DWORD got=0;if(!ReadFile(output,p,std::min(length,available),&got,nullptr)||!got)throw std::runtime_error("Translation response failed");p+=got;length-=got;}
 }
 std::string receive(const std::atomic<bool>& stop){uint32_t length=0;readExact(&length,4,stop);if(length>16384)throw std::runtime_error("Invalid translation response length");std::string value(length,'\0');if(length)readExact(value.data(),length,stop);return value;}
 std::string translate(const std::string& text,const std::atomic<bool>& stop){
  uint32_t length=uint32_t(text.size());std::vector<unsigned char> message(length+4);memcpy(message.data(),&length,4);memcpy(message.data()+4,text.data(),length);DWORD sent=0;
  if(!WriteFile(input,message.data(),DWORD(message.size()),&sent,nullptr)||sent!=message.size())throw std::runtime_error("Translation request failed");
  auto result=receive(stop);if(result.starts_with("ERROR:"))throw std::runtime_error(result);return result;
 }
};
}
void SubtitleSettings::sanitize(){
 auto finite=[](float v,float fallback){return std::isfinite(v)?v:fallback;};
 left=std::clamp(finite(left,.1f),0.f,.95f);right=std::clamp(finite(right,.9f),left+.02f,1.f);
 top=std::clamp(finite(top,.72f),0.f,.95f);bottom=std::clamp(finite(bottom,.97f),top+.02f,1.f);
 fontPercent=std::clamp(finite(fontPercent,4),2.f,8.f);position=std::clamp(finite(position,.88f),.1f,.96f);background=std::clamp(finite(background,.8f),0.f,1.f);intervalMs=std::clamp(intervalMs,200,1500);
}
void SubtitleSettings::load(const std::filesystem::path& p){int version=0;std::ifstream f(p);if(f>>version&&(version==1||version==2)){f>>left>>top>>right>>bottom>>fontPercent>>position>>background>>intervalMs>>showOriginal;if(version==2)f>>coverSource;}sanitize();enabled=false;showRegion=false;}
void SubtitleSettings::save(const std::filesystem::path& p)const{std::ofstream(p)<<2<<' '<<left<<' '<<top<<' '<<right<<' '<<bottom<<' '<<fontPercent<<' '<<position<<' '<<background<<' '<<intervalMs<<' '<<showOriginal<<' '<<coverSource<<'\n';}
struct SubtitleTranslator::Impl {
 struct Job{std::vector<unsigned char> pixels;unsigned w=0,h=0;double time=0;uint64_t generation=0;};
 std::filesystem::path app;mutable std::mutex mutex;std::condition_variable wake;std::atomic<bool> stop=false,failed=false;
 SubtitleSettings settings;SubtitleSnapshot result;Job pending;bool available=false;uint64_t generation=1;double lastSubmitted=0;std::thread worker;
 explicit Impl(std::filesystem::path p):app(std::move(p)),worker([this]{run();}){}
 ~Impl(){stop=true;wake.notify_all();worker.join();}
 void clearLocked(){++generation;pending={};available=false;lastSubmitted=0;result={};result.generation=generation;}
 void run(){
  try{
   winrt::init_apartment(winrt::apartment_type::multi_threaded);
   winrt::Windows::Media::Ocr::OcrEngine engine{nullptr};std::unique_ptr<TranslationProcess> process;SubtitleState state;uint64_t seenGeneration=0;
   for(;;){Job job;{std::unique_lock lock(mutex);wake.wait(lock,[&]{return stop||available;});if(stop)break;job=std::move(pending);available=false;}
    const auto started=clockSeconds();
    if(seenGeneration!=job.generation){seenGeneration=job.generation;state.clear();}
    if(!engine){engine=winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(winrt::Windows::Globalization::Language(L"en-US"));if(!engine)throw std::runtime_error("Windows English OCR unavailable. Install English OCR in Windows language settings.");}
    if(!process){
     {std::lock_guard lock(mutex);if(generation==job.generation)result.status="Loading local EN > CS model...";}
     process=std::make_unique<TranslationProcess>();process->launch(app);if(process->receive(stop)!="READY")throw std::runtime_error("Translation model could not initialize");
    }
    winrt::Windows::Storage::Streams::DataWriter writer;writer.WriteBytes(job.pixels);
    auto bitmap=winrt::Windows::Graphics::Imaging::SoftwareBitmap::CreateCopyFromBuffer(writer.DetachBuffer(),winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,int(job.w),int(job.h),winrt::Windows::Graphics::Imaging::BitmapAlphaMode::Ignore);
    auto recognized=engine.RecognizeAsync(bitmap).get();const auto source=SubtitleState::normalize(winrt::to_string(recognized.Text()));bitmap.Close();writer.Close();
    bool translate=state.observe(source);
    {std::lock_guard lock(mutex);if(generation!=job.generation)continue;result.source=source;result.observedAt=job.time;result.generation=job.generation;result.status=source.empty()?"Waiting for English subtitles":"Reading English subtitles";if(source.empty()||source!=state.committed)result.translated.clear();}
    std::string translated;if(translate)translated=process->translate(source,stop);
    {std::lock_guard lock(mutex);if(generation!=job.generation)continue;if(translate)result.translated=std::move(translated);result.milliseconds=float((clockSeconds()-started)*1000);result.status="Local OCR + EN > CS ready";}
   }
  }catch(const winrt::hresult_error& e){std::lock_guard lock(mutex);result.status="OCR: "+winrt::to_string(e.message());result.translated.clear();failed=true;}
   catch(const std::exception& e){std::lock_guard lock(mutex);result.status=e.what();result.translated.clear();failed=true;}
 }
};
SubtitleTranslator::SubtitleTranslator(const std::filesystem::path& app):impl(std::make_unique<Impl>(app)){}
SubtitleTranslator::~SubtitleTranslator()=default;
void SubtitleTranslator::configure(SubtitleSettings settings){settings.sanitize();if(impl->failed.exchange(false)){impl->worker.join();impl->worker=std::thread([this]{impl->run();});}std::lock_guard lock(impl->mutex);impl->settings=settings;impl->clearLocked();}
void SubtitleTranslator::reset(){std::lock_guard lock(impl->mutex);impl->clearLocked();}
SubtitleSnapshot SubtitleTranslator::snapshot()const{std::lock_guard lock(impl->mutex);return impl->result;}
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
