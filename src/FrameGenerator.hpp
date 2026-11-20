#pragma once
#include "Vfg.hpp"
#include "OpticalFlow.hpp"
// Backend boundary: capture, presentation, audio and diagnostics are shared.
class FrameGenerator {
 Vfg nvidia;std::unique_ptr<OpticalFlow> optical;
public:
 bool useOptical=false;
 const char* name()const{return useOptical?"D3D11 Optical Flow (experimental)":"NVIDIA VFG";}
 void initialize(ID3D11Device* d,ID3D11Texture2D* t,unsigned w,unsigned h,unsigned mode,const std::filesystem::path& runtime,bool cuts=true){
  reset();if(useOptical){optical=std::make_unique<OpticalFlow>();optical->initialize(d,t,w,h);}else nvidia.initialize(d,t,w,h,mode,runtime,cuts);
 }
 void upload(const unsigned char* p,unsigned i){if(optical)optical->upload(p,i);else nvidia.upload(p,i);}
 void upload(ID3D11Texture2D* t,unsigned i){if(optical)optical->upload(t,i);else nvidia.upload(t,i);}
 void generate(unsigned current,bool cut,unsigned index=1,unsigned multiplier=2){if(optical)optical->generate(current,cut,index,multiplier);else nvidia.generate(current,cut,index,multiplier);}
 bool ready(){return optical?optical->ready():nvidia.ready();}
 float gpuMs()const{return optical?optical->gpuMs():nvidia.gpuMs();}
 void reset(){optical.reset();nvidia.reset();}
};
