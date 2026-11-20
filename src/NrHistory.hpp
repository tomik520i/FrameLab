#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <cstddef>

// Skipping a few queued frames is not a scene cut. The next motion estimate
// compares against the last frame actually processed, so preserve that history
// across short forward skips; invalidate only at a real discontinuity.
struct NrHistory {
 bool valid=false;
 uint64_t scene=0,frames=0,resets=0;
 double time=0;
 void clear(){*this={};}
 bool needsReset(uint64_t nextScene,double nextTime,bool explicitReset=false)const{
  return explicitReset||!valid||nextScene!=scene||!std::isfinite(nextTime)||nextTime<time||nextTime-time>.15;
 }
 void submitted(uint64_t nextScene,double nextTime,bool reset){
  scene=nextScene;time=nextTime;valid=std::isfinite(nextTime);++frames;if(reset)++resets;
 }
};

// One preceding output + a freshly completed FG batch is normal, not overload.
// Bound both queue capacity and source age; do not discard a fresh 2x pair just
// because it briefly takes the queue from one to three entries.
template<class Queue,class Recycle>
unsigned trimNrQueue(Queue& queue,int multiplier,double sourceInterval,double now,Recycle recycle){
 unsigned dropped=0;
 const auto capacity=std::size_t(std::clamp(multiplier,1,4)+1);
 // A whole extra input interval retained forever is visible input lag. Allow
 // arrival jitter, but start shedding old sources before two intervals accrue.
 const double maxAge=std::clamp(sourceInterval*1.25,.010,.075);
 while(queue.size()>1){
  const bool stale=now-queue.front().sourceTime>maxAge&&queue.back().sourceTime>queue.front().sourceTime+.0001;
  if(queue.size()<=capacity&&!stale)break;
  recycle(queue.front());queue.pop_front();++dropped;
 }
 return dropped;
}
