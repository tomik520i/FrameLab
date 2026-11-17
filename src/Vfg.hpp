#pragma once
#include "Graphics.hpp"
#include <memory>
#include <filesystem>
class Vfg {
 struct Impl;std::unique_ptr<Impl> p;
public:
 Vfg();~Vfg();
 void initialize(ID3D11Device* device,ID3D11Texture2D* output,unsigned width,unsigned height,unsigned mode,const std::filesystem::path& runtime,bool automaticCuts=true);
 void upload(const unsigned char* pixels,unsigned parity);
 void generate(unsigned current,bool cut,unsigned index=1,unsigned multiplier=2);
 bool ready();
 float gpuMs() const;
 void reset();
};
