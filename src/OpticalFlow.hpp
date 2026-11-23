#pragma once
#include "Graphics.hpp"
#include <d3dcompiler.h>
#include <algorithm>
#include <vector>
#include <cstring>
#include <memory>

// SM5-only, vendor-independent hierarchical bidirectional block matching.
// Uncertain motion falls back to an original image instead of arbitrary warping.
class OpticalFlow {
 struct Surface {
  ComPtr<ID3D11Texture2D> texture;ComPtr<ID3D11ShaderResourceView> view;ComPtr<ID3D11UnorderedAccessView> write;unsigned w=0,h=0;
  void create(ID3D11Device* d,unsigned width,unsigned height,DXGI_FORMAT format){
   w=width;h=height;D3D11_TEXTURE2D_DESC desc{};desc.Width=w;desc.Height=h;desc.MipLevels=desc.ArraySize=1;desc.Format=format;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;
   check(d->CreateTexture2D(&desc,nullptr,&texture),"Optical texture");check(d->CreateShaderResourceView(texture.Get(),nullptr,&view),"Optical SRV");check(d->CreateUnorderedAccessView(texture.Get(),nullptr,&write),"Optical UAV");
  }
 };
 struct Level {Surface a,b,flow;};
 struct Params {float size[2],coarseSize[2],timestep,blockSize,radius,patchRadius,coarsest,cut,automaticCuts,pad;};
 ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;Texture inputs[2];std::vector<Level> levels;Surface scene;
 ComPtr<ID3D11RenderTargetView> target;ComPtr<ID3D11ComputeShader> luma,downsample,estimate,detectCut;
 ComPtr<ID3D11VertexShader> vs;ComPtr<ID3D11PixelShader> ps;ComPtr<ID3D11Buffer> constants;ComPtr<ID3D11SamplerState> sampler;
 ComPtr<ID3D11Query> done,beginTime,endTime,disjoint;
 unsigned w=0,h=0,quality=1,block=8;bool pending=false,motionValid=false,automaticCuts=true;float ms=-1;
 static constexpr const char* shader=R"HLSL(
cbuffer Params:register(b0){float2 size;float2 coarseSize;float timestep;float blockSize;float radius;float patchRadius;float coarsest;float cut;float automaticCuts;float pad;};
Texture2D<float4> A:register(t0);Texture2D<float4> B:register(t1);Texture2D<float4> Flow:register(t2);Texture2D<float4> Coarse:register(t3);Texture2D<float> Scene:register(t4);
RWTexture2D<float4> OutFlow:register(u0);RWTexture2D<float> OutA:register(u1);RWTexture2D<float> OutB:register(u2);RWTexture2D<float> OutScene:register(u3);
SamplerState linearClamp:register(s0);
float lum(float3 c){return dot(c,float3(.299,.587,.114));}
[numthreads(8,8,1)]void lumaMain(uint3 id:SV_DispatchThreadID){if(any(id.xy>=uint2(size)))return;OutA[id.xy]=lum(A.Load(int3(id.xy,0)).rgb);OutB[id.xy]=lum(B.Load(int3(id.xy,0)).rgb);}
[numthreads(8,8,1)]void downsampleMain(uint3 id:SV_DispatchThreadID){
 if(any(id.xy>=uint2(size)))return;float a=0,b=0;
 [unroll]for(int y=0;y<2;++y)[unroll]for(int x=0;x<2;++x){int2 p=min(int2(id.xy)*2+int2(x,y),int2(coarseSize)-1);a+=A.Load(int3(p,0)).r;b+=B.Load(int3(p,0)).r;}
 OutA[id.xy]=a*.25;OutB[id.xy]=b*.25;
}
float errorAt(float2 p,float2 delta,bool reverse){
 float e=0;int r=int(patchRadius);
 [loop]for(int y=-r;y<=r;++y)[loop]for(int x=-r;x<=r;++x){
  float2 q=p+float2(x,y);float2 uv=(q+.5)/size,other=(q+delta+.5)/size;
  float a=reverse?B.SampleLevel(linearClamp,uv,0).r:A.SampleLevel(linearClamp,uv,0).r;
  float b=reverse?A.SampleLevel(linearClamp,other,0).r:B.SampleLevel(linearClamp,other,0).r;e+=abs(a-b);
 }
 return e/((2*r+1)*(2*r+1));
}
float2 match(float2 p,bool reverse){
 float2 prior=0;
 if(coarsest<.5){float4 f=Coarse.SampleLevel(linearClamp,(p*.5)/(ceil(coarseSize/blockSize)*blockSize),0);prior=round((reverse?f.zw:f.xy)*2);}
 float2 bestMotion=prior;float best=errorAt(p,prior,reverse);float zero=errorAt(p,0,reverse);
 if(zero<best-.001){best=zero;bestMotion=0;}
 float2 center=prior;int r=coarsest>.5?4:int(radius);
 [loop]for(int y=-r;y<=r;++y)[loop]for(int x=-r;x<=r;++x){float2 d=center+float2(x,y);float e=errorAt(p,d,reverse)+.000025*length(d);if(e<best){best=e;bestMotion=d;}}
 // Fixed HUD and independently moving foreground need a zero-motion candidate.
 [loop]for(int y=-1;y<=1;++y)[loop]for(int x=-1;x<=1;++x){float2 d=float2(x,y);float e=errorAt(p,d,reverse);if(e<best-.001){best=e;bestMotion=d;}}
 return bestMotion;
}
[numthreads(8,8,1)]void flowMain(uint3 id:SV_DispatchThreadID){if(any(id.xy>=uint2(ceil(size/blockSize))))return;float2 p=min(float2(id.xy)*blockSize+blockSize*.5,size-1);OutFlow[id.xy]=float4(match(p,false),match(p,true));}
groupshared float cutDifference[64];
groupshared uint histogramA[48];
groupshared uint histogramB[48];
[numthreads(8,8,1)]void cutMain(uint3 id:SV_GroupThreadID,uint index:SV_GroupIndex){
 if(index<48){histogramA[index]=0;histogramB[index]=0;}GroupMemoryBarrierWithGroupSync();
 float difference=0;
 [unroll]for(uint sample=0;sample<4;++sample){float2 uv=(float2(id.xy)*2+float2(sample%2,sample/2)+.5)/16;
  float3 a=A.SampleLevel(linearClamp,uv,0).rgb,b=B.SampleLevel(linearClamp,uv,0).rgb;
  difference+=dot(abs(a-b),float3(.333333,.333333,.333333));
  [unroll]for(uint c=0;c<3;++c){InterlockedAdd(histogramA[c*16+min(uint(a[c]*16),15)],1);InterlockedAdd(histogramB[c*16+min(uint(b[c]*16),15)],1);}
 }
 cutDifference[index]=difference*.25;GroupMemoryBarrierWithGroupSync();
 // A pan changes pixels at fixed coordinates, but usually preserves the colour distribution.
 if(index==0){float total=0,histogram=0;[unroll]for(int i=0;i<64;++i)total+=cutDifference[i];[unroll]for(int j=0;j<48;++j)histogram+=abs(int(histogramA[j])-int(histogramB[j]));OutScene[uint2(0,0)]=(total/64>.18&&histogram/1536>.45)?1:0;}
}
float4 motion(float2 p){return Flow.SampleLevel(linearClamp,(p-.5)/(ceil(size/blockSize)*blockSize),0);}
float rgbError(float3 a,float3 b){float3 d=abs(a-b);return max(d.x,max(d.y,d.z));}
bool inside(float2 p){return all(p>=.5)&&all(p<=size-.5);}
struct V{float4 pos:SV_POSITION;};
V vertexMain(uint id:SV_VertexID){V v;float2 uv=float2((id<<1)&2,id&2);v.pos=float4(uv*float2(2,-2)+float2(-1,1),0,1);return v;}
float4 pixelMain(V v):SV_TARGET{
 float2 p=v.pos.xy,uv=p/size;float3 rawA=A.SampleLevel(linearClamp,uv,0).rgb,rawB=B.SampleLevel(linearClamp,uv,0).rgb;
 if(cut>.5||(automaticCuts>.5&&Scene.Load(int3(0,0,0))>.5))return float4(rawB,1);
 if(timestep<=0)return float4(rawA,1);if(timestep>=1)return float4(rawB,1);
 if(rgbError(rawA,rawB)<.008)return float4(lerp(rawA,rawB,timestep),1);
 float2 pa=p,pb=p;
 [unroll]for(int k=0;k<3;++k){pa=p-timestep*motion(pa).xy;pb=p-(1-timestep)*motion(pb).zw;}
 float2 fa=motion(pa).xy,fb=motion(pb).zw;
 float3 a=A.SampleLevel(linearClamp,pa/size,0).rgb,b=B.SampleLevel(linearClamp,pb/size,0).rgb;
 float3 ba=B.SampleLevel(linearClamp,(pa+fa)/size,0).rgb,ab=A.SampleLevel(linearClamp,(pb+fb)/size,0).rgb;
 float ca=saturate(1-length(fa+motion(pa+fa).zw)/(1.5+.06*length(fa)))*saturate(1-rgbError(a,ba)/.10);
 float cb=saturate(1-length(fb+motion(pb+fb).xy)/(1.5+.06*length(fb)))*saturate(1-rgbError(b,ab)/.10);
 ca*=inside(pa)&&inside(pa+fa);cb*=inside(pb)&&inside(pb+fb);
 float3 fallback=timestep<.5?rawA:rawB;
 float both=smoothstep(.25,.75,min(ca,cb))*saturate(1-rgbError(a,b)/.12);
 float oneA=smoothstep(.60,.95,ca)*smoothstep(.25,.6,ca-cb);
 float oneB=smoothstep(.60,.95,cb)*smoothstep(.25,.6,cb-ca);
 float3 interpolated=lerp(a,b,timestep);float confidence=both;
 if(oneA>confidence){interpolated=a;confidence=oneA;}if(oneB>confidence){interpolated=b;confidence=oneB;}
 return float4(lerp(fallback,interpolated,confidence),1);
}
)HLSL";
 ComPtr<ID3DBlob> compile(const char* entry,const char* model){ComPtr<ID3DBlob> code,error;auto hr=D3DCompile(shader,strlen(shader),"FrameLab portable optical flow",nullptr,nullptr,entry,model,D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error);if(FAILED(hr))throw std::runtime_error(error?std::string((char*)error->GetBufferPointer()):"Optical shader compilation failed");return code;}
 void params(unsigned width,unsigned height,unsigned cw,unsigned ch,float time,bool cut,bool coarsest=false){Params p{{float(width),float(height)},{float(cw),float(ch)},time,float(block),quality==2?3.f:2.f,quality==0?1.f:2.f,coarsest?1.f:0.f,cut?1.f:0.f,automaticCuts?1.f:0.f,0};context->UpdateSubresource(constants.Get(),0,nullptr,&p,0,0);}
 void unbindCompute(){ID3D11ShaderResourceView* views[5]{};ID3D11UnorderedAccessView* writes[4]{};context->CSSetShaderResources(0,5,views);context->CSSetUnorderedAccessViews(0,4,writes,nullptr);context->CSSetShader(nullptr,nullptr,0);}
 void estimateMotion(unsigned current){
  ID3D11ShaderResourceView* source[]={inputs[1-current].view.Get(),inputs[current].view.Get()};
  params(w,h,0,0,0,false);context->CSSetShader(detectCut.Get(),nullptr,0);context->CSSetShaderResources(0,2,source);context->CSSetUnorderedAccessViews(3,1,scene.write.GetAddressOf(),nullptr);context->Dispatch(1,1,1);unbindCompute();
  for(size_t i=0;i<levels.size();++i){auto& l=levels[i];ID3D11ShaderResourceView* views[]={i?levels[i-1].a.view.Get():source[0],i?levels[i-1].b.view.Get():source[1]};ID3D11UnorderedAccessView* writes[]={l.a.write.Get(),l.b.write.Get()};
   params(l.a.w,l.a.h,i?levels[i-1].a.w:0,i?levels[i-1].a.h:0,0,false);context->CSSetShader(i?downsample.Get():luma.Get(),nullptr,0);context->CSSetShaderResources(0,2,views);context->CSSetUnorderedAccessViews(1,2,writes,nullptr);context->Dispatch((l.a.w+7)/8,(l.a.h+7)/8,1);unbindCompute();
  }
  for(int i=int(levels.size())-1;i>=0;--i){auto& l=levels[i];bool top=i==int(levels.size())-1;ID3D11ShaderResourceView* views[]={l.a.view.Get(),l.b.view.Get(),nullptr,top?nullptr:levels[i+1].flow.view.Get()};
   params(l.a.w,l.a.h,top?0:levels[i+1].a.w,top?0:levels[i+1].a.h,0,false,top);context->CSSetShader(estimate.Get(),nullptr,0);context->CSSetShaderResources(0,4,views);context->CSSetUnorderedAccessViews(0,1,l.flow.write.GetAddressOf(),nullptr);context->Dispatch((l.flow.w+7)/8,(l.flow.h+7)/8,1);unbindCompute();
  }
  motionValid=true;
 }
public:
 void initialize(ID3D11Device* d,ID3D11Texture2D* output,unsigned width,unsigned height,unsigned mode=1,bool cuts=true){
  if(!width||!height||d->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0)throw std::runtime_error("Optical Flow requires nonempty frames and Direct3D feature level 11_0");
  device=d;device->GetImmediateContext(&context);w=width;h=height;quality=std::min(mode,2u);block=quality==0?16:8;automaticCuts=cuts;pending=motionValid=false;ms=-1;
  for(auto& t:inputs)t.create(d,w,h);levels.clear();unsigned lw=w,lh=h;
  for(unsigned i=0;i<5;++i){levels.emplace_back();auto& l=levels.back();l.a.create(d,lw,lh,DXGI_FORMAT_R32_FLOAT);l.b.create(d,lw,lh,DXGI_FORMAT_R32_FLOAT);l.flow.create(d,(lw+block-1)/block,(lh+block-1)/block,DXGI_FORMAT_R32G32B32A32_FLOAT);if(lw<=16||lh<=16)break;lw=(lw+1)/2;lh=(lh+1)/2;}
  scene.create(d,1,1,DXGI_FORMAT_R32_FLOAT);check(d->CreateRenderTargetView(output,nullptr,&target),"Optical output RTV");
  for(auto item:{std::pair{"lumaMain",std::addressof(luma)},std::pair{"downsampleMain",std::addressof(downsample)},std::pair{"flowMain",std::addressof(estimate)},std::pair{"cutMain",std::addressof(detectCut)}}){auto c=compile(item.first,"cs_5_0");check(d->CreateComputeShader(c->GetBufferPointer(),c->GetBufferSize(),nullptr,item.second->ReleaseAndGetAddressOf()),"Optical CS");}
  auto c=compile("vertexMain","vs_5_0");check(d->CreateVertexShader(c->GetBufferPointer(),c->GetBufferSize(),nullptr,&vs),"Optical VS");c=compile("pixelMain","ps_5_0");check(d->CreatePixelShader(c->GetBufferPointer(),c->GetBufferSize(),nullptr,&ps),"Optical PS");
  D3D11_BUFFER_DESC bd{};bd.ByteWidth=sizeof(Params);bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;check(d->CreateBuffer(&bd,nullptr,&constants),"Optical constants");
  D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;check(d->CreateSamplerState(&sd,&sampler),"Optical sampler");
  for(auto item:{std::pair{D3D11_QUERY_EVENT,std::addressof(done)},std::pair{D3D11_QUERY_TIMESTAMP,std::addressof(beginTime)},std::pair{D3D11_QUERY_TIMESTAMP,std::addressof(endTime)},std::pair{D3D11_QUERY_TIMESTAMP_DISJOINT,std::addressof(disjoint)}}){D3D11_QUERY_DESC q{item.first,0};check(d->CreateQuery(&q,item.second->ReleaseAndGetAddressOf()),"Optical query");}
 }
 void upload(const unsigned char* data,unsigned parity){if(!data||parity>1||pending)throw std::runtime_error("Invalid or busy optical input");context->UpdateSubresource(inputs[parity].texture.Get(),0,nullptr,data,w*4,0);motionValid=false;}
 void upload(ID3D11Texture2D* texture,unsigned parity){
  if(!texture||parity>1||pending)throw std::runtime_error("Invalid or busy optical input");D3D11_TEXTURE2D_DESC d{};texture->GetDesc(&d);if(d.Width!=w||d.Height!=h||d.Format!=DXGI_FORMAT_B8G8R8A8_UNORM)throw std::runtime_error("Optical input format mismatch");context->CopyResource(inputs[parity].texture.Get(),texture);motionValid=false;
 }
 void generate(unsigned current,bool cut,unsigned index,unsigned multiplier){
  if(pending||current>1||multiplier<2||index>=multiplier)throw std::runtime_error("Invalid or busy optical generation");
  context->Begin(disjoint.Get());context->End(beginTime.Get());context->CSSetConstantBuffers(0,1,constants.GetAddressOf());context->CSSetSamplers(0,1,sampler.GetAddressOf());if(!motionValid&&!cut)estimateMotion(current);
  params(w,h,0,0,float(index)/multiplier,cut);
  ID3D11ShaderResourceView* views[]={inputs[1-current].view.Get(),inputs[current].view.Get(),levels[0].flow.view.Get(),nullptr,scene.view.Get()},*nullViews[5]{};
  context->OMSetRenderTargets(1,target.GetAddressOf(),nullptr);context->OMSetBlendState(nullptr,nullptr,~0u);context->OMSetDepthStencilState(nullptr,0);context->RSSetState(nullptr);
  D3D11_VIEWPORT vp{0,0,float(w),float(h),0,1};context->RSSetViewports(1,&vp);context->IASetInputLayout(nullptr);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(vs.Get(),nullptr,0);context->PSSetShader(ps.Get(),nullptr,0);context->PSSetConstantBuffers(0,1,constants.GetAddressOf());context->PSSetSamplers(0,1,sampler.GetAddressOf());context->PSSetShaderResources(0,5,views);context->Draw(3,0);context->PSSetShaderResources(0,5,nullViews);context->OMSetRenderTargets(0,nullptr,nullptr);
  context->End(endTime.Get());context->End(disjoint.Get());context->End(done.Get());context->Flush();pending=true;
 }
 bool ready(){if(!pending)return false;BOOL complete=FALSE;HRESULT hr=context->GetData(done.Get(),&complete,sizeof(complete),0);check(hr,"Optical completion");if(hr!=S_OK||!complete)return false;UINT64 a=0,b=0;D3D11_QUERY_DATA_TIMESTAMP_DISJOINT d{};if(context->GetData(disjoint.Get(),&d,sizeof(d),0)==S_OK&&!d.Disjoint&&context->GetData(beginTime.Get(),&a,sizeof(a),0)==S_OK&&context->GetData(endTime.Get(),&b,sizeof(b),0)==S_OK)ms=float(double(b-a)*1000/d.Frequency);else ms=-1;pending=false;return true;}
 float gpuMs()const{return ms;}
};
