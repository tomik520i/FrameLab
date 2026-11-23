#pragma once
#include "Graphics.hpp"
#include <d3d11_4.h>
#include <d3d12.h>

// Hardware eligibility is not NVIDIA certification. ShortFuse is a separate
// community runtime with SM75/86/89 compatibility kernels; GTX16 is not RTX.
struct NrHardware {
 bool nvidia=false,rtx=false,d3d12=false,fences=false;int sm=0;
 bool supports(int runtime)const{
  if(!nvidia||!rtx||!d3d12||!fences)return false;
  return runtime==1?sm==120:(sm==75||sm==86||sm==89||sm==120);
 }
 const char* reason(int runtime,bool cs)const{
  if(!nvidia)return cs?"NR vyžaduje NVIDIA RTX. Na AMD/Intelu lze použít Optical Flow nebo RIFE; tento enhancer zapnout nelze.":"NR requires NVIDIA RTX. AMD/Intel can use Optical Flow or RIFE; this enhancer cannot run.";
  if(!rtx)return cs?"Tento NR runtime vyžaduje RTX s Tensor jádry; GeForce GTX není podporovaná.":"This NR runtime requires RTX Tensor hardware; GeForce GTX is unsupported.";
  if(!d3d12||!fences)return cs?"NR vyžaduje DirectX 12 a sdílené D3D11.4 fence. Aktuální ovladač tuto cestu nenabízí.":"NR requires DirectX 12 and shared D3D11.4 fences. The current driver does not expose this path.";
  if(!sm)return cs?"Architekturu GPU se nepodařilo ověřit přes ovladač NVIDIA; NR je vypnuté.":"The NVIDIA driver could not verify GPU architecture; NR is disabled.";
  if(runtime==1&&sm!=120)return cs?"Originální NR 310.8 je určené pro RTX 50 / Blackwell. Pro jiné RTX může být dostupný samostatný komunitní profil ShortFuse.":"Original NR 310.8 targets RTX 50 / Blackwell. Other RTX cards may use the separate community ShortFuse profile.";
  return cs?"Pro tuto architekturu není ověřená kompatibilita vybraného NR runtime; zapnutí je zablokované.":"The selected NR runtime has no established compatibility with this architecture; enabling is blocked.";
 }
 static NrHardware query(ID3D11Device* device,int architecture){
  NrHardware result;result.sm=architecture;ComPtr<IDXGIDevice> dx;ComPtr<IDXGIAdapter> adapter;DXGI_ADAPTER_DESC desc{};
  if(FAILED(device->QueryInterface(IID_PPV_ARGS(&dx)))||FAILED(dx->GetAdapter(&adapter))||FAILED(adapter->GetDesc(&desc)))return result;
  result.nvidia=desc.VendorId==0x10de;result.rtx=std::wstring(desc.Description).find(L"RTX")!=std::wstring::npos;
  result.d3d12=SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,__uuidof(ID3D12Device),nullptr));
  ComPtr<ID3D11Device5> modern;result.fences=SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&modern)));return result;
 }
};
