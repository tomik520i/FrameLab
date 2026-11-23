#pragma once
#include "Graphics.hpp"
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <deque>
#include <cmath>
#include <vector>
#pragma comment(lib,"d2d1.lib")
#pragma comment(lib,"dwrite.lib")
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")

// DirectWrite performs Unicode shaping, bidi, line breaking and system font
// fallback. Rasterize only when text/size changes, at the actual output size.
// The ordinary ImGui atlas is deliberately not expanded to every CJK glyph.
class SubtitleRaster {
 struct Renderer final:IDWriteTextRenderer {
  ID2D1RenderTarget* target;ID2D1Brush* brush;unsigned missing=0;
  Renderer(ID2D1RenderTarget* t,ID2D1Brush* b):target(t),brush(b){}
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(id==__uuidof(IUnknown)||id==__uuidof(IDWritePixelSnapping)||id==__uuidof(IDWriteTextRenderer)){*out=static_cast<IDWriteTextRenderer*>(this);return S_OK;}return E_NOINTERFACE;}
  ULONG STDMETHODCALLTYPE AddRef()override{return 1;}ULONG STDMETHODCALLTYPE Release()override{return 1;}
  HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void*,BOOL* value)override{*value=FALSE;return S_OK;}
  HRESULT STDMETHODCALLTYPE GetCurrentTransform(void*,DWRITE_MATRIX* value)override{D2D1_MATRIX_3X2_F m;target->GetTransform(&m);memcpy(value,&m,sizeof(m));return S_OK;}
  HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void*,FLOAT* value)override{*value=1;return S_OK;}
  HRESULT STDMETHODCALLTYPE DrawGlyphRun(void*,FLOAT x,FLOAT y,DWRITE_MEASURING_MODE mode,const DWRITE_GLYPH_RUN* run,const DWRITE_GLYPH_RUN_DESCRIPTION*,IUnknown*)override{
   for(UINT32 i=0;i<run->glyphCount;++i)if(!run->glyphIndices[i])++missing;
   target->DrawGlyphRun({x,y},run,brush,mode);return S_OK;
  }
  HRESULT STDMETHODCALLTYPE DrawUnderline(void*,FLOAT x,FLOAT y,const DWRITE_UNDERLINE* u,IUnknown*)override{target->FillRectangle(D2D1::RectF(x,y+u->offset,x+u->width,y+u->offset+u->thickness),brush);return S_OK;}
  HRESULT STDMETHODCALLTYPE DrawStrikethrough(void*,FLOAT x,FLOAT y,const DWRITE_STRIKETHROUGH* u,IUnknown*)override{target->FillRectangle(D2D1::RectF(x,y+u->offset,x+u->width,y+u->offset+u->thickness),brush);return S_OK;}
  HRESULT STDMETHODCALLTYPE DrawInlineObject(void*,FLOAT,FLOAT,IDWriteInlineObject*,BOOL,BOOL,IUnknown*)override{return E_NOTIMPL;}
 };
 ComPtr<IDWriteFactory> write;ComPtr<ID2D1Factory> d2d;ComPtr<IWICImagingFactory> wic;bool comOwned=false;
public:
 struct Image {Texture texture;std::vector<unsigned char> premultipliedBgra;unsigned width=0,height=0,missingGlyphs=0;std::string key;int frame=0;};
 ComPtr<ID3D11Device> device;std::string error;std::deque<Image> cache;
 ~SubtitleRaster(){cache.clear();wic.Reset();write.Reset();d2d.Reset();if(comOwned)CoUninitialize();}
 static std::wstring wide(const std::string& text){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0);if(n<=0)throw std::runtime_error("Invalid subtitle UTF-8");std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),out.data(),n);return out;}
 void init(){
  if(write)return;
  auto hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);comOwned=SUCCEEDED(hr);if(FAILED(hr)&&hr!=RPC_E_CHANGED_MODE)check(hr,"Subtitle COM");
  check(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),reinterpret_cast<IUnknown**>(write.GetAddressOf())),"Subtitle DirectWrite");
  check(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,d2d.GetAddressOf()),"Subtitle Direct2D");
  check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&wic)),"Subtitle WIC");
 }
 Image* get(const std::string& text,float size,float wrap,const std::string& locale,bool rtl,int frame,bool cpuOnly=false){
  if((!device&&!cpuOnly)||text.empty())return nullptr;
  const std::string key=text+'\x1f'+std::to_string(size)+':'+std::to_string(wrap)+':'+locale+':'+std::to_string(rtl)+':'+std::to_string(cpuOnly);
  for(auto& item:cache)if(item.key==key){item.frame=frame;return &item;}
  try{
   init();const auto input=wide(text),language=wide(locale);
   ComPtr<IDWriteTextFormat> format;check(write->CreateTextFormat(L"Segoe UI",nullptr,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,size,language.c_str(),&format),"Subtitle text format");
   format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
   if(rtl)format->SetReadingDirection(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
   ComPtr<IDWriteTextLayout> layout;check(write->CreateTextLayout(input.data(),UINT32(input.size()),format.Get(),std::max(8.f,wrap),16384,&layout),"Subtitle text layout");
   DWRITE_TEXT_METRICS m{};check(layout->GetMetrics(&m),"Subtitle text metrics");
   const unsigned width=unsigned(std::ceil(std::min(wrap,std::max(1.f,m.width))))+4;
   const unsigned height=unsigned(std::ceil(m.height))+4;
   if(width>16384||height>16384||uint64_t(width)*height>32*1024*1024)throw std::runtime_error("Subtitle bitmap is too large");
   layout->SetMaxWidth(float(width-4));
   // Recompute after tightening the text box: fractional widths can alter wraps.
   DWRITE_TEXT_METRICS adjusted{};layout->GetMetrics(&adjusted);
   const unsigned actualHeight=unsigned(std::ceil(adjusted.height))+4;
   if(actualHeight>16384||uint64_t(width)*actualHeight>32*1024*1024)throw std::runtime_error("Subtitle bitmap is too large");
   ComPtr<IWICBitmap> bitmap;check(wic->CreateBitmap(width,actualHeight,GUID_WICPixelFormat32bppPBGRA,WICBitmapCacheOnLoad,&bitmap),"Subtitle bitmap");
   ComPtr<ID2D1RenderTarget> target;auto props=D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED),96,96);
   check(d2d->CreateWicBitmapRenderTarget(bitmap.Get(),props,&target),"Subtitle raster target");
   target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
   ComPtr<ID2D1SolidColorBrush> white,black;target->CreateSolidColorBrush(D2D1::ColorF(1,1,1,1),&white);target->CreateSolidColorBrush(D2D1::ColorF(0,0,0,1),&black);
   Renderer shadow(target.Get(),black.Get()),glyphs(target.Get(),white.Get());
   target->BeginDraw();target->Clear(D2D1::ColorF(0,0));layout->Draw(nullptr,&shadow,3,3);layout->Draw(nullptr,&glyphs,2,2);check(target->EndDraw(),"Subtitle raster drawing");
   std::vector<unsigned char> pixels(size_t(width)*actualHeight*4);check(bitmap->CopyPixels(nullptr,width*4,UINT(pixels.size()),pixels.data()),"Subtitle pixels");
   if(cpuOnly){
    // Layered HWND consumes premultiplied BGRA directly, at physical pixel size.
    if(cache.size()>=4)cache.pop_front();Image image;image.width=width;image.height=actualHeight;image.key=key;image.frame=frame;image.missingGlyphs=glyphs.missing;image.premultipliedBgra=std::move(pixels);cache.push_back(std::move(image));return &cache.back();
   }
   // WIC premultiplied BGRA -> ImGui straight-alpha RGBA; prevents dark fringes.
   for(size_t i=0;i<pixels.size();i+=4){unsigned a=pixels[i+3];const unsigned b=pixels[i],g=pixels[i+1],r=pixels[i+2];pixels[i]=a?static_cast<unsigned char>(std::min(255u,(r*255+a/2)/a)):0;pixels[i+1]=a?static_cast<unsigned char>(std::min(255u,(g*255+a/2)/a)):0;pixels[i+2]=a?static_cast<unsigned char>(std::min(255u,(b*255+a/2)/a)):0;}
   // Keep textures referenced by this/previous draw lists alive across recording.
   if(cache.size()>=12)for(auto it=cache.begin();it!=cache.end();){if(it->frame+2<frame)it=cache.erase(it);else ++it;}
   Image image;image.width=width;image.height=actualHeight;image.key=key;image.frame=frame;image.missingGlyphs=glyphs.missing;
   D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=actualHeight;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
   D3D11_SUBRESOURCE_DATA initial{pixels.data(),width*4,0};check(device->CreateTexture2D(&desc,&initial,&image.texture.texture),"Subtitle GPU texture");check(device->CreateShaderResourceView(image.texture.texture.Get(),nullptr,&image.texture.view),"Subtitle GPU view");
   error=glyphs.missing?"Windows is missing a font for some subtitle characters. Add supplemental language fonts in Windows Settings.":"";
   cache.push_back(std::move(image));return &cache.back();
  }catch(const std::exception& e){error=e.what();return nullptr;}
 }
};
inline SubtitleRaster subtitleRaster;
