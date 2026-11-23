#pragma once
#include "SubtitleRaster.hpp"
#include "SubtitleLayout.hpp"
#include "SubtitleLanguages.hpp"

// A separate, non-activating layered window: only caption pixels, never a game
// screenshot. Works while the settings HWND is minimized. No injection or hook.
class SubtitleWindowOverlay {
 HWND hwnd_=nullptr,source_=nullptr;DWORD sourcePid_=0;SubtitleRaster raster;
 RECT previous_{};std::string lastText_;SubtitleSettings lastSettings_;bool visible_=false;
 int frame_=0;unsigned updates_=0;
 static LRESULT CALLBACK procedure(HWND h,UINT msg,WPARAM w,LPARAM l){
  if(msg==WM_NCHITTEST)return HTTRANSPARENT;if(msg==WM_MOUSEACTIVATE)return MA_NOACTIVATE;if(msg==WM_ERASEBKGND)return 1;
  return DefWindowProcW(h,msg,w,l);
 }
 void hide(){if(hwnd_&&visible_)ShowWindow(hwnd_,SW_HIDE);visible_=false;}
public:
 static constexpr int hotkey=0xF1B;
 std::string error;
 ~SubtitleWindowOverlay(){stop();}
 bool active()const{return hwnd_!=nullptr;}bool visible()const{return visible_;}HWND handle()const{return hwnd_;}HWND source()const{return source_;}unsigned updates()const{return updates_;}
 bool start(HWND owner,HWND source){
  stop();if(!IsWindow(source)){error="Source window closed.";return false;}
  WNDCLASSEXW wc{sizeof(wc)};wc.lpfnWndProc=procedure;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"FrameLabSubtitleOnly";
  if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS){error="Cannot register subtitle overlay.";return false;}
  hwnd_=CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,wc.lpszClassName,L"FrameLab subtitles",WS_POPUP,0,0,1,1,nullptr,nullptr,wc.hInstance,nullptr);
  if(!hwnd_){error="Cannot create subtitle overlay.";return false;}
  if(!RegisterHotKey(owner,hotkey,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_F11)){DestroyWindow(hwnd_);hwnd_=nullptr;error="Ctrl+Alt+F11 is already used.";return false;}
  SetPropW(hwnd_,L"FrameLabOwner",owner);source_=source;GetWindowThreadProcessId(source_,&sourcePid_);previous_={};lastText_.clear();updates_=0;error.clear();return true;
 }
 void stop(){if(hwnd_){UnregisterHotKey(static_cast<HWND>(GetPropW(hwnd_,L"FrameLabOwner")),hotkey);DestroyWindow(hwnd_);}hwnd_=source_=nullptr;visible_=false;raster.cache.clear();}
 void update(const SubtitleSettings& settings,const SubtitleSnapshot& caption,double time,bool paused=false){
  if(!hwnd_)return;DWORD pid=0;GetWindowThreadProcessId(source_,&pid);
  if(!IsWindow(source_)||pid!=sourcePid_){stop();return;}
  const auto foreground=GetAncestor(GetForegroundWindow(),GA_ROOT);
  if(paused||!settings.enabled||!IsWindowVisible(source_)||IsIconic(source_)||foreground!=GetAncestor(source_,GA_ROOT)||caption.translated.empty()||time>=caption.expiresAt){hide();return;}
  RECT r{};GetClientRect(source_,&r);POINT origin{};ClientToScreen(source_,&origin);
  const int width=r.right,height=r.bottom;if(width<=0||height<=0||uint64_t(width)*height>40*1024*1024){hide();return;}
  std::string text=caption.translated;if(settings.showOriginal)text+='\n'+(caption.displaySource.empty()?caption.source:caption.displaySource);
  if(text!=lastText_||settings!=lastSettings_||width!=previous_.right||height!=previous_.bottom){
   auto language=subtitleLanguage(settings.targetLanguage);
   float fs=std::round(std::max(16.f,height*settings.fontPercent/100));const float px=std::min(width,height)*.009f,py=std::min(width,height)*.006f;
   auto image=raster.get(text,fs,width*.86f,settings.targetLanguage,language&&language->rtl,++frame_,true);
   if(!image){error=raster.error;hide();return;}
   // Reduce unusually tall captions to fit; never scale a low-res bitmap up.
   for(int i=0;i<8&&image->height+2*py>height*.95f;++i){fs*=.8f;image=raster.get(text,fs,width*.86f,settings.targetLanguage,language&&language->rtl,++frame_,true);if(!image){hide();return;}}
   auto box=placeSubtitle(settings,float(width),float(height),image->width+2*px,image->height+2*py);
   BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
   void* memory=nullptr;HDC dc=CreateCompatibleDC(nullptr);HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&memory,nullptr,0);
   if(!dc||!bitmap||!memory){if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);error="Cannot allocate subtitle overlay bitmap.";hide();return;}
   auto old=SelectObject(dc,bitmap);auto pixels=static_cast<unsigned char*>(memory);memset(pixels,0,size_t(width)*height*4);
   auto background=[&](float x,float y,float w,float h,unsigned char alpha){int x0=std::clamp(int(x),0,width),y0=std::clamp(int(y),0,height),x1=std::clamp(int(std::ceil(x+w)),0,width),y1=std::clamp(int(std::ceil(y+h)),0,height);for(int yy=y0;yy<y1;++yy)for(int xx=x0;xx<x1;++xx)pixels[(size_t(yy)*width+xx)*4+3]=std::max(pixels[(size_t(yy)*width+xx)*4+3],alpha);};
   if(settings.coverSource)background(settings.left*width,settings.top*height,(settings.right-settings.left)*width,(settings.bottom-settings.top)*height,255);
   background(box.x,box.y,box.width,box.height,static_cast<unsigned char>(settings.background*255));
   int dx=int(std::round(box.x+px)),dy=int(std::round(box.y+py));
   for(unsigned y=0;y<image->height;++y)for(unsigned x=0;x<image->width;++x){int xx=dx+x,yy=dy+y;if(xx<0||yy<0||xx>=width||yy>=height)continue;
    auto src=image->premultipliedBgra.data()+(size_t(y)*image->width+x)*4;auto dst=pixels+(size_t(yy)*width+xx)*4;
    for(int c=0;c<4;++c)dst[c]=static_cast<unsigned char>(src[c]+(unsigned(dst[c])*(255-src[3])+127)/255);
   }
   POINT zero{};SIZE size{width,height};BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};
   bool ok=UpdateLayeredWindow(hwnd_,nullptr,&origin,&size,dc,&zero,0,&blend,ULW_ALPHA)!=FALSE;
   SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
   if(!ok){error="Cannot update subtitle overlay.";hide();return;}
   lastText_=text;lastSettings_=settings;++updates_;error=image->missingGlyphs?"Install Windows fonts for this subtitle language.":"";
  }
  SetWindowPos(hwnd_,HWND_TOPMOST,origin.x,origin.y,width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW);previous_=r;visible_=true;
 }
};
