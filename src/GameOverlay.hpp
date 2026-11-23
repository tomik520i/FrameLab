#pragma once
#include <Windows.h>
#include <string>
// The target keeps input. This HWND only displays the processed client area.
class GameOverlay {
 HWND output_=nullptr,source_=nullptr;LONG_PTR style_=0,ex_=0;WINDOWPLACEMENT placement_{sizeof(WINDOWPLACEMENT)};bool visible_=false;
 RECT last_{};
public:
 static constexpr int hotkey=0xF1A;
 ~GameOverlay(){stop();}
 bool active()const{return output_!=nullptr;}bool visible()const{return visible_;}
 bool start(HWND output,HWND source,std::string& error){
  if(active()||!IsWindow(source)||IsIconic(source)){error="Restore the source window first";return false;}
  if(!RegisterHotKey(output,hotkey,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_F11)){error="Ctrl+Alt+F11 is already used; cannot safely enter overlay mode";return false;}
  output_=output;source_=source;style_=GetWindowLongPtrW(output,GWL_STYLE);ex_=GetWindowLongPtrW(output,GWL_EXSTYLE);GetWindowPlacement(output,&placement_);
  // Layered + transparent passes mouse input to the game. Keep the window
  // activatable explicitly (taskbar/Alt+Tab) so settings remain reachable.
  // Positioning uses SWP_NOACTIVATE and WM_MOUSEACTIVATE returns MA_NOACTIVATE.
  SetWindowLongPtrW(output,GWL_STYLE,WS_POPUP);SetWindowLongPtrW(output,GWL_EXSTYLE,(ex_&~WS_EX_NOACTIVATE)|WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_APPWINDOW);
  SetLayeredWindowAttributes(output,0,255,LWA_ALPHA);SetForegroundWindow(source);last_={};tick();return true;
 }
 bool tick(){
  if(!active())return false;if(!IsWindow(source_)){stop();return false;}
  const auto foreground=GetForegroundWindow();bool show=IsWindowVisible(source_)&&!IsIconic(source_)&&foreground==source_;
  if(show){RECT client{};GetClientRect(source_,&client);POINT p{};ClientToScreen(source_,&p);RECT next{p.x,p.y,p.x+client.right,p.y+client.bottom};
   if(client.right>0&&client.bottom>0&&(!visible_||!EqualRect(&next,&last_))){SetLayeredWindowAttributes(output_,0,255,LWA_ALPHA);SetWindowPos(output_,HWND_TOPMOST,p.x,p.y,client.right,client.bottom,SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_FRAMECHANGED);last_=next;}}
  // Transparent while another app is active, but keep the taskbar/Alt+Tab
  // entry. SW_HIDE removes that entry and leaves only the global hotkey.
  else if(visible_)SetLayeredWindowAttributes(output_,0,0,LWA_ALPHA);visible_=show;return true;
 }
 void stop(){
  if(!output_)return;auto h=output_;output_=nullptr;UnregisterHotKey(h,hotkey);SetWindowLongPtrW(h,GWL_STYLE,style_);SetWindowLongPtrW(h,GWL_EXSTYLE,ex_);SetWindowPos(h,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_FRAMECHANGED);SetWindowPlacement(h,&placement_);ShowWindow(h,SW_SHOW);visible_=false;
 }
};
