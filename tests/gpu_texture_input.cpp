#include "TextureConvert.hpp"
#include "FrameGenerator.hpp"
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>
std::vector<unsigned> read(Graphics& g,Texture& image){
 D3D11_TEXTURE2D_DESC d{};image.texture->GetDesc(&d);d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;d.MiscFlags=0;
 ComPtr<ID3D11Texture2D> staging;check(g.device->CreateTexture2D(&d,nullptr,&staging),"Readback");g.context->CopyResource(staging.Get(),image.texture.Get());
 D3D11_MAPPED_SUBRESOURCE m{};check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&m),"Map");std::vector<unsigned> pixels(d.Width*d.Height);
 for(unsigned y=0;y<d.Height;++y)memcpy(pixels.data()+y*d.Width,static_cast<char*>(m.pData)+y*m.RowPitch,d.Width*4);
 g.context->Unmap(staging.Get(),0);return pixels;
}
int main(int argc,char** argv){try{
 Graphics g;g.initDevice();constexpr unsigned w=640,h=360;Texture rgba,bgra,out[2];bgra.create(g.device.Get(),w,h);for(auto& t:out)t.create(g.device.Get(),w,h);
 D3D11_TEXTURE2D_DESC d{};bgra.texture->GetDesc(&d);d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;check(g.device->CreateTexture2D(&d,nullptr,&rgba.texture),"RGBA");check(g.device->CreateShaderResourceView(rgba.texture.Get(),nullptr,&rgba.view),"RGBA view");
 std::vector<unsigned> pixels(w*h);for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)pixels[y*w+x]=0xff000000|((x*255/w)<<16)|((y*255/h)<<8)|23;
 g.context->UpdateSubresource(rgba.texture.Get(),0,nullptr,pixels.data(),w*4,0);TextureConvert converter;converter.copy(g,rgba.view.Get(),bgra,w,h);
 auto converted=read(g,bgra);for(size_t i=0;i<pixels.size();++i){auto p=pixels[i];auto expected=(p&0xff00ff00)|((p&255)<<16)|((p>>16)&255);if(converted[i]!=expected)throw std::runtime_error("RGBA/BGRA conversion changed channels or pixels");}
 if(argc>1){for(bool optical:{false,true}){
  FrameGenerator cpu,gpu;cpu.useOptical=gpu.useOptical=optical;cpu.initialize(g.device.Get(),out[0].texture.Get(),w,h,0,argv[1]);gpu.initialize(g.device.Get(),out[1].texture.Get(),w,h,0,argv[1]);
  for(unsigned frame=0;frame<5;++frame){unsigned parity=frame%2;
   for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)converted[y*w+x]=0xff000000|(((x+frame*9)%w*255/w)<<16)|((y*255/h)<<8)|23;
   g.context->UpdateSubresource(bgra.texture.Get(),0,nullptr,converted.data(),w*4,0);cpu.upload(reinterpret_cast<const unsigned char*>(converted.data()),parity);gpu.upload(bgra.texture.Get(),parity);
   if(!frame)continue;
   for(unsigned phase=1;phase<4;++phase){cpu.generate(parity,false,phase,4);gpu.generate(parity,false,phase,4);bool a=false,b=false;auto start=std::chrono::steady_clock::now();
    while(!a||!b){if(!a)a=cpu.ready();if(!b)b=gpu.ready();if(std::chrono::steady_clock::now()-start>std::chrono::seconds(20))throw std::runtime_error("FG timeout");std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    auto expected=read(g,out[0]),actual=read(g,out[1]);if(actual!=expected)throw std::runtime_error("GPU texture input differs from CPU pixel input");
   }
  }
 }}
 std::cout<<"PASS: exact GPU RGBA/BGRA conversion; CPU/GPU FG uploads across both parities and 4x phases\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
