#include "OutputStats.hpp"
#include <limits>
#include <stdexcept>
#include <iostream>
void require(bool ok){if(!ok)throw std::runtime_error("Output stats mislabeled a measurement");}
int main(){
 const auto observed=outputStatsText(true,true,57.2f,180.f,50.6f,72.3f);
 require(observed=="OS 57.2 FPS\nAPP 50.6 ms  |  P95 72.3");
 require(outputStatsText(true,false,57.f,60.f,-1,-1)=="OUT 60.0 FPS\nAPP -- ms  |  P95 --");
 require(outputStatsText(true,true,-1,59.f,42,60).find("OUT 59.0 FPS")==0);
 require(outputStatsText(false,false,59,60,42,60)=="OUT -- FPS\nAPP -- ms  |  P95 --");
 require(outputStatsText(true,false,0,0,std::numeric_limits<float>::quiet_NaN(),-1)=="OUT 0.0 FPS\nAPP -- ms  |  P95 --");
 std::cout<<"PASS: confirmed vs submitted FPS, stopped/no-sample values, app latency label\n";
}
