// Deterministic, sequential export through the SAME enhancer used by the app.
// Readback here is diagnostic only; elapsed export time is not live FPS.
#include "Enhancer.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>

int wmain(int argc,wchar_t** argv){try{
 if(argc!=7)throw std::runtime_error("nr_export app-dir input.bgra output.bgra width height inference-height");
 const auto app=std::filesystem::absolute(argv[1]);const unsigned w=_wtoi(argv[4]),h=_wtoi(argv[5]),ih=_wtoi(argv[6]);
 if(!w||!h||w>7680||h>4320)throw std::runtime_error("Invalid dimensions");
 const size_t bytes=size_t(w)*h*4,total=std::filesystem::file_size(argv[2]);if(total%bytes||!total)throw std::runtime_error("Incomplete raw input");
 std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("Cannot open export files");
 Graphics g;g.initDevice();Texture source;source.create(g.device.Get(),w,h);
 Enhancer nr(g,w,h,ih,app,app/L"runtime"/L"nr-sf");
 auto wait=[&](bool loading){auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(150);
  while(loading?nr.state()==Enhancer::State::Loading:!nr.take()){
   if(nr.state()==Enhancer::State::Failed)throw std::runtime_error(nr.error());
   if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("NR export timeout");Sleep(1);
  }
  if(nr.state()==Enhancer::State::Failed)throw std::runtime_error(nr.error());
 };
 wait(true);
 D3D11_TEXTURE2D_DESC d{};nr.result.texture->GetDesc(&d);d.Usage=D3D11_USAGE_STAGING;d.BindFlags=d.MiscFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ComPtr<ID3D11Texture2D> resultRead,referenceRead;check(g.device->CreateTexture2D(&d,nullptr,&resultRead),"Export result staging");d.Format=DXGI_FORMAT_B8G8R8A8_UNORM;check(g.device->CreateTexture2D(&d,nullptr,&referenceRead),"Export reference staging");
 std::vector<unsigned char> input(bytes),output(bytes);size_t frame=0;double absoluteDifference=0;
 while(in.read(reinterpret_cast<char*>(input.data()),bytes)){
  g.context->UpdateSubresource(source.texture.Get(),0,nullptr,input.data(),w*4,0);
  if(!nr.submit(source.texture.Get(),frame==0,true))throw std::runtime_error("NR submit refused");wait(false);
  g.context->CopyResource(referenceRead.Get(),nr.original.texture.Get());D3D11_MAPPED_SUBRESOURCE mapped{};check(g.context->Map(referenceRead.Get(),0,D3D11_MAP_READ,0,&mapped),"Export reference map");
  bool exact=true;for(unsigned y=0;y<h;++y)exact&=memcmp(input.data()+size_t(y)*w*4,static_cast<unsigned char*>(mapped.pData)+size_t(y)*mapped.RowPitch,w*4)==0;g.context->Unmap(referenceRead.Get(),0);if(!exact)throw std::runtime_error("Input/reference mismatch");
  g.context->CopyResource(resultRead.Get(),nr.result.texture.Get());check(g.context->Map(resultRead.Get(),0,D3D11_MAP_READ,0,&mapped),"Export output map");
  for(unsigned y=0;y<h;++y){auto src=static_cast<unsigned char*>(mapped.pData)+size_t(y)*mapped.RowPitch;auto dst=output.data()+size_t(y)*w*4;
   for(unsigned x=0;x<w;++x){dst[4*x]=src[4*x+2];dst[4*x+1]=src[4*x+1];dst[4*x+2]=src[4*x];dst[4*x+3]=255;}}
  g.context->Unmap(resultRead.Get(),0);for(size_t i=0;i<bytes;++i)if(i%4!=3)absoluteDifference+=std::abs(int(input[i])-int(output[i]));
  out.write(reinterpret_cast<char*>(output.data()),bytes);if(!out)throw std::runtime_error("Export write failed");++frame;if(frame%30==0)std::cout<<"exported "<<frame<<" frames\n"<<std::flush;
 }
 if(frame!=total/bytes)throw std::runtime_error("Export did not consume every input frame");
 out.close();std::ofstream meta(std::filesystem::path(argv[3]).wstring()+L".json");
 meta<<"{\"frames\":"<<frame<<",\"width\":"<<w<<",\"height\":"<<h<<",\"inference_width\":"<<nr.inferenceWidth()<<",\"inference_height\":"<<nr.inferenceHeight()<<",\"reference_errors\":0,\"rgb_mae\":"<<absoluteDifference/(frame*w*h*3)<<",\"strength\":1,\"motion_estimate\":true,\"fg\":false,\"note\":\"Offline sequential export, no dropped frames; pixel difference is not a quality score or live performance measurement.\"}\n";
 std::cout<<"PASS: "<<frame<<" exact paired references\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
