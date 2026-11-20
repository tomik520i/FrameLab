#include "SubtitleTranslator.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>

void SubtitleSettings::sanitize(){
 auto finite=[](float v,float fallback){return std::isfinite(v)?v:fallback;};
 left=std::clamp(finite(left,.1f),0.f,.95f);right=std::clamp(finite(right,.9f),left+.02f,1.f);
 top=std::clamp(finite(top,.72f),0.f,.95f);bottom=std::clamp(finite(bottom,.97f),top+.02f,1.f);
 fontPercent=std::clamp(finite(fontPercent,5.5f),2.f,8.f);position=std::clamp(finite(position,.88f),0.f,1.f);background=std::clamp(finite(background,.8f),0.f,1.f);
 horizontal=std::clamp(finite(horizontal,.5f),0.f,1.f);gapPercent=std::clamp(finite(gapPercent,1.f),0.f,10.f);
 placement=std::clamp(placement,0,2);horizontalAlign=std::clamp(horizontalAlign,0,2);verticalAlign=std::clamp(verticalAlign,0,2);edgeFallback=std::clamp(edgeFallback,0,1);
 intervalMs=std::clamp(intervalMs,200,1500);
 model=std::clamp(model,0,3);
 ocrEngine=std::clamp(ocrEngine,0,1);
 holdSeconds=std::clamp(finite(holdSeconds,1.2f),.5f,4.f);
 lingerSeconds=std::clamp(finite(lingerSeconds,3.f),holdSeconds,8.f);
 if(!std::isfinite(style.height)||style.height<.001f||style.height>.25f){style.enabled=false;style.height=0;}
 for(auto& c:style.rgb)c=std::clamp(finite(c,0),0.f,255.f);
 style.heightTolerance=std::clamp(finite(style.heightTolerance,.42f),.15f,.8f);
 style.colorTolerance=std::clamp(finite(style.colorTolerance,35.f),10.f,65.f);
}
void SubtitleSettings::load(const std::filesystem::path& p){
 int version=0;std::ifstream f(p);
 if(f>>version&&version>=1&&version<=7){
  f>>left>>top>>right>>bottom>>fontPercent>>position>>background>>intervalMs>>showOriginal;
  if(version>=2)f>>coverSource;
  if(version>=3)f>>horizontal>>gapPercent>>placement>>horizontalAlign>>verticalAlign>>edgeFallback;
  if(version>=4)f>>model;
  if(version>=5)f>>ocrEngine>>holdSeconds>>lingerSeconds;
  if(version>=6)f>>joinFragments;
  if(version>=7){SubtitleStyleProfile candidate;if(f>>candidate.enabled>>candidate.height>>candidate.rgb[0]>>candidate.rgb[1]>>candidate.rgb[2]>>candidate.heightTolerance>>candidate.colorTolerance)style=candidate;}
 }
 sanitize();enabled=false;showRegion=false;editOverlay=false;
}
void SubtitleSettings::save(const std::filesystem::path& p)const{
 std::ofstream(p)<<std::setprecision(std::numeric_limits<float>::max_digits10)<<7<<' '<<left<<' '<<top<<' '<<right<<' '<<bottom<<' '<<fontPercent<<' '<<position<<' '<<background<<' '<<intervalMs<<' '<<showOriginal<<' '<<coverSource<<' '<<horizontal<<' '<<gapPercent<<' '<<placement<<' '<<horizontalAlign<<' '<<verticalAlign<<' '<<edgeFallback<<' '<<model<<' '<<ocrEngine<<' '<<holdSeconds<<' '<<lingerSeconds<<' '<<joinFragments<<' '<<style.enabled<<' '<<style.height<<' '<<style.rgb[0]<<' '<<style.rgb[1]<<' '<<style.rgb[2]<<' '<<style.heightTolerance<<' '<<style.colorTolerance<<'\n';
}
