#include "Graphics.hpp"
#include "SubtitleControls.hpp"
#include "imgui_impl_dx11.h"
#include <iostream>
#include <vector>
#include <stdexcept>
// Actual ImGui input and D3D pixels in our own target. No desktop capture/input.
int main(){try{
 Graphics g;g.initDevice();constexpr unsigned w=1280,h=720;
 Texture target;target.create(g.device.Get(),w,h);ComPtr<ID3D11RenderTargetView> rtv;
 check(g.device->CreateRenderTargetView(target.texture.Get(),nullptr,&rtv),"Subtitle RTV");
 D3D11_TEXTURE2D_DESC desc{};target.texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ComPtr<ID3D11Texture2D> staging;check(g.device->CreateTexture2D(&desc,nullptr,&staging),"Subtitle staging");
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={float(w),float(h)};io.DeltaTime=1.f/60;ui::initialize();
 ImGui_ImplDX11_Init(g.device.Get(),g.context.Get());
 SubtitleSettings s;s.enabled=true;s.showRegion=true;s.editOverlay=true;
 SubtitleSnapshot caption;caption.source="The door is locked.";caption.translated="Dveře jsou zamčené.";caption.observedAt=10;
 auto frame=[&](bool clean){
  ImGui_ImplDX11_NewFrame();ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({float(w),float(h)});
  ImGui::Begin("Subtitle test",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);
  drawSubtitleOverlay({0,0},{float(w),float(h)},s,caption,10,clean);ImGui::End();ImGui::Render();
  const float clear[]={0,0,0,1};g.context->ClearRenderTargetView(rtv.Get(),clear);g.context->OMSetRenderTargets(1,rtv.GetAddressOf(),nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  g.context->CopyResource(staging.Get(),target.texture.Get());D3D11_MAPPED_SUBRESOURCE pixels{};check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&pixels),"Subtitle readback");
  unsigned green=0,white=0;for(unsigned y=0;y<h;++y){auto row=(const unsigned char*)pixels.pData+size_t(y)*pixels.RowPitch;for(unsigned x=0;x<w;++x){auto p=row+x*4;if(p[1]>190&&p[2]>100&&p[2]<200&&p[0]<120)++green;if(p[0]>220&&p[1]>220&&p[2]>220)++white;}}
  g.context->Unmap(staging.Get(),0);return std::pair{green,white};
 };
 frame(false);io.AddMousePosEvent(640,633);frame(false);io.AddMouseButtonEvent(0,true);frame(false);io.AddMousePosEvent(740,563);frame(false);io.AddMouseButtonEvent(0,false);auto edited=frame(false);
 if(std::abs(s.horizontal-(.5f+100.f/w))>.005f||std::abs(s.position-(.88f-70.f/h))>.005f)throw std::runtime_error("Overlay drag did not move normalized position");
 if(s.left!=.1f||s.top!=.72f||s.right!=.9f||s.bottom!=.97f)throw std::runtime_error("Dragging changed OCR region");
 if(edited.first<100||edited.second<100)throw std::runtime_error("Editing handles or caption pixels missing");
 auto clean=frame(true);if(clean.first!=0||clean.second<100)throw std::runtime_error("Fullscreen leaks green OCR/edit controls or hides caption");
 const auto before=s;io.AddMouseButtonEvent(0,true);frame(true);io.AddMousePosEvent(800,500);frame(true);io.AddMouseButtonEvent(0,false);frame(true);
 if(!(s==before))throw std::runtime_error("Clean fullscreen allowed editing");
 s.enabled=false;clean=frame(true);if(clean.first||clean.second)throw std::runtime_error("Disabled subtitle or edit preview leaked into fullscreen");
 ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();std::cout<<"PASS: actual drag, independent OCR crop, clean fullscreen pixels, no fullscreen editing\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
