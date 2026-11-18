#include "Enhancer.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
using Clock=std::chrono::steady_clock;
std::vector<unsigned char> pixels(Graphics& g,Texture& tex,unsigned w,unsigned h,bool rgba){
 D3D11_TEXTURE2D_DESC d{};tex.texture->GetDesc(&d);d.Usage=D3D11_USAGE_STAGING;d.BindFlags=d.MiscFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ComPtr<ID3D11Texture2D> read;check(g.device->CreateTexture2D(&d,nullptr,&read),"Test readback");g.context->CopyResource(read.Get(),tex.texture.Get());D3D11_MAPPED_SUBRESOURCE m{};check(g.context->Map(read.Get(),0,D3D11_MAP_READ,0,&m),"Test map");std::vector<unsigned char> result(size_t(w)*h*4);
 for(unsigned y=0;y<h;++y)memcpy(result.data()+size_t(y)*w*4,(char*)m.pData+size_t(y)*m.RowPitch,w*4);g.context->Unmap(read.Get(),0);
 if(rgba)for(size_t i=0;i<result.size();i+=4)std::swap(result[i],result[i+2]);return result;
}
void bmp(const std::filesystem::path& path,const std::vector<unsigned char>& data,unsigned w,unsigned h){
 BITMAPFILEHEADER f{};f.bfType=0x4d42;f.bfOffBits=sizeof(f)+sizeof(BITMAPINFOHEADER);f.bfSize=f.bfOffBits+DWORD(data.size());BITMAPINFOHEADER i{};i.biSize=sizeof(i);i.biWidth=w;i.biHeight=-int(h);i.biPlanes=1;i.biBitCount=32;i.biCompression=BI_RGB;
 std::ofstream out(path,std::ios::binary);out.write((char*)&f,sizeof(f));out.write((char*)&i,sizeof(i));out.write((char*)data.data(),data.size());
}
int wmain(int argc,wchar_t** argv){
 try{
  if(argc<3)throw std::runtime_error("Usage: nr_gpu_test app-dir result-dir [input.bgra width height] [runtime-name] [inference-height]");
  auto app=std::filesystem::absolute(argv[1]),dir=std::filesystem::absolute(argv[2]);std::filesystem::create_directories(dir);
  unsigned w=argc>4?_wtoi(argv[4]):640,h=argc>5?_wtoi(argv[5]):360,ih=argc>7?_wtoi(argv[7]):360;
  std::vector<unsigned char> image(size_t(w)*h*4);
  if(argc>3){std::ifstream in(argv[3],std::ios::binary);if(!in.read((char*)image.data(),image.size()))throw std::runtime_error("Input BGRA missing or truncated");}
  else for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){auto p=image.data()+(size_t(y)*w+x)*4;p[0]=32+x*160/w;p[1]=32+y*160/h;p[2]=((x/20+y/20)%2)?170:90;p[3]=255;}
  Graphics g;g.initDevice();Texture input;input.create(g.device.Get(),w,h);g.context->UpdateSubresource(input.texture.Get(),0,nullptr,image.data(),w*4,0);
  Enhancer nr(g,w,h,ih,app,app/L"runtime"/(argc>6?argv[6]:L"nr-sf"));
  auto deadline=Clock::now()+std::chrono::seconds(150);
  while(nr.state()==Enhancer::State::Loading){if(Clock::now()>deadline)throw std::runtime_error("Loading timeout");Sleep(10);}
  if(nr.state()==Enhancer::State::Failed)throw std::runtime_error(nr.error());
  const auto initial=image;
  for(int frame=0;frame<12;++frame){
   image=initial;
   // Vary a marker so a stale input/output pairing fails byte verification.
   if(frame<11)for(unsigned y=0;y<std::min(h,32u);++y)for(unsigned x=0;x<std::min(w,32u);++x){auto p=image.data()+(size_t(y)*w+x)*4;p[0]=frame*21;p[1]=255-frame*19;}
   g.context->UpdateSubresource(input.texture.Get(),0,nullptr,image.data(),w*4,0);
   if(!nr.submit(input.texture.Get(),frame==0||frame==6,frame!=7))throw std::runtime_error("Submit refused");
   if(nr.submit(input.texture.Get()))throw std::runtime_error("Accepted a second in-flight frame");
   deadline=Clock::now()+std::chrono::seconds(30);
   while(!nr.take()){if(nr.state()==Enhancer::State::Failed)throw std::runtime_error(nr.error());if(Clock::now()>deadline)throw std::runtime_error("Inference timeout");Sleep(1);}
   std::cout<<"frame "<<frame<<" work_ms="<<nr.processingMs()<<std::endl;
   if(pixels(g,nr.original,w,h,false)!=image)throw std::runtime_error("Stale A/B reference frame");
   if(nr.take())throw std::runtime_error("Published a frame twice");
  }
  auto before=pixels(g,nr.original,w,h,false),after=pixels(g,nr.result,w,h,true);
  size_t changed=0,referenceErrors=0,opaque=0;double difference=0,brightness=0;
  for(size_t i=0;i<image.size();i+=4){bool different=false;for(int c=0;c<3;++c){referenceErrors+=before[i+c]!=image[i+c];auto delta=std::abs(int(after[i+c])-int(before[i+c]));difference+=delta;different|=delta>1;brightness+=after[i+c];}changed+=different;opaque+=after[i+3]==255;}
  double mae=difference/(w*h*3.0);bool pass=referenceErrors==0&&changed>w*h/1000&&brightness>w*h*3.0&&opaque==size_t(w)*h;
  bmp(dir/L"before.bmp",before,w,h);bmp(dir/L"after.bmp",after,w,h);
  std::ofstream(dir/L"result.json")<<"{\"passed\":"<<(pass?"true":"false")<<",\"width\":"<<w<<",\"height\":"<<h<<",\"changed_pixels\":"<<changed<<",\"reference_errors\":"<<referenceErrors<<",\"opaque_pixels\":"<<opaque<<",\"rgb_mae\":"<<mae<<",\"last_work_ms\":"<<nr.processingMs()<<",\"note\":\"Readback only in this test. Pixel difference is not a quality metric.\"}";
  std::cout<<"reference_errors="<<referenceErrors<<" changed="<<changed<<" mae="<<mae<<" passed="<<pass<<std::endl;return pass?0:2;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}
