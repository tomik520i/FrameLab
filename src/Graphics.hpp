#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <stdexcept>
#include <string>
using Microsoft::WRL::ComPtr;
inline void check(HRESULT h,const char* s){if(FAILED(h))throw std::runtime_error(std::string(s)+" HRESULT="+std::to_string(unsigned(h)));}
struct Texture {
 ComPtr<ID3D11Texture2D> texture;
 ComPtr<ID3D11ShaderResourceView> view;
 void create(ID3D11Device* device,unsigned w,unsigned h);
};
struct Graphics {
 ComPtr<ID3D11Device> device;
 ComPtr<ID3D11DeviceContext> context;
 ComPtr<IDXGISwapChain2> swap;
 ComPtr<ID3D11RenderTargetView> target;
 HANDLE waitable=nullptr; bool tearing=false;
 void init(HWND window);
 void resize(unsigned w,unsigned h);
 void begin();
 HRESULT present(bool vsync);
 ~Graphics();
};
