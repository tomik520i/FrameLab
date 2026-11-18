#pragma once
#include <cstdint>

// SetImage starts a new SDK input pair, even if the GPU pointers did not change.
// Keep that pair bound while walking its remaining interpolation phases.
struct VfgPairBinding {
 std::uint64_t revision=~std::uint64_t(0);
 unsigned current=2,multiplier=0;bool cut=false;
 bool needs(std::uint64_t r,unsigned c,bool shot,unsigned m,unsigned index)const{
  return r!=revision||c!=current||shot!=cut||m!=multiplier||index==1;
 }
 void bound(std::uint64_t r,unsigned c,bool shot,unsigned m){revision=r;current=c;cut=shot;multiplier=m;}
};
