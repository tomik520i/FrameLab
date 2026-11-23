#pragma once
#include "Vfg.hpp"
#include "OpticalFlow.hpp"
#include "RifeDirectML.hpp"
// Backend boundary: capture, presentation, audio and diagnostics are shared.
class FrameGenerator {
 Vfg nvidia;std::unique_ptr<OpticalFlow> optical;std::unique_ptr<RifeDirectML> rife;
public:
 bool useOptical=false,useRife=false;
 const char* name()const{return useRife?"RIFE 4.22 lite / DirectML":useOptical?"D3D11 Optical Flow v2 (experimental)":"NVIDIA VFG";}
 int backend()const{return useRife?2:useOptical?1:0;}
 void selectBackend(int value){useRife=value==2;useOptical=value==1;}
 void initialize(ID3D11Device* d,ID3D11Texture2D* t,unsigned w,unsigned h,unsigned mode,const std::filesystem::path& runtime,bool cuts=true){
  reset();if(useRife){rife=std::make_unique<RifeDirectML>();rife->initialize(d,t,w,h,mode,runtime,cuts);}else if(useOptical){optical=std::make_unique<OpticalFlow>();optical->initialize(d,t,w,h,mode,cuts);}else nvidia.initialize(d,t,w,h,mode,runtime,cuts);
 }
 void upload(const unsigned char* p,unsigned i){if(rife)rife->upload(p,i);else if(optical)optical->upload(p,i);else nvidia.upload(p,i);}
 void upload(ID3D11Texture2D* t,unsigned i){if(rife)rife->upload(t,i);else if(optical)optical->upload(t,i);else nvidia.upload(t,i);}
 void generate(unsigned current,bool cut,unsigned index=1,unsigned multiplier=2){if(rife)rife->generate(current,cut,index,multiplier);else if(optical)optical->generate(current,cut,index,multiplier);else nvidia.generate(current,cut,index,multiplier);}
 bool ready(){return rife?rife->ready():optical?optical->ready():nvidia.ready();}
 bool loading()const{return rife&&rife->loading();}
 float gpuMs()const{return rife?-1:optical?optical->gpuMs():nvidia.gpuMs();}
 float processingMs()const{return rife?rife->processingMs():gpuMs();}
 void reset(){rife.reset();optical.reset();nvidia.reset();}
};
