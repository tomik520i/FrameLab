// Experimental Feature-18 contract adapted from Veyra-NRVideo, GPL-3.0.
// See THIRD_PARTY_NOTICES.md for pinned source and the separate runtime terms.
#include "NrApi.hpp"
#include "NrShaders.hpp"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "NrParameters.hpp"
using nrabi::Result; using nrabi::Parameters; using nrabi::Handle;
using ProgressCallback=void(__cdecl*)(float,bool&);
#include <filesystem>
#include <fstream>
#include <array>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <chrono>
using Microsoft::WRL::ComPtr;
namespace {
void hr(HRESULT r,const char* name){if(FAILED(r)){char b[160];snprintf(b,sizeof(b),"%s: HRESULT 0x%08X",name,unsigned(r));throw std::runtime_error(b);}}
void ngx(Result r,const char* name){if(r!=nrabi::success){char b[160];snprintf(b,sizeof(b),"%s: NGX 0x%08X",name,unsigned(r));throw std::runtime_error(b);}}
void report(char* out,unsigned size,const char* text){if(out&&size)snprintf(out,size,"%s",text);}
using InitFn=Result(__cdecl*)(unsigned long long,const wchar_t*,ID3D12Device*,unsigned,const Parameters*);
using CreateFn=Result(__cdecl*)(ID3D12GraphicsCommandList*,unsigned,Parameters*,Handle**);
using EvalFn=Result(__cdecl*)(ID3D12GraphicsCommandList*,const Handle*,const Parameters*,ProgressCallback);
using ReleaseFn=Result(__cdecl*)(Handle*);
using ShutdownFn=Result(__cdecl*)(ID3D12Device*);
// Keep SEH frames separate from functions holding C++ objects.
Result safeInit(InitFn fn,const wchar_t* path,ID3D12Device* d){__try{return fn(0x0876232Cull,path,d,0x15u,nullptr);}__except(EXCEPTION_EXECUTE_HANDLER){return nrabi::platformError;}}
Result safeCreate(CreateFn fn,ID3D12GraphicsCommandList* l,Parameters* p,Handle** h){__try{return fn(l,18u,p,h);}__except(EXCEPTION_EXECUTE_HANDLER){return nrabi::platformError;}}
Result safeEval(EvalFn fn,ID3D12GraphicsCommandList* l,Handle* h,Parameters* p){__try{return fn(l,h,p,nullptr);}__except(EXCEPTION_EXECUTE_HANDLER){return nrabi::platformError;}}
void safeRelease(ReleaseFn fn,Handle* h){__try{fn(h);}__except(EXCEPTION_EXECUTE_HANDLER){}}
void safeShutdown(ShutdownFn fn,ID3D12Device* d){__try{fn(d);}__except(EXCEPTION_EXECUTE_HANDLER){}}
Result __cdecl scaling(Parameters* p){p->Set("DLSSNR.ScalingRatio",1.f);return nrabi::success;}
struct Resource {ComPtr<ID3D12Resource> value;D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COMMON;};
struct Host {
 NrConfig config{};std::wstring runtime,cache;std::ofstream log;
 ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;
 std::array<ComPtr<ID3D12CommandAllocator>,3> alloc;
 std::array<ComPtr<ID3D12GraphicsCommandList>,3> lists;
 ComPtr<ID3D12Fence> fence,inputFence;uint64_t serial=0;HANDLE event=nullptr;
 ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12DescriptorHeap> heap;
 ComPtr<ID3D12PipelineState> prepare,flow,guides,composite;
 Resource input,output,proxy,previous,neural,smallFlow,motion,depth;
 HMODULE dll=nullptr;InitFn initFn=nullptr;CreateFn createFn=nullptr;EvalFn evalFn=nullptr;ReleaseFn releaseFn=nullptr;ShutdownFn shutdownFn=nullptr;
 Parameters parameterStorage;Parameters* params=&parameterStorage;Handle* feature=nullptr;bool initialized=false,history=false;
 UINT descriptorSize=0;
 void transition(ID3D12GraphicsCommandList* l,Resource& r,D3D12_RESOURCE_STATES s){if(r.state==s)return;D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=r.value.Get();b.Transition.StateBefore=r.state;b.Transition.StateAfter=s;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;l->ResourceBarrier(1,&b);r.state=s;}
 void uav(ID3D12GraphicsCommandList* l,Resource& r){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=r.value.Get();l->ResourceBarrier(1,&b);}
 Resource texture(UINT w,UINT h,DXGI_FORMAT f){Resource r;D3D12_HEAP_PROPERTIES p{};p.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=w;d.Height=h;d.DepthOrArraySize=1;d.MipLevels=1;d.Format=f;d.SampleDesc.Count=1;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;hr(device->CreateCommittedResource(&p,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&r.value)),"NR texture");return r;}
 ID3D12GraphicsCommandList* begin(unsigned i){hr(alloc[i]->Reset(),"NR allocator reset");hr(lists[i]->Reset(alloc[i].Get(),nullptr),"NR list reset");return lists[i].Get();}
 void submit(ID3D12GraphicsCommandList* l){hr(l->Close(),"NR list close");ID3D12CommandList* a[]={l};queue->ExecuteCommandLists(1,a);}
 void wait(){const auto value=++serial;hr(queue->Signal(fence.Get(),value),"NR signal");if(fence->GetCompletedValue()<value){hr(fence->SetEventOnCompletion(value,event),"NR completion");if(WaitForSingleObject(event,20000)!=WAIT_OBJECT_0)throw std::runtime_error("NR GPU timeout (20 s)");}hr(device->GetDeviceRemovedReason(),"NR device status");}
 ComPtr<ID3D12PipelineState> shader(const char* name){ComPtr<ID3DBlob> code,error;auto result=D3DCompile(nrShaders,sizeof(nrShaders)-1,"FrameLab NR",nullptr,nullptr,name,"cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error);if(FAILED(result))throw std::runtime_error(error?std::string((char*)error->GetBufferPointer()):"NR shader compilation failed");D3D12_COMPUTE_PIPELINE_STATE_DESC p{};p.pRootSignature=root.Get();p.CS={code->GetBufferPointer(),code->GetBufferSize()};ComPtr<ID3D12PipelineState> s;hr(device->CreateComputePipelineState(&p,IID_PPV_ARGS(&s)),"NR shader");return s;}
 void bind(ID3D12GraphicsCommandList* l,ID3D12PipelineState* p,bool reset,bool useMotion){ID3D12DescriptorHeap* h[]={heap.Get()};l->SetDescriptorHeaps(1,h);l->SetComputeRootSignature(root.Get());l->SetPipelineState(p);auto gpu=heap->GetGPUDescriptorHandleForHeapStart();l->SetComputeRootDescriptorTable(0,gpu);gpu.ptr+=descriptorSize*5;l->SetComputeRootDescriptorTable(1,gpu);unsigned c[]={config.inferenceWidth,config.inferenceHeight,config.width,config.height,unsigned(reset),unsigned(useMotion),0,0};l->SetComputeRoot32BitConstants(2,8,c,0);}
 void setup(const NrConfig& c){
  if(c.size!=sizeof(c)||!c.width||!c.height||!c.inferenceWidth||!c.inferenceHeight||c.width>7680||c.height>4320)throw std::runtime_error("Invalid NR configuration");
  config=c;runtime=c.runtimeDirectory;cache=c.cacheDirectory;std::filesystem::create_directories(cache);log.open(std::filesystem::path(cache)/L"nr-host.log",std::ios::app);log<<"Initializing "<<c.width<<"x"<<c.height<<" inference "<<c.inferenceWidth<<"x"<<c.inferenceHeight<<std::endl;
  ComPtr<IDXGIFactory4> factory;hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"NR factory");ComPtr<IDXGIAdapter1> adapter;hr(factory->EnumAdapterByLuid(c.adapter,IID_PPV_ARGS(&adapter)),"NR adapter LUID");hr(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"NR D3D12 device");
  D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;hr(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"NR queue");
  for(unsigned i=0;i<3;++i){hr(device->CreateCommandAllocator(q.Type,IID_PPV_ARGS(&alloc[i])),"NR allocator");hr(device->CreateCommandList(0,q.Type,alloc[i].Get(),nullptr,IID_PPV_ARGS(&lists[i])),"NR list");hr(lists[i]->Close(),"NR initial close");}
  hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"NR fence");event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("NR event failed");
  hr(device->OpenSharedHandle(c.input,IID_PPV_ARGS(&input.value)),"NR shared input");hr(device->OpenSharedHandle(c.output,IID_PPV_ARGS(&output.value)),"NR shared output");hr(device->OpenSharedHandle(c.inputFence,IID_PPV_ARGS(&inputFence)),"NR shared fence");
  auto w=c.inferenceWidth,h=c.inferenceHeight;proxy=texture(w,h,DXGI_FORMAT_R8G8B8A8_UNORM);previous=texture(w,h,DXGI_FORMAT_R8G8B8A8_UNORM);neural=texture(w,h,DXGI_FORMAT_R8G8B8A8_UNORM);smallFlow=texture((w+15)/16,(h+15)/16,DXGI_FORMAT_R16G16_FLOAT);motion=texture(w,h,DXGI_FORMAT_R16G16_FLOAT);depth=texture(w,h,DXGI_FORMAT_R32_FLOAT);
  D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,5,0,0,0};ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,5,0,0,0};D3D12_ROOT_PARAMETER rp[3]{};
  for(int i=0;i<2;++i){rp[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;rp[i].DescriptorTable={1,&ranges[i]};}rp[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;rp[2].Constants={0,0,8};
  D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sampler.MaxLOD=D3D12_FLOAT32_MAX;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
  D3D12_ROOT_SIGNATURE_DESC rs{3,rp,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;hr(D3D12SerializeRootSignature(&rs,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"NR root serialization");hr(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)),"NR root");
  D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=10;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;hr(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)),"NR descriptors");descriptorSize=device->GetDescriptorHandleIncrementSize(hd.Type);
  Resource* srvs[]={&input,&proxy,&previous,&smallFlow,&neural};Resource* uavs[]={&proxy,&smallFlow,&motion,&depth,&output};auto cpu=heap->GetCPUDescriptorHandleForHeapStart();
  for(auto r:srvs){D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=r->value->GetDesc().Format;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;device->CreateShaderResourceView(r->value.Get(),&v,cpu);cpu.ptr+=descriptorSize;}
  for(auto r:uavs){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=r->value->GetDesc().Format;v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(r->value.Get(),nullptr,&v,cpu);cpu.ptr+=descriptorSize;}
  prepare=shader("prepare");flow=shader("flow");guides=shader("guides");composite=shader("composite");
  auto path=std::filesystem::absolute(std::filesystem::path(runtime)/L"nvngx_dlssnr.dll");dll=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);if(!dll)throw std::runtime_error("Cannot load NR runtime, Win32 error "+std::to_string(GetLastError()));
  initFn=reinterpret_cast<InitFn>(GetProcAddress(dll,"NVSDK_NGX_D3D12_Init_Ext"));createFn=reinterpret_cast<CreateFn>(GetProcAddress(dll,"NVSDK_NGX_D3D12_CreateFeature"));evalFn=reinterpret_cast<EvalFn>(GetProcAddress(dll,"NVSDK_NGX_D3D12_EvaluateFeature"));releaseFn=reinterpret_cast<ReleaseFn>(GetProcAddress(dll,"NVSDK_NGX_D3D12_ReleaseFeature"));shutdownFn=reinterpret_cast<ShutdownFn>(GetProcAddress(dll,"NVSDK_NGX_D3D12_Shutdown1"));
  if(!initFn||!createFn||!evalFn||!releaseFn||!shutdownFn)throw std::runtime_error("Incomplete NR runtime exports");
  ngx(safeInit(initFn,cache.c_str(),device.Get()),"NR init");initialized=true;
  for(auto name:{"DLSSNR.Width","DLSSNR.InputWidth","DLSSNR.OutputWidth","DLSSNR.Output.Width","Width"})params->Set(name,w);
  for(auto name:{"DLSSNR.Height","DLSSNR.InputHeight","DLSSNR.OutputHeight","DLSSNR.Output.Height","Height"})params->Set(name,h);
  params->Set("DLSSNR.Upscaling",0u);params->Set("DLSSNR.Scale",1.f);params->Set("DLSSNR.ScalingRatio",1.f);params->Set("DLSSNRComputeScalingRatioCallback",reinterpret_cast<void*>(&scaling));params->Set("DLSSNR.Hint.Render.Preset",2);params->Set("PerfQualityValue",1);params->Set("CreationNodeMask",1u);params->Set("VisibilityNodeMask",1u);
  auto l=begin(0);ngx(safeCreate(createFn,l,params,&feature),"NR create feature 18");if(!feature)throw std::runtime_error("NR returned no feature");submit(l);wait();log<<"Feature 18 created"<<std::endl;
 }
 void process(const NrFrame& frame,float& milliseconds){
  const auto start=std::chrono::steady_clock::now();bool reset=frame.reset||!history||!frame.useMotion;auto w=config.inferenceWidth,h=config.inferenceHeight;
  hr(queue->Wait(inputFence.Get(),frame.inputFenceValue),"NR wait for D3D11 input");
  auto l=begin(0);transition(l,input,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);transition(l,proxy,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  bind(l,prepare.Get(),reset,frame.useMotion);l->Dispatch((w+7)/8,(h+7)/8,1);uav(l,proxy);transition(l,proxy,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);transition(l,previous,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);transition(l,smallFlow,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  bind(l,flow.Get(),reset,frame.useMotion);l->Dispatch((((w+15)/16)+7)/8,(((h+15)/16)+7)/8,1);uav(l,smallFlow);transition(l,smallFlow,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);transition(l,motion,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);transition(l,depth,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  bind(l,guides.Get(),reset,frame.useMotion);l->Dispatch((w+7)/8,(h+7)/8,1);uav(l,motion);uav(l,depth);transition(l,motion,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);transition(l,depth,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);transition(l,neural,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);submit(l);
  params->Set("DLSSNR.Color",proxy.value.Get());params->Set("DLSSNR.Output",neural.value.Get());params->Set("DLSSNR.MVec",motion.value.Get());params->Set("DLSSNR.Depth",depth.value.Get());
  for(auto name:{"DLSSNR.ColorSubrectWidth","DLSSNR.OutputSubrectWidth","DLSSNR.MVecSubrectWidth","DLSSNR.DepthSubrectWidth"})params->Set(name,w);
  for(auto name:{"DLSSNR.ColorSubrectHeight","DLSSNR.OutputSubrectHeight","DLSSNR.MVecSubrectHeight","DLSSNR.DepthSubrectHeight"})params->Set(name,h);
  for(auto name:{"DLSSNR.ColorSubrectBaseX","DLSSNR.ColorSubrectBaseY","DLSSNR.OutputSubrectBaseX","DLSSNR.OutputSubrectBaseY","DLSSNR.MVecSubrectBaseX","DLSSNR.MVecSubrectBaseY","DLSSNR.DepthSubrectBaseX","DLSSNR.DepthSubrectBaseY"})params->Set(name,0u);
  params->Set("DLSSNR.MVecScaleX",1.f);params->Set("DLSSNR.MVecScaleY",1.f);params->Set("DLSSNR.DepthInverted",1);params->Set("DLSS.Indicator.Invert.X.Axis",0);params->Set("DLSS.Indicator.Invert.Y.Axis",0);params->Set("DLSSNR.Enabled",1);params->Set("DLSSNR.Reset",reset?1:0);params->Set("DLSSNR.Style",0);params->Set("DLSSNR.Intensity",1.f);params->Set("DLSSNR.LocalToneStrength",1.f);params->Set("DLSSNR.LocalStructureStrength",1.f);params->Set("DLSSNR.SkinStructureStrength",.5f);params->Set("DLSSNR.UseAutoMask",1);params->Set("DLSSNR.UICorrection",1);
  l=begin(1);ngx(safeEval(evalFn,l,feature,params),"NR evaluate");uav(l,neural);transition(l,neural,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);transition(l,output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  bind(l,composite.Get(),reset,frame.useMotion);l->Dispatch((config.width+7)/8,(config.height+7)/8,1);uav(l,output);transition(l,output,D3D12_RESOURCE_STATE_COMMON);transition(l,input,D3D12_RESOURCE_STATE_COMMON);
  transition(l,proxy,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(l,previous,D3D12_RESOURCE_STATE_COPY_DEST);l->CopyResource(previous.value.Get(),proxy.value.Get());submit(l);wait();history=true;
  milliseconds=std::chrono::duration<float,std::milli>(std::chrono::steady_clock::now()-start).count();if(frame.inputFenceValue<=12||frame.inputFenceValue%120==0||reset)log<<"frame "<<frame.inputFenceValue<<" reset="<<reset<<" work_ms="<<milliseconds<<std::endl;
 }
 ~Host(){
  if(queue&&fence&&event){try{wait();}catch(...){}}
  if(feature&&releaseFn)safeRelease(releaseFn,feature);
  if(initialized&&shutdownFn)safeShutdown(shutdownFn,device.Get());
  if(dll)FreeLibrary(dll);if(event)CloseHandle(event);
 }
};
}
extern "C" __declspec(dllexport) void* __cdecl NrCreate(const NrConfig* c,char* error,unsigned capacity){try{auto h=std::make_unique<Host>();h->setup(*c);return h.release();}catch(const std::exception& e){report(error,capacity,e.what());return nullptr;}}
extern "C" __declspec(dllexport) bool __cdecl NrProcess(void* instance,const NrFrame* frame,float* ms,char* error,unsigned capacity){try{static_cast<Host*>(instance)->process(*frame,*ms);return true;}catch(const std::exception& e){report(error,capacity,e.what());return false;}}
extern "C" __declspec(dllexport) void __cdecl NrDestroy(void* instance){delete static_cast<Host*>(instance);}
