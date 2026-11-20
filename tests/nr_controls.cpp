#include "NrControls.hpp"
#include "imgui_impl_dx11.h"
#include <iostream>
#include <vector>

// Offscreen integration test of the actual preview draw path. No window input.
int main(){try{
 auto settings=std::filesystem::temp_directory_path()/L"framelab-nr-passes-test.txt";
 {std::ofstream(settings)<<"1 0.65 0.5 1 720 0 1\n";NrControls old;old.load(settings);if(old.passes!=1)throw std::runtime_error("Old settings must use one pass");}
 {NrControls saved;saved.passes=4;saved.strength=.42f;saved.economy=true;saved.save(settings);NrControls loaded;loaded.load(settings);if(!loaded.economy||loaded.passes!=4||std::abs(loaded.strength-.42f)>.001f)throw std::runtime_error("NR settings did not round-trip");}
 {std::ofstream(settings)<<"2 1 0.5 1 1440 0 1 1\n";NrControls old;old.economy=true;old.load(settings);if(old.economy||old.height!=1440||old.strength!=1)throw std::runtime_error("Existing users must keep the standard mode");}
 {std::ofstream(settings)<<"3 1 0.5 1 1440 0 1 1 77\n";NrControls invalid;invalid.load(settings);if(invalid.economy)throw std::runtime_error("Invalid economy mode must not be enabled");}
 {std::ofstream(settings)<<"2 0.65 0.5 1 720 0 1 99\n";NrControls invalid;invalid.load(settings);if(invalid.passes!=4)throw std::runtime_error("NR passes not bounded");}
 std::filesystem::remove(settings);
 Graphics g;g.initDevice();constexpr unsigned w=256,h=128;
 Enhancer fixture(g,w,h,h,std::filesystem::temp_directory_path(),L"missing-test-runtime");
 if(fixture.state()!=Enhancer::State::Failed||fixture.error().find("missing")==std::string::npos)throw std::runtime_error("Missing runtime was not handled");
 fixture.original.create(g.device.Get(),w,h);fixture.result.create(g.device.Get(),w,h);fixture.hasResult=true;
 std::vector<unsigned> red(w*h,0xffff0000),blue(w*h,0xff0000ff);
 g.context->UpdateSubresource(fixture.original.texture.Get(),0,nullptr,red.data(),w*4,0);
 g.context->UpdateSubresource(fixture.result.texture.Get(),0,nullptr,blue.data(),w*4,0);
 Texture target;target.create(g.device.Get(),w,h);ComPtr<ID3D11RenderTargetView> rtv;check(g.device->CreateRenderTargetView(target.texture.Get(),nullptr,&rtv),"Test RTV");
 D3D11_TEXTURE2D_DESC desc{};target.texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ComPtr<ID3D11Texture2D> read;check(g.device->CreateTexture2D(&desc,nullptr,&read),"Test staging");
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={float(w),float(h)};io.DeltaTime=1.f/60;
 if(!ImGui_ImplDX11_Init(g.device.Get(),g.context.Get()))throw std::runtime_error("ImGui backend init failed");
 struct Case {bool enabled,matched;int view;float strength;unsigned left,right;bool clean=false;};
 const Case cases[]={{false,true,1,1,0xffff0000,0xffff0000},{true,true,1,0,0xffff0000,0xffff0000},{true,true,1,1,0xff0000ff,0xff0000ff},{true,true,2,1,0xffff0000,0xff0000ff},{true,true,0,1,0xffff0000,0xffff0000},{true,false,1,1,0xffff0000,0xffff0000},{true,true,1,.5f,0xff7f0080,0xff7f0080},
  {true,true,0,1,0xff0000ff,0xff0000ff,true},{true,true,1,1,0xff0000ff,0xff0000ff,true},{true,true,2,1,0xff0000ff,0xff0000ff,true},{false,true,2,1,0xffff0000,0xffff0000,true},{true,true,2,0,0xffff0000,0xffff0000,true}};
 for(const auto& c:cases){
  ImGui_ImplDX11_NewFrame();ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({float(w),float(h)});
  ImGui::Begin("test",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);ImGui::SetCursorScreenPos({0,0});
  NrControls controls;controls.enabled=c.enabled;controls.view=c.view;controls.strength=c.strength;controls.draw(fixture.original.view.Get(),{float(w),float(h)},&fixture,c.matched,c.clean);ImGui::End();ImGui::Render();
  const float clear[]={0,1,0,1};g.context->ClearRenderTargetView(rtv.Get(),clear);g.context->OMSetRenderTargets(1,rtv.GetAddressOf(),nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  g.context->CopyResource(read.Get(),target.texture.Get());D3D11_MAPPED_SUBRESOURCE m{};check(g.context->Map(read.Get(),0,D3D11_MAP_READ,0,&m),"Test readback");
  // In clean output sample where the normal preview labels would obscure pixels.
  auto row=reinterpret_cast<unsigned*>(static_cast<unsigned char*>(m.pData)+(c.clean?15:100)*m.RowPitch);const auto a=row[32],b=row[224];
  if(c.clean){auto middle=reinterpret_cast<unsigned*>(static_cast<unsigned char*>(m.pData)+64*m.RowPitch);if(middle[128]!=c.right)throw std::runtime_error("Comparison divider leaked into fullscreen");}
  g.context->Unmap(read.Get(),0);
  auto close=[](unsigned a,unsigned b){for(int s=0;s<32;s+=8)if(std::abs(int((a>>s)&255)-int((b>>s)&255))>1)return false;return true;};
  if(!close(a,c.left)||!close(b,c.right))throw std::runtime_error("Preview strength/split/reference mismatch");
 }
 ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();std::cout<<"PASS: off, zero/full/half strength, original, split, unmatched frame, missing runtime, clean fullscreen pixels\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
