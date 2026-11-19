#include "UiTheme.hpp"
#include <stdexcept>
#include <iostream>
#include <cmath>

int main(){
 ImGui::CreateContext();ui::initialize();
 auto atlas=ImGui::GetIO().Fonts;
 if(!atlas->Build())throw std::runtime_error("Subtitle atlas build failed");
 if(atlas->TexWidth>16384||atlas->TexHeight>16384)throw std::runtime_error("Font atlas exceeds D3D11 texture limits");
 // The selectable subtitle sizes must use sufficient raster detail at both
 // 1080p and 4K; Czech characters must never silently use fallback glyphs.
 for(float outputHeight:{1080.f,2160.f})for(float percent:{2.f,3.5f,5.5f,8.f}){
  float pixels=std::round(outputHeight*percent/100);auto font=ui::subtitleFont(pixels);
  if(font==ui::body||font->FontSize<pixels)throw std::runtime_error("Subtitles enlarge a low-resolution UI font");
  for(ImWchar code:{0x011B,0x0161,0x010D,0x0159,0x017E,0x016F,0x010F,0x0165,0x0148,0x2026})
   if(!font->FindGlyphNoFallback(code))throw std::runtime_error("Missing Czech subtitle glyph");
 }
 std::cout<<"PASS: native subtitle raster sizes for 1080p/4K, Czech glyphs, atlas "<<atlas->TexWidth<<"x"<<atlas->TexHeight<<"\n";
 ImGui::DestroyContext();return 0;
}
