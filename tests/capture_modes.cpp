#include "CaptureModes.hpp"
#include <stdexcept>
void require(bool ok){if(!ok)throw std::runtime_error("Capture mode regression");}
int main(){
 VideoModeInfo fixed{3840,2160,60.00024,166666,166666};
 require(allowsCaptureRate(fixed,60));require(!allowsCaptureRate(fixed,120));require(!allowsCaptureRate(fixed,59.94));
 VideoModeInfo variable{1920,1080,60,41666,416667};
 require(allowsCaptureRate(variable,240));require(allowsCaptureRate(variable,120));require(allowsCaptureRate(variable,30000./1001));
 require(!allowsCaptureRate(variable,500));require(!allowsCaptureRate(variable,0));
 auto rates=captureRates({fixed,variable},3840,2160);require(rates.size()==1&&rates[0]==60);
 auto fast=captureRates({fixed,variable},1920,1080);require(std::find(fast.begin(),fast.end(),120)!=fast.end());
 require(aspectLabel(1920,1080)=="16:9");require(aspectLabel(1920,1200)=="16:10");
 require(aspectLabel(640,480)=="4:3");require(aspectLabel(3440,1440)=="43:18 ultrawide");
}
