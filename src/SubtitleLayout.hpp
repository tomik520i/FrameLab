#pragma once
#include "SubtitleTranslator.hpp"
#include <algorithm>

// All placement values are relative to the input image, independent of OCR.
// Measured text includes its background padding; this keeps multiline boxes in view.
struct SubtitleLayout {
 float x=0,y=0,width=0,height=0;
 bool usedOppositeSide=false,clamped=false;
};
inline SubtitleLayout placeSubtitle(const SubtitleSettings& s,float imageWidth,float imageHeight,float boxWidth,float boxHeight){
 SubtitleLayout r;
 if(imageWidth<=0||imageHeight<=0)return r;
 const float margin=std::min(imageWidth,imageHeight)*.008f;
 r.width=std::min(std::max(0.f,boxWidth),imageWidth-2*margin);
 r.height=std::min(std::max(0.f,boxHeight),imageHeight-2*margin);
 r.x=s.horizontal*imageWidth-r.width*(s.horizontalAlign*.5f);
 r.y=s.position*imageHeight-r.height*(s.verticalAlign*.5f);
 if(s.placement!=0){
  const float gap=imageHeight*s.gapPercent/100;
  const float below=s.bottom*imageHeight+gap,above=s.top*imageHeight-gap-r.height;
  r.y=s.placement==1?below:above;
  const float alternate=s.placement==1?above:below;
  if(s.edgeFallback==1&&(r.y<margin||r.y+r.height>imageHeight-margin)&&alternate>=margin&&alternate+r.height<=imageHeight-margin){r.y=alternate;r.usedOppositeSide=true;}
 }
 const float x=std::clamp(r.x,margin,imageWidth-margin-r.width),y=std::clamp(r.y,margin,imageHeight-margin-r.height);
 r.clamped=x!=r.x||y!=r.y;r.x=x;r.y=y;return r;
}
