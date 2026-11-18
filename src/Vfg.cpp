// Public NVIDIA VFX/CUDA C APIs, loaded dynamically. Integration informed by
// NVIDIA-Maxine/VFX-SDK-Samples (MIT) and Veyra VfgBackend (GPL-3.0).
#include "Vfg.hpp"
#include "VfgPairBinding.hpp"
#include <cuda.h>
#include <cudaD3D11.h>
#include <vector>
#include <fstream>
struct Vfg::Impl {
 HMODULE driver=nullptr;std::vector<HMODULE> dlls;DLL_DIRECTORY_COOKIE directory=nullptr;
 CUcontext ctx=nullptr;CUdevice gpu=0;CUstream stream=nullptr;CUevent first=nullptr,last=nullptr;
 CUgraphicsResource resource=nullptr;CUdeviceptr inputs[2]{},output=0;void* images[3]{};void* effect=nullptr;
 unsigned w=0,h=0,multiplier=2;bool pending=false;float ms=0;
 std::uint64_t uploadRevision=0;VfgPairBinding pairBinding;
#define FN(n) decltype(&n) n##Fn=nullptr
 FN(cuDeviceGetAttribute);FN(cuInit);FN(cuD3D11GetDevice);FN(cuDevicePrimaryCtxRetain);FN(cuDevicePrimaryCtxRelease);FN(cuCtxSetCurrent);
 FN(cuStreamCreate);FN(cuStreamDestroy);FN(cuStreamSynchronize);FN(cuMemAlloc);FN(cuMemFree);FN(cuMemcpyHtoD);
 FN(cuGraphicsD3D11RegisterResource);FN(cuGraphicsUnregisterResource);FN(cuGraphicsMapResources);FN(cuGraphicsUnmapResources);FN(cuGraphicsSubResourceGetMappedArray);
 FN(cuMemcpy2DAsync);FN(cuEventCreate);FN(cuEventDestroy);FN(cuEventRecord);FN(cuEventQuery);FN(cuEventElapsedTime);
#undef FN
 int(__cdecl* create)(const char*,void**)=nullptr;void(__cdecl* destroy)(void*)=nullptr;
 int(__cdecl* setU32)(void*,const char*,unsigned)=nullptr;int(__cdecl* setImage)(void*,const char*,void*)=nullptr;
 int(__cdecl* setStream)(void*,const char*,CUstream)=nullptr;int(__cdecl* load)(void*)=nullptr;int(__cdecl* run)(void*,int)=nullptr;
 int(__cdecl* imageCreate)(unsigned,unsigned,int,int,unsigned,unsigned,unsigned,void**)=nullptr;
 void(__cdecl* imageDealloc)(void*)=nullptr;void(__cdecl* imageDestroy)(void*)=nullptr;
 int(__cdecl* imageInit)(void*,unsigned,unsigned,int,void*,int,int,unsigned,unsigned)=nullptr;
 int(__cdecl* fromD3D)(DXGI_FORMAT,int*,int*,unsigned char*)=nullptr;
 void cu(CUresult r,const char* s){if(r!=CUDA_SUCCESS)throw std::runtime_error(std::string(s)+" CUDA="+std::to_string(r));}
 void cv(int r,const char* s){if(r)throw std::runtime_error(std::string(s)+" VFX="+std::to_string(r));}
 template<class T> void bind(HMODULE m,const char* name,T& out){out=reinterpret_cast<T>(GetProcAddress(m,name));if(!out)throw std::runtime_error(std::string("Missing API: ")+name);}
 ~Impl(){
  if(ctx&&cuCtxSetCurrentFn)cuCtxSetCurrentFn(ctx);if(stream&&cuStreamSynchronizeFn)cuStreamSynchronizeFn(stream);
  if(effect&&destroy)destroy(effect);for(auto i:images)if(i&&imageDestroy)imageDestroy(i);
  if(resource&&cuGraphicsUnregisterResourceFn)cuGraphicsUnregisterResourceFn(resource);
  for(auto a:inputs)if(a&&cuMemFreeFn)cuMemFreeFn(a);if(output&&cuMemFreeFn)cuMemFreeFn(output);
  if(first&&cuEventDestroyFn)cuEventDestroyFn(first);if(last&&cuEventDestroyFn)cuEventDestroyFn(last);
  if(stream&&cuStreamDestroyFn)cuStreamDestroyFn(stream);if(ctx&&cuDevicePrimaryCtxReleaseFn)cuDevicePrimaryCtxReleaseFn(gpu);
  for(auto it=dlls.rbegin();it!=dlls.rend();++it)FreeLibrary(*it);if(directory)RemoveDllDirectory(directory);if(driver)FreeLibrary(driver);
 }
};
Vfg::Vfg()=default;Vfg::~Vfg()=default;void Vfg::reset(){p.reset();}
void Vfg::initialize(ID3D11Device* device,ID3D11Texture2D* texture,unsigned w,unsigned h,unsigned mode,const std::filesystem::path& runtime,bool automaticCuts){
 reset();auto q=std::make_unique<Impl>();auto& a=*q;a.w=w;a.h=h;
 a.driver=LoadLibraryExW(L"nvcuda.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!a.driver)throw std::runtime_error("CUDA driver DLL missing");
#define B(n,s) a.bind(a.driver,s,a.n##Fn)
 B(cuInit,"cuInit");B(cuD3D11GetDevice,"cuD3D11GetDevice");B(cuDevicePrimaryCtxRetain,"cuDevicePrimaryCtxRetain");B(cuDevicePrimaryCtxRelease,"cuDevicePrimaryCtxRelease_v2");B(cuCtxSetCurrent,"cuCtxSetCurrent");
 B(cuStreamCreate,"cuStreamCreate");B(cuStreamDestroy,"cuStreamDestroy_v2");B(cuStreamSynchronize,"cuStreamSynchronize");B(cuMemAlloc,"cuMemAlloc_v2");B(cuMemFree,"cuMemFree_v2");B(cuMemcpyHtoD,"cuMemcpyHtoD_v2");
 B(cuGraphicsD3D11RegisterResource,"cuGraphicsD3D11RegisterResource");B(cuGraphicsUnregisterResource,"cuGraphicsUnregisterResource");B(cuGraphicsMapResources,"cuGraphicsMapResources");B(cuGraphicsUnmapResources,"cuGraphicsUnmapResources");B(cuGraphicsSubResourceGetMappedArray,"cuGraphicsSubResourceGetMappedArray");
 B(cuMemcpy2DAsync,"cuMemcpy2DAsync_v2");B(cuEventCreate,"cuEventCreate");B(cuEventDestroy,"cuEventDestroy_v2");B(cuEventRecord,"cuEventRecord");B(cuEventQuery,"cuEventQuery");B(cuEventElapsedTime,"cuEventElapsedTime");
#undef B
 a.cu(a.cuInitFn(0),"Init");ComPtr<IDXGIDevice> gd;check(device->QueryInterface(IID_PPV_ARGS(&gd)),"DXGI device");ComPtr<IDXGIAdapter> adapter;gd->GetAdapter(&adapter);
 a.cu(a.cuD3D11GetDeviceFn(&a.gpu,adapter.Get()),"D3D CUDA adapter");a.bind(a.driver,"cuDeviceGetAttribute",a.cuDeviceGetAttributeFn);int major=0,minor=0;
 a.cu(a.cuDeviceGetAttributeFn(&major,CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR,a.gpu),"GPU architecture");a.cu(a.cuDeviceGetAttributeFn(&minor,CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR,a.gpu),"GPU architecture");
 if(!((major==8&&minor==9)||major==12))throw std::runtime_error("NVIDIA VFG requires Ada/Blackwell (RTX 40/50). Select D3D11 Optical Flow for older GPUs.");
 a.cu(a.cuDevicePrimaryCtxRetainFn(&a.ctx,a.gpu),"Context");a.cu(a.cuCtxSetCurrentFn(a.ctx),"Current context");
 a.cu(a.cuStreamCreateFn(&a.stream,CU_STREAM_NON_BLOCKING),"Stream");a.cu(a.cuEventCreateFn(&a.first,0),"Event");a.cu(a.cuEventCreateFn(&a.last,0),"Event");
 const auto folder=std::filesystem::absolute(runtime);a.directory=AddDllDirectory(folder.c_str());HMODULE core=nullptr,image=nullptr;
 for(const auto name:{L"cudart64_12.dll",L"NVCVImage.dll",L"nvngxruntime.dll",L"NVVideoEffects.dll",L"nvVFXVideoFrameGeneration.dll"}){
  auto path=folder/name;auto m=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32|LOAD_LIBRARY_SEARCH_USER_DIRS);
  if(!m)throw std::runtime_error("Cannot load "+path.string()+" error="+std::to_string(GetLastError()));a.dlls.push_back(m);
  if(std::wstring(name)==L"NVCVImage.dll")image=m;if(std::wstring(name)==L"NVVideoEffects.dll")core=m;
 }
#define V(n,s) a.bind(core,s,a.n)
 V(create,"NvVFX_CreateEffect");V(destroy,"NvVFX_DestroyEffect");V(setU32,"NvVFX_SetU32");V(setImage,"NvVFX_SetImage");V(setStream,"NvVFX_SetCudaStream");V(load,"NvVFX_Load");V(run,"NvVFX_Run");
#undef V
#define I(n,s) a.bind(image,s,a.n)
 I(imageCreate,"NvCVImage_Create");I(imageDealloc,"NvCVImage_Dealloc");I(imageDestroy,"NvCVImage_Destroy");I(imageInit,"NvCVImage_Init");I(fromD3D,"NvCVImage_FromD3DFormat");
#undef I
 int pixel=0,type=0;unsigned char layout=0;a.cv(a.fromD3D(DXGI_FORMAT_B8G8R8A8_UNORM,&pixel,&type,&layout),"Image format");
 for(int i=0;i<3;++i){CUdeviceptr& mem=i<2?a.inputs[i]:a.output;a.cu(a.cuMemAllocFn(&mem,size_t(w)*h*4),"Allocate GPU pixels");
  a.cv(a.imageCreate(1,1,pixel,type,layout,0,1,&a.images[i]),"Image descriptor");a.imageDealloc(a.images[i]);a.cv(a.imageInit(a.images[i],w,h,w*4,reinterpret_cast<void*>(mem),pixel,type,layout,1),"Bind GPU image");}
 a.cu(a.cuGraphicsD3D11RegisterResourceFn(&a.resource,texture,0),"CUDA D3D11 output");
 a.cv(a.create("VideoFrameGeneration",&a.effect),"Create VFG");a.cv(a.setStream(a.effect,"CudaStream",a.stream),"VFG stream");
 a.cv(a.setU32(a.effect,"InputWidth",w),"Width");a.cv(a.setU32(a.effect,"InputHeight",h),"Height");a.cv(a.setU32(a.effect,"Mode",mode),"Quality");
 a.cv(a.setU32(a.effect,"AutomaticShotChangeDetectionEnabled",automaticCuts?1:0),"Cut detection");
 a.cv(a.setImage(a.effect,"SrcImage0",a.images[0]),"Input A");a.cv(a.setImage(a.effect,"SrcImage1",a.images[1]),"Input B");
 a.cv(a.load(a.effect),"Load VFG model");a.cv(a.setImage(a.effect,"DstImage0",a.images[2]),"Output");a.cv(a.setU32(a.effect,"FrameMultiplier",2),"2x mode");p=std::move(q);
}
void Vfg::upload(const unsigned char* pixels,unsigned i){auto& a=*p;if(i>1||a.pending)throw std::runtime_error("Invalid or busy VFG upload");a.cu(a.cuMemcpyHtoDFn(a.inputs[i],pixels,size_t(a.w)*a.h*4),"Upload CUDA input");++a.uploadRevision;}
void Vfg::generate(unsigned current,bool cut,unsigned index,unsigned multiplier){auto& a=*p;
 if(current>1||a.pending||multiplier<2||multiplier>4||index<1||index>=multiplier)throw std::runtime_error("Invalid or busy VFG temporal position");
 if(a.multiplier!=multiplier){a.cv(a.setU32(a.effect,"FrameMultiplier",multiplier),"Frame multiplier");a.multiplier=multiplier;}
 bool newPair=a.pairBinding.needs(a.uploadRevision,current,cut,multiplier,index);
#ifdef FRAMELAB_VFG_REBIND_EACH_OUTPUT
 newPair=true; // Offline benchmark baseline only; reproduces preview.6 integration.
#endif
 if(newPair){
  a.cv(a.setImage(a.effect,"SrcImage0",a.images[1-current]),"Previous");a.cv(a.setImage(a.effect,"SrcImage1",a.images[current]),"Current");
  a.cv(a.setU32(a.effect,"ShotChange",cut?1:0),"Cut");a.pairBinding.bound(a.uploadRevision,current,cut,multiplier);
 }
 a.cv(a.setU32(a.effect,"FrameIndex",index),"Temporal position");
 a.cu(a.cuGraphicsMapResourcesFn(1,&a.resource,a.stream),"Map output");
 a.cu(a.cuEventRecordFn(a.first,a.stream),"Start GPU timer");a.cv(a.run(a.effect,1),"Run VFG");
 CUarray array=nullptr;a.cu(a.cuGraphicsSubResourceGetMappedArrayFn(&array,a.resource,0,0),"Output array");
 CUDA_MEMCPY2D copy{};copy.srcMemoryType=CU_MEMORYTYPE_DEVICE;copy.srcDevice=a.output;copy.srcPitch=a.w*4;copy.dstMemoryType=CU_MEMORYTYPE_ARRAY;copy.dstArray=array;copy.WidthInBytes=a.w*4;copy.Height=a.h;
 a.cu(a.cuMemcpy2DAsyncFn(&copy,a.stream),"GPU output copy");a.cu(a.cuGraphicsUnmapResourcesFn(1,&a.resource,a.stream),"Unmap output");a.cu(a.cuEventRecordFn(a.last,a.stream),"End GPU timer");a.pending=true;
}
bool Vfg::ready(){if(!p||!p->pending)return false;auto& a=*p;auto result=a.cuEventQueryFn(a.last);if(result==CUDA_ERROR_NOT_READY)return false;a.cu(result,"GPU completion");a.cu(a.cuEventElapsedTimeFn(&a.ms,a.first,a.last),"GPU elapsed");a.pending=false;return true;}
float Vfg::gpuMs()const{return p?p->ms:0;}
