#include "Localization.hpp"
#pragma once
#include "Graphics.hpp"
#include "imgui.h"
#include "UiTheme.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>
#include <cmath>
#include <thread>
#include <atomic>
#include <sstream>
#include <iomanip>

// Three dedicated snapshots. Never display the live CUDA output while it is
// being overwritten; copy only after its completion event is signalled.
struct Comparison {
 std::string backendName="NVIDIA VFG";
 Texture frames[3];unsigned width=0,height=0;bool valid=false,enabled=true,frozen=false,next=false;
 uint64_t pair=0,sourceA=0,sourceB=0;double gapMs=0,timestep=.5;int mode=0;
 float zoom=1,focusX=.5f,focusY=.5f;std::string exportStatus;
 struct Recorded {unsigned w,h;uint64_t pair,a,b;double gap,time,position;int quality;std::array<std::vector<unsigned char>,3> pixels;};
 std::vector<Recorded> recording;bool recordingActive=false;std::atomic<bool> writing=false;std::jthread writer;unsigned recordTarget=60;
 ComPtr<ID3D11Texture2D> recordStaging;unsigned stageW=0,stageH=0;
 void startRecording(){if(recordingActive||writing)return;recording.clear();recording.reserve(recordTarget);recordingActive=true;exportStatus="Recording 60 actual A / generated / B pairs (diagnostic readback)";}
 void record(Graphics& g,const std::filesystem::path& root,double sourceTime){
  if(!recordingActive||!valid)return;
  if(stageW!=width||stageH!=height||!recordStaging){recordStaging.Reset();D3D11_TEXTURE2D_DESC desc{};frames[0].texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;check(g.device->CreateTexture2D(&desc,nullptr,&recordStaging),"Record staging");stageW=width;stageH=height;}
  Recorded shot{width,height,pair,sourceA,sourceB,gapMs,sourceTime,timestep,mode};
  for(int i=0;i<3;++i){g.context->CopyResource(recordStaging.Get(),frames[i].texture.Get());D3D11_MAPPED_SUBRESOURCE map{};check(g.context->Map(recordStaging.Get(),0,D3D11_MAP_READ,0,&map),"Record readback");shot.pixels[i].resize(size_t(width)*height*4);for(unsigned y=0;y<height;++y)memcpy(shot.pixels[i].data()+size_t(y)*width*4,(unsigned char*)map.pData+size_t(y)*map.RowPitch,width*4);g.context->Unmap(recordStaging.Get(),0);}
  recording.push_back(std::move(shot));
  if(recording.size()<recordTarget)return;
  recordingActive=false;writing=true;auto folder=root/L"recordings"/("take-"+std::to_string(GetTickCount64()));std::filesystem::create_directories(folder);
  exportStatus="Saving diagnostic recording to "+folder.string();
  writer=std::jthread([this,folder,shots=std::move(recording)]() mutable {
   std::ofstream meta(folder/L"pairs.csv");meta<<"index,pair,source_A,source_B,source_gap_ms,source_B_time_s,width,height,quality,timestep\n";
   for(size_t n=0;n<shots.size();++n){auto& shot=shots[n];meta<<n<<','<<shot.pair<<','<<shot.a<<','<<shot.b<<','<<shot.gap<<','<<std::fixed<<std::setprecision(6)<<shot.time<<','<<shot.w<<','<<shot.h<<','<<shot.quality<<','<<shot.position<<'\n';
    for(int i=0;i<3;++i){std::ostringstream name;name<<std::setfill('0')<<std::setw(4)<<n<<"-"<<"AMB"[i]<<".bmp";BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(ih);fh.bfSize=fh.bfOffBits+DWORD(shot.pixels[i].size());ih.biSize=sizeof(ih);ih.biWidth=shot.w;ih.biHeight=-LONG(shot.h);ih.biPlanes=1;ih.biBitCount=32;ih.biCompression=BI_RGB;
     std::ofstream file(folder/name.str(),std::ios::binary);file.write((char*)&fh,sizeof(fh));file.write((char*)&ih,sizeof(ih));file.write((char*)shot.pixels[i].data(),shot.pixels[i].size());shot.pixels[i].clear();shot.pixels[i].shrink_to_fit();}
   }meta.close();std::ofstream(folder/L"complete.txt")<<"60 matched live VFG pairs. Diagnostic readback may introduce capture gaps. See source IDs/timestamps; do not infer live performance from this recording.\n";writing=false;
  });
 }
 void clear(){valid=false;next=false;exportStatus.clear();}
 void capture(Graphics& g,Texture& a,Texture& middle,Texture& b,unsigned w,unsigned h,uint64_t id,uint64_t aId,uint64_t bId,double gap,int quality,double position=.5){
  if((!enabled||(frozen&&!next))&&!recordingActive)return;
  if(width!=w||height!=h){width=w;height=h;for(auto& frame:frames)frame.create(g.device.Get(),w,h);}
  g.context->CopyResource(frames[0].texture.Get(),a.texture.Get());
  g.context->CopyResource(frames[1].texture.Get(),middle.texture.Get());
  g.context->CopyResource(frames[2].texture.Get(),b.texture.Get());
  pair=id;sourceA=aId;sourceB=bId;gapMs=gap;timestep=position;mode=quality;valid=true;next=false;
 }
 std::filesystem::path save(Graphics& g,const std::filesystem::path& root){
  if(!valid)throw std::runtime_error("No completed VFG pair to export");
  const auto folder=root/L"comparisons"/("pair-"+std::to_string(GetTickCount64())+"-"+std::to_string(pair));std::filesystem::create_directories(folder);
  std::array<std::vector<unsigned char>,3> pixels;
  const char* names[]={"01-raw-A.bmp","02-generated-only.bmp","03-raw-B.bmp"};
  D3D11_TEXTURE2D_DESC desc{};frames[0].texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
  ComPtr<ID3D11Texture2D> staging;check(g.device->CreateTexture2D(&desc,nullptr,&staging),"Comparison readback");
  for(int i=0;i<3;++i){g.context->CopyResource(staging.Get(),frames[i].texture.Get());D3D11_MAPPED_SUBRESOURCE map{};check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"Comparison map");
   pixels[i].resize(size_t(width)*height*4);for(unsigned y=0;y<height;++y)memcpy(pixels[i].data()+size_t(y)*width*4,(unsigned char*)map.pData+size_t(y)*map.RowPitch,width*4);g.context->Unmap(staging.Get(),0);
   BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(ih);fh.bfSize=fh.bfOffBits+DWORD(pixels[i].size());ih.biSize=sizeof(ih);ih.biWidth=width;ih.biHeight=-LONG(height);ih.biPlanes=1;ih.biBitCount=32;ih.biCompression=BI_RGB;ih.biSizeImage=DWORD(pixels[i].size());
   std::ofstream file(folder/names[i],std::ios::binary);file.write((char*)&fh,sizeof(fh));file.write((char*)&ih,sizeof(ih));file.write((char*)pixels[i].data(),pixels[i].size());if(!file)throw std::runtime_error("Failed writing comparison image");
  }
  double sums[3]{};uint64_t differences[2]{};
  for(size_t i=0;i<pixels[0].size();i+=4){bool da=false,db=false;for(int c=0;c<3;++c){double a=pixels[0][i+c],m=pixels[1][i+c],b=pixels[2][i+c];sums[0]+=std::abs(m-a);sums[1]+=std::abs(m-b);sums[2]+=std::abs(m-(a+b)*.5);da|=m!=a;db|=m!=b;}differences[0]+=da;differences[1]+=db;}
  const double n=double(width)*height;
  std::ofstream json(folder/L"comparison.json");json<<"{\"backend\":"<<std::quoted(backendName)<<",\"pair\":"<<pair<<",\"source_A\":"<<sourceA<<",\"source_B\":"<<sourceB<<",\"width\":"<<width<<",\"height\":"<<height<<",\"source_gap_ms\":"<<gapMs<<",\"timestep\":"<<timestep<<",\"VFG_mode\":"<<mode<<",\"mean_abs_RGB_vs_A\":"<<sums[0]/(n*3)<<",\"mean_abs_RGB_vs_B\":"<<sums[1]/(n*3)<<",\"mean_abs_RGB_vs_simple_blend\":"<<sums[2]/(n*3)<<",\"different_pixels_vs_A_percent\":"<<100*differences[0]/n<<",\"different_pixels_vs_B_percent\":"<<100*differences[1]/n<<",\"note\":\"Pixel differences are not a quality score. No ground-truth middle frame is available. RAW means captured, RGB-normalized HDMI input, not internal source rendering buffers. Export causes diagnostic CPU readback; live comparison remains GPU-only.\"}\n";
  exportStatus="Saved A / VFG / B + pixel comparison in app/comparisons";return folder;
 }
 void draw(Graphics& g,const std::filesystem::path& root,float availableHeight){
  if(ImGui::Checkbox(tr("A / VFG / B comparison"),&enabled))clear();
  if(enabled){ui::nextButton(frozen?tr("Resume pairs"):tr("Freeze pair"));if(ImGui::Button(frozen?tr("Resume pairs"):tr("Freeze pair")))frozen=!frozen;ui::nextButton(tr("Next pair"));if(ImGui::Button(tr("Next pair"))){frozen=true;next=true;}ui::nextButton(tr("Save full-res"));
   if(ImGui::Button(tr("Save full-res"))){try{save(g,root);}catch(const std::exception& e){exportStatus=e.what();}}}
  if(!enabled)return;
  if(ImGui::Button(tr("Record 60 pairs"))){startRecording();}
  if(recordingActive){ImGui::SameLine();ImGui::Text(tr("Recording %u / %u"),unsigned(recording.size()),recordTarget);}else if(writing){ImGui::SameLine();ImGui::TextUnformatted(tr("Writing recording..."));}else if(ImGui::GetContentRegionAvail().x>650){ImGui::SameLine();ImGui::TextDisabled(tr("Diagnostic recording: full-resolution A / VFG / B"));}
  ImGui::SetNextItemWidth(140);ImGui::SliderFloat(tr("Zoom"),&zoom,1,8,"%.1fx");ImGui::SameLine();if(ImGui::GetContentRegionAvail().x>400)ImGui::TextDisabled(tr("Drag an image to move the shared detail. %s"),frozen?"FROZEN":"LIVE");else ImGui::TextDisabled("%s",frozen?"FROZEN":"LIVE");
  if(!valid){ImGui::TextWrapped(tr("Waiting for a completed interpolation pair. Enable frame generation to compare A / generated / B."));return;}
  ImGui::PushFont(ui::mono);ImGui::TextDisabled(tr("Pair %llu | A #%llu -> VFG t=%.3f -> B #%llu | %.2f ms source interval"),(unsigned long long)pair,(unsigned long long)sourceA,timestep,(unsigned long long)sourceB,gapMs);ImGui::PopFont();
  const char* hint=tr("Same source pair. Middle panel contains ONLY the generated image. Drag to position zoom.");
  float footer=ImGui::CalcTextSize(hint,nullptr,false,ImGui::GetContentRegionAvail().x).y+ImGui::GetTextLineHeightWithSpacing()*2+18;
  float col=(ImGui::GetContentRegionAvail().x-20)/3;float ih=std::max(20.f,std::min(col*height/width,ImGui::GetContentRegionAvail().y-footer));float iw=ih*width/height;
  float half=.5f/zoom;focusX=std::clamp(focusX,half,1-half);focusY=std::clamp(focusY,half,1-half);
  ImVec2 uv0(focusX-half,focusY-half),uv1(focusX+half,focusY+half);
  const char* labels[]={"RAW INPUT / A (before)","NVIDIA VFG ONLY","RAW INPUT / B (after)"};
  if(ImGui::BeginTable("Synchronized frame comparison",3,ImGuiTableFlags_SizingStretchSame)){
   for(int i=0;i<3;++i){ImGui::TableNextColumn();ImGui::PushFont(ui::mono);ImGui::TextColored(i==1?ui::accent:ui::muted,"%s",i==1?backendName.c_str():tr(labels[i]));ImGui::PopFont();auto origin=ImGui::GetCursorScreenPos();ImGui::Image((ImTextureID)(intptr_t)frames[i].view.Get(),{iw,ih},uv0,uv1);
    if(ImGui::IsItemHovered()&&ImGui::IsMouseDown(ImGuiMouseButton_Left)){focusX=std::clamp((ImGui::GetMousePos().x-origin.x)/iw,half,1-half);focusY=std::clamp((ImGui::GetMousePos().y-origin.y)/ih,half,1-half);}
   }ImGui::EndTable();}
  ImGui::PushStyleColor(ImGuiCol_Text,ui::muted);ImGui::TextWrapped("%s",hint);ImGui::PopStyleColor();
  if(!exportStatus.empty())ImGui::TextWrapped("%s",exportStatus.c_str());
 }
};
