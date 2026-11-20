#pragma once
#include "SubtitleTranslator.hpp"
#include "Graphics.hpp"
#include "imgui.h"
#include <Windows.h>
#include <fstream>
#include <chrono>
#include <algorithm>

// A small, explicitly selected source word. Never capture a desktop/window.
// Only a numeric ID is persisted; no caller-supplied file path is followed.
inline bool saveSubtitleStyleEvidence(const std::filesystem::path& root,const std::vector<unsigned char>& pixels,unsigned w,unsigned h,const std::array<float,4>& rect,SubtitleStyleProfile& profile){
 if(root.empty()||!w||!h||pixels.size()!=size_t(w)*h*4)return false;
 const auto x0=std::min(w-1,unsigned(std::clamp(rect[0],0.f,1.f)*w)),y0=std::min(h-1,unsigned(std::clamp(rect[1],0.f,1.f)*h));
 const auto x1=std::clamp(unsigned(std::clamp(rect[2],0.f,1.f)*w),x0+1,w),y1=std::clamp(unsigned(std::clamp(rect[3],0.f,1.f)*h),y0+1,h);
 const float scale=std::min({1.f,512.f/(x1-x0),128.f/(y1-y0)});const unsigned ow=std::max(1u,unsigned((x1-x0)*scale)),oh=std::max(1u,unsigned((y1-y0)*scale));
 std::vector<unsigned char> crop(size_t(ow)*oh*4);
 for(unsigned y=0;y<oh;++y)for(unsigned x=0;x<ow;++x){auto dst=crop.data()+(size_t(y)*ow+x)*4;auto src=pixels.data()+(size_t(y0+y*(y1-y0)/oh)*w+x0+x*(x1-x0)/ow)*4;memcpy(dst,src,4);dst[3]=255;}
 const auto id=uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
 std::error_code ec;const auto folder=root/L"subtitle-style-samples";std::filesystem::create_directories(folder,ec);if(ec)return false;
 const auto path=folder/(std::to_wstring(id)+L".bmp");if(std::filesystem::exists(path))return false;
 BITMAPFILEHEADER fh{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(BITMAPINFOHEADER);fh.bfSize=fh.bfOffBits+DWORD(crop.size());
 BITMAPINFOHEADER ih{};ih.biSize=sizeof(ih);ih.biWidth=ow;ih.biHeight=-int(oh);ih.biPlanes=1;ih.biBitCount=32;ih.biCompression=BI_RGB;
 std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<char*>(&fh),sizeof(fh));out.write(reinterpret_cast<char*>(&ih),sizeof(ih));out.write(reinterpret_cast<char*>(crop.data()),crop.size());out.close();if(!out)return false;
 profile.sampleId=id;profile.sampleFrameHeight=h;return true;
}

class SubtitleStyleEvidence {
 Texture texture;uint64_t loaded=0;unsigned w=0,h=0;
public:
 bool load(Graphics& g,const std::filesystem::path& root,uint64_t id){
  if(id==loaded)return bool(texture.view);loaded=id;texture={};w=h=0;if(!id||root.empty())return false;
  std::ifstream in(root/L"subtitle-style-samples"/(std::to_wstring(id)+L".bmp"),std::ios::binary);BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};
  if(!in.read(reinterpret_cast<char*>(&fh),sizeof(fh))||!in.read(reinterpret_cast<char*>(&ih),sizeof(ih))||fh.bfType!=0x4d42||fh.bfOffBits!=sizeof(fh)+sizeof(ih)||ih.biSize!=sizeof(ih)||ih.biPlanes!=1||ih.biCompression!=BI_RGB||ih.biBitCount!=32||ih.biWidth<1||ih.biWidth>512||ih.biHeight>=0||ih.biHeight< -128)return false;
  w=unsigned(ih.biWidth);h=unsigned(-ih.biHeight);std::vector<unsigned char> pixels(size_t(w)*h*4);if(!in.read(reinterpret_cast<char*>(pixels.data()),pixels.size()))return false;
  texture.create(g.device.Get(),w,h);g.context->UpdateSubresource(texture.texture.Get(),0,nullptr,pixels.data(),w*4,0);return true;
 }
 void draw(float available){if(texture.view){const float scale=std::min({2.f,std::max(1.f,available)/w,110.f/h});ImGui::Image((ImTextureID)texture.view.Get(),{w*scale,h*scale});}}
};
