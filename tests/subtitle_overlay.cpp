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
 subtitleRaster.device=g.device;
 SubtitleSettings s;s.enabled=true;s.showRegion=true;s.editOverlay=true;
 SubtitleSnapshot caption;caption.source="The door is locked.";caption.translated="Dveře jsou zamčené.";caption.observedAt=10;
 unsigned outsideCaptionPixels=0,minWhiteY=h,maxWhiteY=0,whiteBands=0;
 std::vector<std::pair<unsigned,unsigned>> lineBounds;
 auto frame=[&](bool clean,bool cropped=false){
  ImGui_ImplDX11_NewFrame();ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({float(w),float(h)});
  ImGui::Begin("Subtitle test",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);
  if(cropped){auto draw=ImGui::GetWindowDrawList();draw->PushClipRect({200,160},{1000,560},true);drawSubtitleOverlay({-640,-360},{2560,1440},s,caption,10,clean);draw->PopClipRect();}
  else drawSubtitleOverlay({0,0},{float(w),float(h)},s,caption,10,clean);ImGui::End();ImGui::Render();
  const float clear[]={0,0,0,1};g.context->ClearRenderTargetView(rtv.Get(),clear);g.context->OMSetRenderTargets(1,rtv.GetAddressOf(),nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  g.context->CopyResource(staging.Get(),target.texture.Get());D3D11_MAPPED_SUBRESOURCE pixels{};check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&pixels),"Subtitle readback");
  unsigned green=0,white=0;outsideCaptionPixels=0;minWhiteY=h;maxWhiteY=0;whiteBands=0;bool previousWhite=false;lineBounds.clear();
  for(unsigned y=0;y<h;++y){bool rowWhite=false;unsigned left=w,right=0;auto row=(const unsigned char*)pixels.pData+size_t(y)*pixels.RowPitch;for(unsigned x=0;x<w;++x){auto p=row+x*4;if(p[1]>190&&p[2]>100&&p[2]<200&&p[0]<120)++green;if(p[0]>220&&p[1]>220&&p[2]>220){rowWhite=true;left=std::min(left,x);right=std::max(right,x);++white;minWhiteY=std::min(minWhiteY,y);maxWhiteY=std::max(maxWhiteY,y);if(x<200||x>=1000||y<160||y>=560)++outsideCaptionPixels;}}if(rowWhite){if(!previousWhite){++whiteBands;lineBounds.push_back({left,right});}else{lineBounds.back().first=std::min(lineBounds.back().first,left);lineBounds.back().second=std::max(lineBounds.back().second,right);}}previousWhite=rowWhite;}
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
 s.enabled=true;s.horizontal=.5f;s.position=.6f;s.fontPercent=5.5f;s.placement=0;
 auto checkCentered=[&]{frame(true);if(lineBounds.size()<2)throw std::runtime_error("Expected multiple visible subtitle lines");auto center=(lineBounds.front().first+lineBounds.front().second)*.5f;for(auto [left,right]:lineBounds)if(std::abs((left+right)*.5f-center)>2.f)throw std::runtime_error("Subtitle lines do not share the same visual center");};
 caption.translated="HHHHH HHHHH HHHHH\nHHH";checkCentered();
 caption.translated="";for(int word=0;word<12;++word)caption.translated+="HHHHH ";checkCentered();
 s.enabled=true;s.horizontal=1;s.position=1;s.fontPercent=8;s.placement=0;
 // Solid capital stems give one connected white-pixel band per line; dots and
 // antialiased curves can produce multiple bands even when a line is intact.
 caption.translated="";for(int line=0;line<18;++line)caption.translated+="HHHHH HHHHH HHHHH\n";
 clean=frame(true,true);
 std::cout<<"Cropped: white="<<clean.second<<" outside="<<outsideCaptionPixels<<" y="<<minWhiteY<<".."<<maxWhiteY<<" bands="<<whiteBands<<'\n';
 if(clean.first||clean.second<500||outsideCaptionPixels||minWhiteY<=160||maxWhiteY>=559||whiteBands!=18)throw std::runtime_error("Multiline caption failed to fit all 18 lines in visible cropped viewport");
 ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();std::cout<<"PASS: actual drag, independent OCR crop, clean fullscreen pixels, no fullscreen editing\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
