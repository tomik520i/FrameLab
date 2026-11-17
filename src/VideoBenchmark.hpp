#pragma once
#include "Vfg.hpp"
#include <fstream>
#include <filesystem>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>

// Offline held-out-frame experiment. Timings separate CUDA work from readback,
// disk IO and metrics; none are physical end-to-end display latency.
inline void runVideoBenchmark(Graphics& g,const std::filesystem::path& root,bool automaticCuts=true){
 const unsigned w=1920,h=1080;const size_t bytes=size_t(w)*h*4;
 auto source=root/L"samples"/L"csgo-1080p60.bgra";
 std::ifstream input(source,std::ios::binary);if(!input)throw std::runtime_error("Missing prepared CS:GO sample");
 const auto count=std::filesystem::file_size(source)/bytes;if(count<17)throw std::runtime_error("Sample too short");
 auto folder=root/L"benchmarks"/(automaticCuts?L"csgo":L"csgo-no-cut");std::filesystem::create_directories(folder);
 std::ofstream summary(folder/L"summary.csv");summary<<"quality,multiplier,input_fps,output_fps,generated_count,exact_endpoint_copies,mean_psnr_db,mean_hold_psnr_db,mean_blend_psnr_db,gpu_mean_ms,gpu_p95_ms,gpu_max_ms,wall_seconds\n";
 Texture output;output.create(g.device.Get(),w,h);D3D11_TEXTURE2D_DESC desc{};output.texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ComPtr<ID3D11Texture2D> staging;check(g.device->CreateTexture2D(&desc,nullptr,&staging),"Benchmark staging");
 std::vector<unsigned char> a(bytes),b(bytes),truth(bytes),actual(bytes),worst(bytes),worstTruth(bytes);
 auto read=[&](size_t n,std::vector<unsigned char>& pixels){input.clear();input.seekg(n*bytes);if(!input.read((char*)pixels.data(),bytes))throw std::runtime_error("Truncated sample");};
 auto psnr=[](double sse,double n){return sse==0?100.:10*std::log10(255.*255.*n/sse);};
 auto bmp=[&](const std::filesystem::path& path,const std::vector<unsigned char>& pixels){BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(ih);fh.bfSize=DWORD(fh.bfOffBits+bytes);ih.biSize=sizeof(ih);ih.biWidth=w;ih.biHeight=-LONG(h);ih.biPlanes=1;ih.biBitCount=32;std::ofstream f(path,std::ios::binary);f.write((char*)&fh,sizeof(fh));f.write((char*)&ih,sizeof(ih));f.write((char*)pixels.data(),bytes);if(!f)throw std::runtime_error("Benchmark image write failed");};
 for(unsigned quality=automaticCuts?0:1;quality<(automaticCuts?3u:2u);++quality)for(unsigned multiplier=2;multiplier<=4;++multiplier){
  Vfg vfg;vfg.initialize(g.device.Get(),output.texture.Get(),w,h,quality,root/L"runtime",automaticCuts);
  auto name=std::string(quality==0?"low":quality==1?"medium":"high")+"-"+std::to_string(multiplier)+"x";
  std::ofstream rows(folder/(name+".csv"));rows<<"frame,index,multiplier,psnr_db,hold_psnr_db,blend_psnr_db,exact_A,exact_B,gpu_ms\n";
  std::ofstream video;if(quality==1)video.open(folder/(name+".bgra"),std::ios::binary);
  unsigned copies=0,outputs=0;double score=0,hold=0,blend=0,worstScore=101;std::vector<float> times;
  auto begin=std::chrono::steady_clock::now();
  for(size_t n=0;n+multiplier<count;n+=multiplier){
   read(n,a);read(n+multiplier,b);vfg.upload(a.data(),0);vfg.upload(b.data(),1);
   if(video.is_open())video.write((char*)a.data(),bytes);
   for(unsigned index=1;index<multiplier;++index){
    vfg.generate(1,false,index,multiplier);auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
    while(!vfg.ready()){if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("Benchmark GPU timeout");Sleep(1);}
    float gpu=vfg.gpuMs();if(n>=multiplier*3)times.push_back(gpu);
    g.context->CopyResource(staging.Get(),output.texture.Get());D3D11_MAPPED_SUBRESOURCE map{};check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"Benchmark readback");
    for(unsigned y=0;y<h;++y)memcpy(actual.data()+size_t(y)*w*4,(char*)map.pData+size_t(y)*map.RowPitch,w*4);g.context->Unmap(staging.Get(),0);
    read(n+index,truth);double mse=0,hmse=0,bmse=0,t=double(index)/multiplier;bool equalA=true,equalB=true;
    for(size_t i=0;i<bytes;i+=4)for(unsigned c=0;c<3;++c){double v=actual[i+c],gt=truth[i+c],av=a[i+c],bv=b[i+c];mse+=(v-gt)*(v-gt);hmse+=(av-gt)*(av-gt);double mix=av*(1-t)+bv*t;bmse+=(mix-gt)*(mix-gt);equalA&=v==av;equalB&=v==bv;}
    double q=psnr(mse,double(w)*h*3),hq=psnr(hmse,double(w)*h*3),bq=psnr(bmse,double(w)*h*3);score+=q;hold+=hq;blend+=bq;++outputs;copies+=equalA||equalB;
    rows<<n+index<<','<<index<<','<<multiplier<<','<<q<<','<<hq<<','<<bq<<','<<equalA<<','<<equalB<<','<<gpu<<'\n';
    if(q<worstScore){worstScore=q;worst=actual;worstTruth=truth;}
    if(video.is_open())video.write((char*)actual.data(),bytes);
   }
  }
  if(video.is_open()){video.write((char*)b.data(),bytes);if(!video)throw std::runtime_error("Benchmark video write failed");}
  std::sort(times.begin(),times.end());double mean=0;for(float t:times)mean+=t;mean/=std::max(size_t(1),times.size());
  double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
  summary<<name.substr(0,name.find('-'))<<','<<multiplier<<','<<60./multiplier<<",60,"<<outputs<<','<<copies<<','<<score/outputs<<','<<hold/outputs<<','<<blend/outputs<<','<<mean<<','<<times[std::min(times.size()-1,size_t(times.size()*.95))]<<','<<times.back()<<','<<elapsed<<std::endl;
  bmp(folder/(name+"-worst-generated.bmp"),worst);bmp(folder/(name+"-worst-truth.bmp"),worstTruth);
 }
 std::ofstream(folder/L"complete.txt")<<"Held-out reconstruction at original 60 FPS. Inputs 30/20/15 FPS for 2x/3x/4x. PSNR is RGB per generated frame, arithmetic mean, exact matches capped at 100 dB. GPU times exclude first three pairs, include VFG + D3D output copy, exclude upload, readback, metrics, disk and presentation. Not physical latency.\n";
}
