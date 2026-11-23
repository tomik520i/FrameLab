#include "RifeDirectML.hpp"
#include "NrHardware.hpp"
#include "Vfg.hpp"
#include <cmath>
#include <iostream>

using Pixels=std::vector<unsigned char>;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Pixels fixture(unsigned w,unsigned h,double dx,double dy,bool hud=false){
 Pixels p(size_t(w)*h*4);
 for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){double xx=x-dx,yy=y-dy;auto i=(size_t(y)*w+x)*4;
  for(int c=0;c<3;++c)p[i+c]=(unsigned char)std::clamp(112+43*sin(xx*.11+c*1.6)+32*cos(yy*.17+c)+28*sin(xx*.037+yy*.043+c*.8)+20*cos(xx*.29-yy*.13),0.,255.);
  if(hud&&x>12&&x<96&&y>12&&y<35)p[i]=p[i+1]=p[i+2]=((x/4+y/3)%3)?245:20;p[i+3]=255;
 }return p;
}
Pixels readback(Graphics& g,Texture& image,unsigned w,unsigned h){
 D3D11_TEXTURE2D_DESC d{};image.texture->GetDesc(&d);d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ComPtr<ID3D11Texture2D> staging;check(g.device->CreateTexture2D(&d,nullptr,&staging),"Test staging");g.context->CopyResource(staging.Get(),image.texture.Get());
 D3D11_MAPPED_SUBRESOURCE m{};check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&m),"Test map");Pixels p(size_t(w)*h*4);
 for(unsigned y=0;y<h;++y)memcpy(p.data()+size_t(y)*w*4,(char*)m.pData+size_t(y)*m.RowPitch,w*4);g.context->Unmap(staging.Get(),0);return p;
}
double error(const Pixels& a,const Pixels& b,unsigned w,unsigned x0,unsigned y0,unsigned x1,unsigned y1){
 double sum=0,count=0;for(unsigned y=y0;y<y1;++y)for(unsigned x=x0;x<x1;++x)for(unsigned c=0;c<3;++c){auto i=(size_t(y)*w+x)*4+c;sum+=abs(int(a[i])-int(b[i]));++count;}return sum/count;
}
void wait(RifeDirectML& f){auto deadline=GetTickCount64()+180000;while(!f.ready()){require(GetTickCount64()<deadline,"GPU did not complete");Sleep(1);}}
template<class F> void rejects(F&& f){bool rejected=false;try{f();}catch(const std::runtime_error&){rejected=true;}require(rejected,"Invalid/busy input accepted");}
int main(int argc,char** argv){try{
 if(argc<3)throw std::runtime_error("adapter runtime-folder");
 auto runtime=std::filesystem::absolute(argv[2]);
 for(int sm:{0,52,61,75,80,86,89,90,120}){
  NrHardware hw{true,true,true,true,sm};
  require(hw.supports(1)==(sm==120),"Original NR hardware gate incorrect");
  require(hw.supports(0)==(sm==75||sm==86||sm==89||sm==120),"Community NR hardware gate incorrect");
  hw.nvidia=false;require(!hw.supports(0)&&!hw.supports(1),"Non NVIDIA accepted NR");
  hw.nvidia=true;hw.rtx=false;require(!hw.supports(0),"GTX accepted NR");
 }

 Graphics g;g.initDevice(argc>1?std::stoi(argv[1]):-1);std::cout<<"Adapter: "<<g.adapterName<<" index="<<g.adapterIndex<<'\n';
 if(g.adapterVendor!=0x10de)require(!Vfg::supportedAdapter(g.device.Get()),"Non-NVIDIA adapter advertised VFG");
 constexpr unsigned w=317,h=181;Texture out,source;out.create(g.device.Get(),w,h);source.create(g.device.Get(),w,h);RifeDirectML f;
 f.initialize(g.device.Get(),out.texture.Get(),w,h,1,runtime,true);
 auto a=fixture(w,h,0,0),b=a;
 f.upload(a.data(),0);f.upload(b.data(),1);f.generate(1,false,1,2);
 rejects([&]{f.upload(a.data(),0);});rejects([&]{f.generate(1,false,1,2);});wait(f);
 require(readback(g,out,w,h)==a,"Static frame changed");require(!f.ready(),"Ready returned stale completion");
 rejects([&]{f.upload(a.data(),2);});rejects([&]{f.generate(2,false,1,2);});rejects([&]{f.generate(1,false,2,2);});
 for(auto delta:{std::pair{16.,8.},std::pair{-32.,0.},std::pair{12.,0.}}){
  a=fixture(w,h,0,0,true);b=fixture(w,h,delta.first,delta.second,true);f.upload(a.data(),0);
  g.context->UpdateSubresource(source.texture.Get(),0,nullptr,b.data(),w*4,0);f.upload(source.texture.Get(),1);
  for(unsigned factor=2;factor<=4;++factor)for(unsigned phase=1;phase<factor;++phase){
   double t=double(phase)/factor;auto truth=fixture(w,h,delta.first*t,delta.second*t,true);Pixels blended(a.size());
   for(size_t i=0;i<a.size();++i)blended[i]=(unsigned char)std::lround(a[i]*(1-t)+b[i]*t);
   f.generate(1,false,phase,factor);wait(f);auto actual=readback(g,out,w,h);
   double e=error(actual,truth,w,40,40,w-40,h-40),base=error(blended,truth,w,40,40,w-40,h-40);
   std::cout<<"motion="<<delta.first<<","<<delta.second<<" phase="<<phase<<"/"<<factor<<" MAE="<<e<<" blend="<<base<<'\n';
   require(e<base*.85,"Interpolation did not improve on simple blending");
   require(error(actual,truth,w,16,16,92,31)<8,"Fixed HUD was warped");
  }
 }
 a=Pixels(size_t(w)*h*4,0);b=Pixels(a.size(),255);for(size_t i=3;i<a.size();i+=4)a[i]=255;
 f.upload(a.data(),1);f.upload(b.data(),0);f.generate(0,false,1,2);wait(f);require(readback(g,out,w,h)==b,"Automatic cut blended distinct scenes");
 f.initialize(g.device.Get(),out.texture.Get(),w,h,0,runtime,false);f.upload(a.data(),1);f.upload(b.data(),0);f.generate(0,true,1,3);wait(f);require(readback(g,out,w,h)==b,"Explicit cut changed current frame");
 // GPU texture input must agree with the exact same CPU pixels.
 f.initialize(g.device.Get(),out.texture.Get(),w,h,0,runtime,false);
 a=fixture(w,h,0,0);b=fixture(w,h,8,0);f.upload(a.data(),0);f.upload(b.data(),1);f.generate(1,false,1,2);wait(f);auto cpu=readback(g,out,w,h);
 g.context->UpdateSubresource(source.texture.Get(),0,nullptr,b.data(),w*4,0);f.upload(source.texture.Get(),1);f.generate(1,false,1,2);wait(f);
 require(cpu==readback(g,out,w,h),"CPU/GPU input paths disagree");
 // Reversed buffer parity must still mean chronological A -> B.
 f.upload(a.data(),1);f.upload(b.data(),0);f.generate(0,false,1,2);wait(f);require(cpu==readback(g,out,w,h),"Parity reversed frame order");
 std::cout<<"PASS: static, 18 motion/phase combinations, fixed HUD, cuts, texture upload, parity, validation, reinitialization\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
