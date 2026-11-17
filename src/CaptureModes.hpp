#pragma once
#include "DeviceEnumeration.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

inline bool allowsCaptureRate(const VideoModeInfo& m,double fps){
 if(!std::isfinite(fps)||fps<=0)return false;
 if(m.frameRate>0&&std::abs(m.frameRate-fps)<.005)return true;
 if(m.minFrameInterval<=0||m.maxFrameInterval<m.minFrameInterval)return false;
 double ticks=1e7/fps;
 return ticks>=m.minFrameInterval-1.0&&ticks<=m.maxFrameInterval+1.0;
}
inline std::vector<double> captureRates(const std::vector<VideoModeInfo>& modes,unsigned w,unsigned h){
 std::vector<double> rates;
 auto add=[&](double rate){if(rate<=0)return;for(double r:rates)if(std::abs(r-rate)<.005)return;rates.push_back(rate);};
 const double common[]={24000./1001,24,25,30000./1001,30,48,50,60000./1001,60,90,100,120000./1001,120,144,165,200,240000./1001,240};
 for(auto& m:modes)if(m.width==w&&m.height==h){
  for(double f:common)if(allowsCaptureRate(m,f))add(f);
  add(m.frameRate);
  if(m.minFrameInterval>0&&m.maxFrameInterval>=m.minFrameInterval){add(1e7/m.minFrameInterval);if(1e7/m.maxFrameInterval>=1)add(1e7/m.maxFrameInterval);}
 }
 std::sort(rates.begin(),rates.end());return rates;
}
inline std::string aspectLabel(unsigned w,unsigned h){
 if(!w||!h)return "--";auto divisor=std::gcd(w,h);
 std::string exact=std::to_string(w/divisor)+":"+std::to_string(h/divisor);
 // 3440x1440 is exactly 43:18, commonly marketed as 21:9 ultrawide.
 if(exact=="8:5")exact="16:10";
 if(double(w)/h>2.2)exact+=" ultrawide";
 return exact;
}
