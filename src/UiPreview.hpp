#pragma once
#include "Graphics.hpp"
#include <filesystem>
#include <fstream>

// Development-only export of this application's render target. No desktop,
// other windows, capture device or screen-capture API is involved.
inline void saveUiPreview(Graphics& g,const std::filesystem::path& path){
 ComPtr<ID3D11Resource> resource;g.target->GetResource(&resource);
 ComPtr<ID3D11Texture2D> source;check(resource.As(&source),"UI preview texture");
 D3D11_TEXTURE2D_DESC desc{};source->GetDesc(&desc);
 desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
 ComPtr<ID3D11Texture2D> staging;check(g.device->CreateTexture2D(&desc,nullptr,&staging),"UI preview readback");
 g.context->CopyResource(staging.Get(),source.Get());
 D3D11_MAPPED_SUBRESOURCE map{};check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"UI preview map");
 BITMAPFILEHEADER file{};BITMAPINFOHEADER bitmap{};
 file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(bitmap);file.bfSize=file.bfOffBits+desc.Width*desc.Height*4;
 bitmap.biSize=sizeof(bitmap);bitmap.biWidth=desc.Width;bitmap.biHeight=-LONG(desc.Height);bitmap.biPlanes=1;bitmap.biBitCount=32;bitmap.biCompression=BI_RGB;
 std::ofstream out(path,std::ios::binary);out.write((char*)&file,sizeof(file));out.write((char*)&bitmap,sizeof(bitmap));
 for(unsigned y=0;y<desc.Height;++y)out.write((char*)map.pData+size_t(y)*map.RowPitch,size_t(desc.Width)*4);
 g.context->Unmap(staging.Get(),0);out.close();if(!out)throw std::runtime_error("Cannot write UI preview");
}
