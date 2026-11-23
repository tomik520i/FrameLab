#pragma once
#include "Graphics.hpp"
#include <filesystem>
#include <memory>

// Persistent native ONNX Runtime worker. DirectML uses the same DXGI adapter as
// the renderer. One pair is in flight; no Python process or unbounded queue.
class RifeDirectML {
 struct Impl;std::shared_ptr<Impl> p;
public:
 RifeDirectML()=default;~RifeDirectML();
 RifeDirectML(const RifeDirectML&)=delete;RifeDirectML& operator=(const RifeDirectML&)=delete;
 static bool supportedAdapter(ID3D11Device* device);
 static bool available(const std::filesystem::path& runtime);
 void initialize(ID3D11Device* device,ID3D11Texture2D* output,unsigned width,unsigned height,unsigned quality,const std::filesystem::path& runtime,bool automaticCuts);
 void upload(const unsigned char* pixels,unsigned parity);
 void upload(ID3D11Texture2D* texture,unsigned parity);
 void generate(unsigned current,bool cut,unsigned index,unsigned multiplier);
 bool ready();
 bool loading()const;
 float processingMs()const; // worker wall time, not a GPU timestamp
 void reset();
};
