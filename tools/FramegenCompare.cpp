#include "FrameGenerator.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include <vector>
#include <mmsystem.h>

// Identical decoded BGRA triplets: A=2*i, truth=2*i+1, B=2*i+2.
// No renderer/pacer. CSV wall_ms includes CPU input copies, dispatch and wait;
// readback_ms is separate. RIFE worker_ms is not a GPU timestamp.
int main(int argc,char** argv){try{
 timeBeginPeriod(1);struct Timer{~Timer(){timeEndPeriod(1);}} timer;
 if(argc!=9)throw std::runtime_error("adapter backend quality width height runtime input.bgra output-prefix");
 int adapter=std::stoi(argv[1]),backend=std::stoi(argv[2]),quality=std::stoi(argv[3]);
 unsigned w=std::stoul(argv[4]),h=std::stoul(argv[5]);size_t bytes=size_t(w)*h*4;
 std::ifstream source(argv[7],std::ios::binary|std::ios::ate);if(!source)throw std::runtime_error("Input missing");
 auto size=size_t(source.tellg());source.seekg(0);std::vector<unsigned char> frames(size);source.read((char*)frames.data(),size);
 if(size%bytes||size/bytes<3)throw std::runtime_error("Incomplete source frames");
 Graphics g;g.initDevice(adapter);Texture out;out.create(g.device.Get(),w,h);FrameGenerator fg;fg.selectBackend(backend);
 std::cout<<g.adapterName<<" | "<<fg.name()<<" | quality "<<quality<<std::endl;
 auto init=std::chrono::steady_clock::now();fg.initialize(g.device.Get(),out.texture.Get(),w,h,quality,std::filesystem::path(argv[6]),true);
 std::ofstream csv(std::string(argv[8])+".csv");csv<<"pair,wall_ms,worker_ms,gpu_ms,readback_ms\n";
 std::ofstream images(std::string(argv[8])+".bgra",std::ios::binary);
 if(!csv||!images)throw std::runtime_error("Cannot create benchmark output files; use a fresh output prefix");
 D3D11_TEXTURE2D_DESC desc{};out.texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;
 check(g.device->CreateTexture2D(&desc,nullptr,&staging),"benchmark staging");std::vector<unsigned char> pixels(bytes);
 int count=int((size/bytes-1)/2);
 for(int i=-10;i<count;++i){unsigned pair=i<0?unsigned((i+10)%count):unsigned(i);auto t=std::chrono::steady_clock::now();
  fg.upload(frames.data()+pair*2*bytes,0);fg.upload(frames.data()+(pair*2+2)*bytes,1);fg.generate(1,false,1,2);
  auto deadline=GetTickCount64()+180000;while(!fg.ready()){if(GetTickCount64()>deadline)throw std::runtime_error("Inference timeout");Sleep(1);}
  auto completed=std::chrono::steady_clock::now();double wall=std::chrono::duration<double,std::milli>(completed-t).count();
  if(i==-10)std::cout<<"first_ready_ms="<<std::chrono::duration<double,std::milli>(completed-init).count()<<std::endl;
  if(i<0)continue;
  g.context->CopyResource(staging.Get(),out.texture.Get());D3D11_MAPPED_SUBRESOURCE m{};check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&m),"benchmark readback");
  for(unsigned y=0;y<h;++y)memcpy(pixels.data()+size_t(y)*w*4,(char*)m.pData+size_t(y)*m.RowPitch,w*4);g.context->Unmap(staging.Get(),0);
  auto read=std::chrono::steady_clock::now();double readMs=std::chrono::duration<double,std::milli>(read-completed).count();
  csv<<i<<','<<wall<<','<<fg.processingMs()<<','<<fg.gpuMs()<<','<<readMs<<std::endl;
  images.write((char*)pixels.data(),pixels.size());
  if(!csv||!images)throw std::runtime_error("Benchmark output write failed");
  if(i%10==0)std::cout<<i<<"/"<<count<<" wall_ms="<<wall<<std::endl;
 }
 std::cout<<"PASS "<<count<<" matched triplets"<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
