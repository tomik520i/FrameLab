#pragma once
#include "Graphics.hpp"
#include <d3dcompiler.h>
#include <memory>
// Experimental GPU block-matching optical flow. No CUDA/NVIDIA SDK dependency.
class OpticalFlow {
 ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
 Texture inputs[2];ComPtr<ID3D11Texture2D> flow;
 ComPtr<ID3D11ShaderResourceView> flowView;ComPtr<ID3D11UnorderedAccessView> flowWrite;
 ComPtr<ID3D11RenderTargetView> target;ComPtr<ID3D11ComputeShader> cs;
 ComPtr<ID3D11VertexShader> vs;ComPtr<ID3D11PixelShader> ps;
 ComPtr<ID3D11Buffer> constants;ComPtr<ID3D11SamplerState> sampler;
 ComPtr<ID3D11Query> done,beginTime,endTime,disjoint;unsigned w=0,h=0;bool pending=false;float ms=-1;
 static constexpr const char* shader=R"HLSL(
cbuffer Params:register(b0){float2 size;float timestep;float cut;};
Texture2D<float4> A:register(t0);Texture2D<float4> B:register(t1);Texture2D<float4> Flow:register(t2);
RWTexture2D<float4> OutFlow:register(u0);SamplerState linearClamp:register(s0);
float lum(float3 c){return dot(c,float3(.299,.587,.114));}
float errorAt(int2 pos,int2 delta,bool reverse){
 float e=0;
 [unroll]for(int y=-1;y<=1;++y)[unroll]for(int x=-1;x<=1;++x){
  int2 p=clamp(pos+int2(x,y)*8,int2(0,0),int2(size)-1),q=clamp(p+delta,int2(0,0),int2(size)-1);
  float a=reverse?lum(B.Load(int3(p,0)).rgb):lum(A.Load(int3(p,0)).rgb);
  float b=reverse?lum(A.Load(int3(q,0)).rgb):lum(B.Load(int3(q,0)).rgb);
  e+=abs(a-b);
 }return e+dot(float2(delta),float2(delta))*.000002;
}
float2 match(int2 p,bool reverse){
 float best=1e20;int2 displacement=0;
 [loop]for(int y=-8;y<=8;++y)[loop]for(int x=-8;x<=8;++x){int2 d=int2(x,y)*8;float e=errorAt(p,d,reverse);if(e<best){best=e;displacement=d;}}
 int2 coarse=displacement;
 [loop]for(int y=-3;y<=3;++y)[loop]for(int x=-3;x<=3;++x){int2 d=coarse+int2(x,y)*2;float e=errorAt(p,d,reverse);if(e<best){best=e;displacement=d;}}
 return displacement;
}
[numthreads(8,8,1)]void flowMain(uint3 id:SV_DispatchThreadID){if(any(id.xy>=uint2(ceil(size/16))))return;int2 p=min(int2(id.xy)*16+8,int2(size)-1);OutFlow[id.xy]=float4(match(p,false),match(p,true));}
struct V{float4 pos:SV_POSITION;};
V vertexMain(uint id:SV_VertexID){V v;float2 uv=float2((id<<1)&2,id&2);v.pos=float4(uv*float2(2,-2)+float2(-1,1),0,1);return v;}
float4 pixelMain(V v):SV_TARGET{
 float2 uv=v.pos.xy/size;if(cut>0)return B.SampleLevel(linearClamp,uv,0);
 float4 motion=Flow.SampleLevel(linearClamp,v.pos.xy/(ceil(size/16)*16),0);
 float3 a=A.SampleLevel(linearClamp,uv-timestep*motion.xy/size,0).rgb;
 float3 b=B.SampleLevel(linearClamp,uv-(1-timestep)*motion.zw/size,0).rgb;
 return float4(lerp(a,b,timestep),1);
}
)HLSL";
 ComPtr<ID3DBlob> compile(const char* entry,const char* model){ComPtr<ID3DBlob> code,error;auto hr=D3DCompile(shader,strlen(shader),"FrameLab optical flow",nullptr,nullptr,entry,model,D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error);if(FAILED(hr))throw std::runtime_error(error?std::string((char*)error->GetBufferPointer()):"Shader compilation failed");return code;}
public:
 void initialize(ID3D11Device* d,ID3D11Texture2D* output,unsigned width,unsigned height){
  device=d;device->GetImmediateContext(&context);w=width;h=height;
  for(auto& t:inputs)t.create(d,w,h);
  D3D11_TEXTURE2D_DESC desc{};desc.Width=(w+15)/16;desc.Height=(h+15)/16;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;
  check(d->CreateTexture2D(&desc,nullptr,&flow),"Flow texture");check(d->CreateShaderResourceView(flow.Get(),nullptr,&flowView),"Flow SRV");check(d->CreateUnorderedAccessView(flow.Get(),nullptr,&flowWrite),"Flow UAV");check(d->CreateRenderTargetView(output,nullptr,&target),"Flow output RTV");
  auto c=compile("flowMain","cs_5_0");check(d->CreateComputeShader(c->GetBufferPointer(),c->GetBufferSize(),nullptr,&cs),"Flow CS");
  c=compile("vertexMain","vs_5_0");check(d->CreateVertexShader(c->GetBufferPointer(),c->GetBufferSize(),nullptr,&vs),"Flow VS");c=compile("pixelMain","ps_5_0");check(d->CreatePixelShader(c->GetBufferPointer(),c->GetBufferSize(),nullptr,&ps),"Flow PS");
  D3D11_BUFFER_DESC bd{};bd.ByteWidth=16;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;check(d->CreateBuffer(&bd,nullptr,&constants),"Flow constants");
  D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;check(d->CreateSamplerState(&sd,&sampler),"Flow sampler");
  for(auto item:{std::pair{D3D11_QUERY_EVENT,std::addressof(done)},std::pair{D3D11_QUERY_TIMESTAMP,std::addressof(beginTime)},std::pair{D3D11_QUERY_TIMESTAMP,std::addressof(endTime)},std::pair{D3D11_QUERY_TIMESTAMP_DISJOINT,std::addressof(disjoint)}}){D3D11_QUERY_DESC q{item.first,0};check(d->CreateQuery(&q,item.second->GetAddressOf()),"Flow query");}
 }
 void upload(const unsigned char* data,unsigned parity){context->UpdateSubresource(inputs[parity].texture.Get(),0,nullptr,data,w*4,0);}
 void upload(ID3D11Texture2D* texture,unsigned parity){
  if(parity>1||pending)throw std::runtime_error("Invalid or busy optical input");
  D3D11_TEXTURE2D_DESC d{};texture->GetDesc(&d);
  if(d.Width!=w||d.Height!=h||d.Format!=DXGI_FORMAT_B8G8R8A8_UNORM)throw std::runtime_error("Optical input format mismatch");
  context->CopyResource(inputs[parity].texture.Get(),texture);
 }
 void generate(unsigned current,bool cut,unsigned index,unsigned multiplier){
  float params[]={float(w),float(h),float(index)/multiplier,cut?1.f:0.f};context->UpdateSubresource(constants.Get(),0,nullptr,params,0,0);
  ID3D11ShaderResourceView* views[]={inputs[1-current].view.Get(),inputs[current].view.Get(),flowView.Get()};ID3D11ShaderResourceView* nullViews[3]{};
  context->Begin(disjoint.Get());context->End(beginTime.Get());
  if(index==1){context->CSSetShader(cs.Get(),nullptr,0);context->CSSetConstantBuffers(0,1,constants.GetAddressOf());context->CSSetShaderResources(0,2,views);context->CSSetUnorderedAccessViews(0,1,flowWrite.GetAddressOf(),nullptr);context->Dispatch(((w+15)/16+7)/8,((h+15)/16+7)/8,1);ID3D11UnorderedAccessView* n=nullptr;context->CSSetUnorderedAccessViews(0,1,&n,nullptr);context->CSSetShaderResources(0,2,nullViews);context->CSSetShader(nullptr,nullptr,0);}
  context->OMSetRenderTargets(1,target.GetAddressOf(),nullptr);context->OMSetBlendState(nullptr,nullptr,~0u);context->OMSetDepthStencilState(nullptr,0);context->RSSetState(nullptr);
  D3D11_VIEWPORT vp{0,0,float(w),float(h),0,1};context->RSSetViewports(1,&vp);context->IASetInputLayout(nullptr);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(vs.Get(),nullptr,0);context->PSSetShader(ps.Get(),nullptr,0);context->PSSetConstantBuffers(0,1,constants.GetAddressOf());context->PSSetSamplers(0,1,sampler.GetAddressOf());context->PSSetShaderResources(0,3,views);context->Draw(3,0);context->PSSetShaderResources(0,3,nullViews);context->OMSetRenderTargets(0,nullptr,nullptr);
  context->End(endTime.Get());context->End(disjoint.Get());context->End(done.Get());context->Flush();pending=true;
 }
 bool ready(){if(!pending)return false;BOOL complete=FALSE;HRESULT hr=context->GetData(done.Get(),&complete,sizeof(complete),0);check(hr,"Flow completion");if(hr!=S_OK||!complete)return false;
  UINT64 a=0,b=0;D3D11_QUERY_DATA_TIMESTAMP_DISJOINT d{};
  if(context->GetData(disjoint.Get(),&d,sizeof(d),0)==S_OK&&!d.Disjoint&&context->GetData(beginTime.Get(),&a,sizeof(a),0)==S_OK&&context->GetData(endTime.Get(),&b,sizeof(b),0)==S_OK)ms=float(double(b-a)*1000/d.Frequency);else ms=-1;
  pending=false;return true;
 }
 float gpuMs() const{return ms;}
};
