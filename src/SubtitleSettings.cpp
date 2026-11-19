#include "SubtitleTranslator.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>

void SubtitleSettings::sanitize(){
 auto finite=[](float v,float fallback){return std::isfinite(v)?v:fallback;};
 left=std::clamp(finite(left,.1f),0.f,.95f);right=std::clamp(finite(right,.9f),left+.02f,1.f);
 top=std::clamp(finite(top,.72f),0.f,.95f);bottom=std::clamp(finite(bottom,.97f),top+.02f,1.f);
 fontPercent=std::clamp(finite(fontPercent,5.5f),2.f,8.f);position=std::clamp(finite(position,.88f),0.f,1.f);background=std::clamp(finite(background,.8f),0.f,1.f);
 horizontal=std::clamp(finite(horizontal,.5f),0.f,1.f);gapPercent=std::clamp(finite(gapPercent,1.f),0.f,10.f);
 placement=std::clamp(placement,0,2);horizontalAlign=std::clamp(horizontalAlign,0,2);verticalAlign=std::clamp(verticalAlign,0,2);edgeFallback=std::clamp(edgeFallback,0,1);
 intervalMs=std::clamp(intervalMs,200,1500);
 model=std::clamp(model,0,2);
}
void SubtitleSettings::load(const std::filesystem::path& p){
 int version=0;std::ifstream f(p);
 if(f>>version&&version>=1&&version<=4){
  f>>left>>top>>right>>bottom>>fontPercent>>position>>background>>intervalMs>>showOriginal;
  if(version>=2)f>>coverSource;
  if(version>=3)f>>horizontal>>gapPercent>>placement>>horizontalAlign>>verticalAlign>>edgeFallback;
  if(version>=4)f>>model;
 }
 sanitize();enabled=false;showRegion=false;editOverlay=false;
}
void SubtitleSettings::save(const std::filesystem::path& p)const{
 std::ofstream(p)<<4<<' '<<left<<' '<<top<<' '<<right<<' '<<bottom<<' '<<fontPercent<<' '<<position<<' '<<background<<' '<<intervalMs<<' '<<showOriginal<<' '<<coverSource<<' '<<horizontal<<' '<<gapPercent<<' '<<placement<<' '<<horizontalAlign<<' '<<verticalAlign<<' '<<edgeFallback<<' '<<model<<'\n';
}
