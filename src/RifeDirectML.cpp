#include "RifeDirectML.hpp"
#define ORT_API_MANUAL_INIT
#include <onnxruntime_cxx_api.h>
#include <dml_provider_factory.h>
#include <d3d12.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <array>
#include <algorithm>

namespace {
using Clock=std::chrono::steady_clock;
struct Runtime {
 HMODULE dml=nullptr,ort=nullptr;const OrtDmlApi* api=nullptr;std::unique_ptr<Ort::Env> env;
 explicit Runtime(const std::filesystem::path& folder){
  // Pin the native libraries for process lifetime: retired asynchronous workers
  // and a second before/after FG instance may still hold ORT objects.
  dml=LoadLibraryExW((folder/L"DirectML.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
  if(!dml)throw std::runtime_error("RIFE: DirectML.dll is missing or cannot load");
  ort=LoadLibraryExW((folder/L"onnxruntime.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
  if(!ort)throw std::runtime_error("RIFE: onnxruntime.dll is missing or cannot load");
  auto get=reinterpret_cast<const OrtApiBase*(ORT_API_CALL*)()>(GetProcAddress(ort,"OrtGetApiBase"));
  if(!get)throw std::runtime_error("RIFE: invalid ONNX Runtime library");
  auto ortApi=get()->GetApi(ORT_API_VERSION);if(!ortApi)throw std::runtime_error("RIFE: ONNX Runtime API version mismatch");
  Ort::InitApi(ortApi);Ort::ThrowOnError(ortApi->GetExecutionProviderApi("DML",ORT_API_VERSION,reinterpret_cast<const void**>(&api)));
  env=std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING,"FrameLab RIFE DirectML");env->DisableTelemetryEvents();
 }
};
std::shared_ptr<Runtime> runtime(const std::filesystem::path& folder){
 static std::mutex mutex;static std::shared_ptr<Runtime> value;std::lock_guard lock(mutex);
 if(!value)value=std::make_shared<Runtime>(folder);return value;
}
ComPtr<IDXGIAdapter> adapterOf(ID3D11Device* device){ComPtr<IDXGIDevice> dx;ComPtr<IDXGIAdapter> adapter;check(device->QueryInterface(IID_PPV_ARGS(&dx)),"RIFE DXGI device");check(dx->GetAdapter(&adapter),"RIFE adapter");return adapter;}
unsigned adapterIndex(ID3D11Device* device){
 auto adapter=adapterOf(device);DXGI_ADAPTER_DESC selected{};check(adapter->GetDesc(&selected),"RIFE adapter identity");
 ComPtr<IDXGIFactory1> factory;check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"RIFE factory");
 for(unsigned i=0;;++i){ComPtr<IDXGIAdapter1> a;if(factory->EnumAdapters1(i,&a)==DXGI_ERROR_NOT_FOUND)break;DXGI_ADAPTER_DESC1 d{};check(a->GetDesc1(&d),"RIFE enumerated adapter");if(d.AdapterLuid.LowPart==selected.AdapterLuid.LowPart&&d.AdapterLuid.HighPart==selected.AdapterLuid.HighPart)return i;}
 throw std::runtime_error("RIFE: renderer adapter is no longer available");
}
bool sceneCut(const std::vector<unsigned char>& a,const std::vector<unsigned char>& b,unsigned w,unsigned h){
 int ha[48]{},hb[48]{};double difference=0;
 for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x){size_t i=(size_t(std::min(h-1,(2*y+1)*h/32))*w+std::min(w-1,(2*x+1)*w/32))*4;
  for(unsigned c=0;c<3;++c){++ha[c*16+a[i+c]/16];++hb[c*16+b[i+c]/16];difference+=abs(int(a[i+c])-int(b[i+c]));}}
 int hist=0;for(unsigned i=0;i<48;++i)hist+=abs(ha[i]-hb[i]);return difference/(256*3*255)>.18&&double(hist)/1536>.45;
}
}

struct RifeDirectML::Impl {
 ComPtr<ID3D11DeviceContext> context;ComPtr<ID3D11Texture2D> output,staging;
 unsigned w=0,h=0,pw=0,ph=0,adapter=0,quality=1;bool automaticCuts=true;
 std::filesystem::path folder;std::array<std::vector<unsigned char>,2> inputs;
 std::vector<unsigned char> pixels;std::array<bool,2> uploaded{};
 std::atomic<bool> stop{false},busy{false},complete{false},initializing{true},failed{false};std::atomic<float> ms{-1};
 std::mutex mutex;std::condition_variable cv;bool queued=false;unsigned current=0,index=1,multiplier=2;bool cut=false;std::string failure;
 void fail(const std::string& message){{std::lock_guard lock(mutex);failure=message;}failed=true;initializing=false;}
 void checkFailure(){if(failed){std::lock_guard lock(mutex);throw std::runtime_error("RIFE DirectML: "+failure);}}
 void pad(const std::vector<unsigned char>& input,std::vector<unsigned char>& padded){
  for(unsigned y=0;y<ph;++y){auto source=input.data()+size_t(std::min(y,h-1))*w*4;auto target=padded.data()+size_t(y)*pw*4;memcpy(target,source,w*4);for(unsigned x=w;x<pw;++x)memcpy(target+x*4,source+(w-1)*4,4);}
 }
 void run(){try{
  auto rt=runtime(folder);if(stop)return;
  Ort::SessionOptions options;options.DisableMemPattern();options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);options.SetIntraOpNumThreads(1);options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
  Ort::ThrowOnError(rt->api->SessionOptionsAppendExecutionProvider_DML(options,int(adapter)));
  // Reject an unsupported graph instead of silently running the model on CPU.
  options.AddConfigEntry("session.disable_cpu_ep_fallback","1");
  Ort::ThrowOnError(Ort::GetApi().AddFreeDimensionOverrideByName(options,"height",ph));
  Ort::ThrowOnError(Ort::GetApi().AddFreeDimensionOverrideByName(options,"width",pw));
  const wchar_t* names[]={L"rife422-lite-low.onnx",L"rife422-lite-medium.onnx",L"rife422-lite-high.onnx"};
  Ort::Session session(*rt->env,(folder/names[quality]).c_str(),options);
  if(stop)return;
  auto memory=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
  const size_t count=size_t(pw)*ph*4;std::array<std::vector<unsigned char>,2> padded;for(auto& v:padded)v.resize(count);std::vector<unsigned char> result(count);
  std::array<int64_t,3> shape{ph,pw,4};std::array<int64_t,4> tshape{1,1,1,1};float timestep=.5f;
  std::array<Ort::Value,3> tensors{
   Ort::Value::CreateTensor<unsigned char>(memory,padded[0].data(),count,shape.data(),shape.size()),
   Ort::Value::CreateTensor<unsigned char>(memory,padded[1].data(),count,shape.data(),shape.size()),
   Ort::Value::CreateTensor<float>(memory,&timestep,1,tshape.data(),tshape.size())};
  auto resultTensor=Ort::Value::CreateTensor<unsigned char>(memory,result.data(),count,shape.data(),shape.size());
  const char* inNames[]={"frame0","frame1","t"};const char* outNames[]={"frame"};initializing=false;
  for(;;){unsigned c,phase,factor;bool shot;
   {std::unique_lock lock(mutex);cv.wait(lock,[&]{return stop||queued;});if(stop)break;c=current;phase=index;factor=multiplier;shot=cut;queued=false;}
   auto started=Clock::now();const auto& a=inputs[1-c];const auto& b=inputs[c];
   if(shot||(automaticCuts&&sceneCut(a,b,w,h))||a==b)pixels=b;
   else{
    pad(a,padded[0]);pad(b,padded[1]);timestep=float(phase)/factor;
    session.Run(Ort::RunOptions{nullptr},inNames,tensors.data(),tensors.size(),outNames,&resultTensor,1);
    for(unsigned y=0;y<h;++y)memcpy(pixels.data()+size_t(y)*w*4,result.data()+size_t(y)*pw*4,w*4);
   }
   ms=std::chrono::duration<float,std::milli>(Clock::now()-started).count();complete=true;
  }
 }catch(const std::exception& e){fail(e.what());}}
};

RifeDirectML::~RifeDirectML(){reset();}
bool RifeDirectML::supportedAdapter(ID3D11Device* device){try{auto adapter=adapterOf(device);return SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,__uuidof(ID3D12Device),nullptr));}catch(...){return false;}}
bool RifeDirectML::available(const std::filesystem::path& root){std::error_code ec;const auto folder=root/L"rife";for(auto name:{L"onnxruntime.dll",L"DirectML.dll",L"rife422-lite-low.onnx",L"rife422-lite-medium.onnx",L"rife422-lite-high.onnx"})if(!std::filesystem::is_regular_file(folder/name,ec))return false;return true;}
void RifeDirectML::initialize(ID3D11Device* device,ID3D11Texture2D* output,unsigned w,unsigned h,unsigned quality,const std::filesystem::path& root,bool cuts){
 reset();if(!w||!h||w>8192||h>8192||!supportedAdapter(device))throw std::runtime_error("RIFE requires a DirectX 12 GPU and valid frame dimensions");
 if(!available(root))throw std::runtime_error("RIFE runtime/model missing from runtime/rife");
 auto q=std::make_shared<Impl>();q->w=w;q->h=h;q->quality=std::min(quality,2u);unsigned alignment=q->quality==0?128:q->quality==1?64:32;q->pw=(w+alignment-1)/alignment*alignment;q->ph=(h+alignment-1)/alignment*alignment;q->adapter=adapterIndex(device);q->automaticCuts=cuts;q->folder=std::filesystem::absolute(root/L"rife");q->output=output;device->GetImmediateContext(&q->context);
 D3D11_TEXTURE2D_DESC d{};output->GetDesc(&d);if(d.Width!=w||d.Height!=h||d.Format!=DXGI_FORMAT_B8G8R8A8_UNORM)throw std::runtime_error("RIFE output format mismatch");d.Usage=D3D11_USAGE_STAGING;d.BindFlags=d.MiscFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;check(device->CreateTexture2D(&d,nullptr,&q->staging),"RIFE staging texture");
 for(auto& v:q->inputs)v.resize(size_t(w)*h*4);q->pixels.resize(size_t(w)*h*4);p=q;
 std::thread([q]{q->run();}).detach(); // q owns every worker resource until it finishes.
}
void RifeDirectML::upload(const unsigned char* pixels,unsigned parity){if(!p||!pixels||parity>1||p->busy)throw std::runtime_error("Invalid or busy RIFE input");p->checkFailure();memcpy(p->inputs[parity].data(),pixels,p->inputs[parity].size());p->uploaded[parity]=true;}
void RifeDirectML::upload(ID3D11Texture2D* texture,unsigned parity){
 if(!p||!texture||parity>1||p->busy)throw std::runtime_error("Invalid or busy RIFE texture input");p->checkFailure();D3D11_TEXTURE2D_DESC d{};texture->GetDesc(&d);if(d.Width!=p->w||d.Height!=p->h||d.Format!=DXGI_FORMAT_B8G8R8A8_UNORM)throw std::runtime_error("RIFE input format mismatch");
 p->context->CopyResource(p->staging.Get(),texture);D3D11_MAPPED_SUBRESOURCE m{};check(p->context->Map(p->staging.Get(),0,D3D11_MAP_READ,0,&m),"RIFE input readback");for(unsigned y=0;y<p->h;++y)memcpy(p->inputs[parity].data()+size_t(y)*p->w*4,(char*)m.pData+size_t(y)*m.RowPitch,p->w*4);p->context->Unmap(p->staging.Get(),0);p->uploaded[parity]=true;
}
void RifeDirectML::generate(unsigned current,bool cut,unsigned index,unsigned multiplier){
 if(!p||p->busy||current>1||multiplier<2||multiplier>4||!index||index>=multiplier||!p->uploaded[0]||!p->uploaded[1])throw std::runtime_error("Invalid or busy RIFE generation");p->checkFailure();
 {std::lock_guard lock(p->mutex);p->current=current;p->cut=cut;p->index=index;p->multiplier=multiplier;p->complete=false;p->busy=true;p->queued=true;}p->cv.notify_one();
}
bool RifeDirectML::ready(){if(!p)return false;p->checkFailure();if(!p->busy||!p->complete)return false;p->context->UpdateSubresource(p->output.Get(),0,nullptr,p->pixels.data(),p->w*4,0);p->complete=false;p->busy=false;return true;}
bool RifeDirectML::loading()const{return p&&p->initializing;}
float RifeDirectML::processingMs()const{return p?p->ms.load():-1;}
void RifeDirectML::reset(){if(p){p->stop=true;p->cv.notify_all();p.reset();}}
