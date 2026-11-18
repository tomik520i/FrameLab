#pragma once
#include <algorithm>
#include <vector>
#include <utility>

inline std::pair<unsigned,unsigned> nrDimensions(unsigned w,unsigned h,unsigned limit){
 if(!w||!h)return {0,0};
 const double scale=limit?std::min(1.,double(std::max(180u,limit))/h):1.;
 return {std::max(32u,unsigned(w*scale)/8*8),std::max(32u,unsigned(h*scale)/8*8)};
}
inline std::vector<int> nrHeightChoices(unsigned sourceHeight,unsigned monitorHeight,int selected){
 std::vector<int> values{360,480,540,720,900,1080,1200,1440,1600,1800,2160,2880,4320};
 if(monitorHeight>=180&&monitorHeight<=8192)values.push_back(int(monitorHeight));
 if(sourceHeight>=180&&sourceHeight<=8192)values.push_back(int(sourceHeight));
 if(selected>=180&&selected<=8192)values.push_back(selected);
 std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end()),values.end());
 return values;
}
