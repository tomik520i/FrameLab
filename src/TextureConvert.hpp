#pragma once
#include "Graphics.hpp"
#include <d3dcompiler.h>

// GPU-only format conversion. CopyResource cannot convert RGBA NR output to
// BGRA VFG input: it would either fail validation or exchange red and blue.
class TextureConvert {
 ComPtr<ID3D11VertexShader> vs;ComPtr<ID3D11PixelShader> ps;
public:
 void copy(Graphics& g,ID3D11ShaderResourceView* source,Texture& destination,unsigned w,unsigned h){
  if(!vs){
   const char* code="Texture2D<float4> src:register(t0);struct V{float4 p:SV_POSITION;};V vertex(uint id:SV_VertexID){V o;float2 u=float2((id<<1)&2,id&2);o.p=float4(u*float2(2,-2)+float2(-1,1),0,1);return o;}float4 pixel(V i):SV_TARGET{return float4(src.Load(int3(int2(i.p.xy),0)).rgb,1);}";
   ComPtr<ID3DBlob> v,p,error;check(D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"vertex","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&v,&error),"Conversion VS");
   check(D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"pixel","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&p,&error),"Conversion PS");
   check(g.device->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&vs),"Conversion vertex shader");
   check(g.device->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&ps),"Conversion pixel shader");
  }
  ComPtr<ID3D11RenderTargetView> target;check(g.device->CreateRenderTargetView(destination.texture.Get(),nullptr,&target),"Conversion target");
  g.context->OMSetRenderTargets(1,target.GetAddressOf(),nullptr);g.context->OMSetBlendState(nullptr,nullptr,~0u);g.context->OMSetDepthStencilState(nullptr,0);g.context->RSSetState(nullptr);
  D3D11_VIEWPORT viewport{0,0,float(w),float(h),0,1};g.context->RSSetViewports(1,&viewport);
  g.context->IASetInputLayout(nullptr);g.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  g.context->VSSetShader(vs.Get(),nullptr,0);g.context->PSSetShader(ps.Get(),nullptr,0);g.context->PSSetShaderResources(0,1,&source);g.context->Draw(3,0);
  ID3D11ShaderResourceView* empty=nullptr;g.context->PSSetShaderResources(0,1,&empty);g.context->OMSetRenderTargets(0,nullptr,nullptr);
 }
};
