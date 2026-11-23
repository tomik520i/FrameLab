#include "Graphics.hpp"
#include <algorithm>
#include <vector>
void Texture::create(ID3D11Device* d,unsigned w,unsigned h){
 view.Reset();texture.Reset();D3D11_TEXTURE2D_DESC desc{};
 desc.Width=w;desc.Height=h;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
 desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
 check(d->CreateTexture2D(&desc,nullptr,&texture),"CreateTexture2D");
 check(d->CreateShaderResourceView(texture.Get(),nullptr,&view),"CreateShaderResourceView");
}
void Graphics::initDevice(int requestedAdapter){
 ComPtr<IDXGIFactory2> factory;check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");
 struct Candidate {ComPtr<IDXGIAdapter1> adapter;DXGI_ADAPTER_DESC1 desc;unsigned index;};
 std::vector<Candidate> candidates;
 for(unsigned i=0;;++i){ComPtr<IDXGIAdapter1> adapter;auto hr=factory->EnumAdapters1(i,&adapter);if(hr==DXGI_ERROR_NOT_FOUND)break;check(hr,"Enumerate GPU");DXGI_ADAPTER_DESC1 desc{};check(adapter->GetDesc1(&desc),"GPU description");
  if((requestedAdapter<0||i==unsigned(requestedAdapter))&&!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE))candidates.push_back({adapter,desc,i});
 }
 // Prefer a discrete card, without requiring a particular vendor. An explicit
 // DXGI index permits reproducible tests on integrated or secondary adapters.
 std::stable_sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.desc.DedicatedVideoMemory>b.desc.DedicatedVideoMemory;});
 const D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0};
 for(auto& candidate:candidates){
  auto hr=D3D11CreateDevice(candidate.adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,levels,1,D3D11_SDK_VERSION,&device,&featureLevel,&context);
  if(FAILED(hr)){device.Reset();context.Reset();continue;}
  adapterVendor=candidate.desc.VendorId;adapterIndex=candidate.index;
  const int count=WideCharToMultiByte(CP_UTF8,0,candidate.desc.Description,-1,nullptr,0,nullptr,nullptr);std::vector<char> name(count);WideCharToMultiByte(CP_UTF8,0,candidate.desc.Description,-1,name.data(),count,nullptr,nullptr);adapterName=name.data();return;
 }
 throw std::runtime_error(requestedAdapter<0?"No hardware GPU with Direct3D feature level 11_0 is available":"Selected DXGI adapter is unavailable or does not support Direct3D feature level 11_0");
}
void Graphics::init(HWND window,int requestedAdapter){
 initDevice(requestedAdapter);
 ComPtr<IDXGIFactory2> factory;check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");
 ComPtr<IDXGIFactory5> f5;if(SUCCEEDED(factory.As(&f5))){BOOL allowed=FALSE;f5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,&allowed,sizeof(allowed));tearing=allowed;}
 DXGI_SWAP_CHAIN_DESC1 desc{};desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;
 desc.Scaling=DXGI_SCALING_NONE;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;desc.Flags=DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT|(tearing?DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING:0);
 ComPtr<IDXGISwapChain1> s;check(factory->CreateSwapChainForHwnd(device.Get(),window,&desc,nullptr,nullptr,&s),"CreateSwapChain");check(s.As(&swap),"SwapChain2");
 factory->MakeWindowAssociation(window,DXGI_MWA_NO_ALT_ENTER);swap->SetMaximumFrameLatency(1);waitable=swap->GetFrameLatencyWaitableObject();resize(0,0);
}
void Graphics::resize(unsigned w,unsigned h){
 context->OMSetRenderTargets(0,nullptr,nullptr);target.Reset();
 if(w&&h)check(swap->ResizeBuffers(0,w,h,DXGI_FORMAT_UNKNOWN,DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT|(tearing?DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING:0)),"ResizeBuffers");
 ComPtr<ID3D11Texture2D> b;check(swap->GetBuffer(0,IID_PPV_ARGS(&b)),"Backbuffer");check(device->CreateRenderTargetView(b.Get(),nullptr,&target),"RTV");
}
void Graphics::begin(){const float c[]={.025f,.035f,.055f,1};context->OMSetRenderTargets(1,target.GetAddressOf(),nullptr);context->ClearRenderTargetView(target.Get(),c);}
HRESULT Graphics::present(bool v){return swap->Present(v?1:0,(!v&&tearing)?DXGI_PRESENT_ALLOW_TEARING:0);}
Graphics::~Graphics(){if(context)context->ClearState();if(waitable)CloseHandle(waitable);}
