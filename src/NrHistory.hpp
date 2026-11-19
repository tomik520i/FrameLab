#pragma once
#include <cstdint>
#include <cmath>

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

// The caller can recycle discarded GPU textures. Pruning never invalidates
// temporal history by itself: continuity is checked on the surviving frame.
template<class Queue,class Recycle>
unsigned trimNrQueue(Queue& queue,Recycle recycle){
 unsigned dropped=0;
 if(queue.size()>2)while(queue.size()>1){recycle(queue.front());queue.pop_front();++dropped;}
 return dropped;
}
