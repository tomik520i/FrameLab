#pragma once
#include <cmath>
#include <cstdio>
#include <string>

// OS-confirmed content IDs and submitted content are distinct measurements.
// Neither is a measurement of physical scanout or end-to-end hardware latency.
inline std::string outputStatsText(bool active,bool osAvailable,float observedFps,float submittedFps,float meanMs,float p95Ms){
 auto number=[](float value){char text[32];if(!std::isfinite(value)||value<0)return std::string("--");std::snprintf(text,sizeof(text),"%.1f",double(value));return std::string(text);};
 const bool confirmed=osAvailable&&std::isfinite(observedFps)&&observedFps>=0;
 const auto fps=active?(confirmed?observedFps:submittedFps):-1.f;
 return std::string(confirmed?"OS ":"OUT ")+number(fps)+" FPS\nAPP "+number(active?meanMs:-1.f)+" ms  |  P95 "+number(active?p95Ms:-1.f);
}
