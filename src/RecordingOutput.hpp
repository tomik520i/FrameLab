#pragma once
#include "Graphics.hpp"
#include "SubtitleControls.hpp"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"

// A source-sized clean video surface, never a capture of the desktop or dashboard.
// Uses the same alpha blend and subtitle drawing code as fullscreen output.
struct RecordingOutput {
 Texture image;ComPtr<ID3D11RenderTargetView> target;unsigned width=0,height=0;
 void render(Graphics& g,unsigned w,unsigned h,ID3D11ShaderResourceView* before,
             ID3D11ShaderResourceView* enhanced,float strength,SubtitleSettings* settings,
             const SubtitleSnapshot& caption,double time){
  if(width!=w||height!=h){image.create(g.device.Get(),w,h);target.Reset();check(g.device->CreateRenderTargetView(image.texture.Get(),nullptr,&target),"Recording output target");width=w;height=h;}
  ImDrawList list(ImGui::GetDrawListSharedData());list._ResetForNewFrame();
  list.PushClipRect({0,0},{float(w),float(h)});list.PushTextureID(ImGui::GetIO().Fonts->TexID);
  list.AddImage((ImTextureID)(intptr_t)before,{0,0},{float(w),float(h)});
  if(enhanced&&strength>0)list.AddImage((ImTextureID)(intptr_t)enhanced,{0,0},{float(w),float(h)},{0,0},{1,1},IM_COL32(255,255,255,int(strength*255+.5f)));
  if(settings)drawSubtitleOverlay({0,0},{float(w),float(h)},*settings,caption,time,true,&list);
  list.PopTextureID();list.PopClipRect();
  ImDrawData data;data.Valid=true;data.DisplayPos={0,0};data.DisplaySize={float(w),float(h)};data.FramebufferScale={1,1};data.AddDrawList(&list);
  ComPtr<ID3D11RenderTargetView> old;ComPtr<ID3D11DepthStencilView> depth;g.context->OMGetRenderTargets(1,&old,&depth);
  const float black[]={0,0,0,1};g.context->ClearRenderTargetView(target.Get(),black);g.context->OMSetRenderTargets(1,target.GetAddressOf(),nullptr);
  ImGui_ImplDX11_RenderDrawData(&data);
  g.context->OMSetRenderTargets(1,old.GetAddressOf(),depth.Get());
 }
};
