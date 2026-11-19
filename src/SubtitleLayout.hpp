#pragma once
#include "SubtitleTranslator.hpp"
#include <algorithm>

// All placement values are relative to the input image, independent of OCR.
// Measured text includes its background padding; this keeps multiline boxes in view.
struct SubtitleLayout {
 float x=0,y=0,width=0,height=0;
 bool usedOppositeSide=false,clamped=false;
};
struct SubtitleBounds {float left=0,top=0,right=-1,bottom=-1;};
inline SubtitleLayout placeSubtitle(const SubtitleSettings& s,float imageWidth,float imageHeight,float boxWidth,float boxHeight,SubtitleBounds visible={}){
 SubtitleLayout r;
 if(imageWidth<=0||imageHeight<=0)return r;
 visible.left=std::clamp(visible.left,0.f,imageWidth);visible.top=std::clamp(visible.top,0.f,imageHeight);
 visible.right=visible.right<0?imageWidth:std::clamp(visible.right,visible.left,imageWidth);
 visible.bottom=visible.bottom<0?imageHeight:std::clamp(visible.bottom,visible.top,imageHeight);
 const float width=visible.right-visible.left,height=visible.bottom-visible.top;
 if(width<=0||height<=0)return r;
 const float margin=std::min(width,height)*.008f;
 r.width=std::min(std::max(0.f,boxWidth),width-2*margin);
 r.height=std::min(std::max(0.f,boxHeight),height-2*margin);
 r.x=s.horizontal*imageWidth-r.width*(s.horizontalAlign*.5f);
 r.y=s.position*imageHeight-r.height*(s.verticalAlign*.5f);
 if(s.placement!=0){
  const float gap=imageHeight*s.gapPercent/100;
  const float below=s.bottom*imageHeight+gap,above=s.top*imageHeight-gap-r.height;
  r.y=s.placement==1?below:above;
  const float alternate=s.placement==1?above:below;
  if(s.edgeFallback==1&&(r.y<visible.top+margin||r.y+r.height>visible.bottom-margin)&&alternate>=visible.top+margin&&alternate+r.height<=visible.bottom-margin){r.y=alternate;r.usedOppositeSide=true;}
 }
 const float x=std::clamp(r.x,visible.left+margin,visible.right-margin-r.width),y=std::clamp(r.y,visible.top+margin,visible.bottom-margin-r.height);
 r.clamped=x!=r.x||y!=r.y;r.x=x;r.y=y;return r;
}
