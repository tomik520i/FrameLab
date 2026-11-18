#include "NrResolution.hpp"
#include <stdexcept>
void check(unsigned w,unsigned h,unsigned limit,unsigned ew,unsigned eh){if(nrDimensions(w,h,limit)!=std::pair{ew,eh})throw std::runtime_error("Wrong NR size");}
int main(){check(960,540,1080,960,536);check(3840,2160,0,3840,2160);check(3440,1440,720,1720,720);check(2560,1600,1200,1920,1200);check(1920,1080,2160,1920,1080);check(0,0,720,0,0);}
