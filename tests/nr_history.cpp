#include "NrHistory.hpp"
#include <deque>
#include <stdexcept>
#include <iostream>
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){
 NrHistory history;
 require(history.needsReset(1,1),"first frame needs initialization");
 history.submitted(1,1,true);
 // A 50-ms forward skip must not reset NR tone/detail history.
 for(int cycle=1;cycle<=120;++cycle){
  const double t=1+cycle*.05;std::deque<double> queue{t};
  require(!history.needsReset(1,queue.front()),"ordinary forward drop reset history");
  history.submitted(1,queue.front(),false);
  require(!history.needsReset(1,queue.front()),"FG partner with the same capture timestamp is continuous");
 }
 require(history.frames==121&&history.resets==1,"backlog caused repeated reset pulses");
 require(history.needsReset(2,7.01),"scene cut must reset");
 require(history.needsReset(1,6.9),"rewind must reset");
 require(history.needsReset(1,7.2),"large gap must reset");
 require(history.needsReset(1,7.01,true),"explicit reload must reset");
 struct Frame {double sourceTime;bool generated;};
 auto recycle=[](auto&){};
 // A pending original followed by a fresh G/B pair must survive a short burst.
 std::deque<Frame> normal{{1,false},{1.033,true},{1.033,false}};
 require(trimNrQueue(normal,2,1./30,1.04,recycle)==0&&normal.size()==3,"fresh 2x pair discarded");
 for(int m=2;m<=4;++m){std::deque<Frame> batch{{1,false}};for(int i=1;i<m;++i)batch.push_back({1.033,true});batch.push_back({1.033,false});require(trimNrQueue(batch,m,1./30,1.04,recycle)==0,"normal FG batch discarded");}
 std::deque<Frame> stale{{1,false},{1.033,true},{1.033,false},{1.066,true},{1.066,false}};unsigned recycled=0;
 auto dropped=trimNrQueue(stale,2,1./30,1.080,[&](auto&){++recycled;});require(dropped==3&&recycled==3&&stale.size()==2,"overload not bounded by age and capacity");
 std::deque<Frame> delayed{{1,false},{1.033,true},{1.033,false}};
 require(trimNrQueue(delayed,2,1./30,1.045,recycle)==1&&delayed.size()==2,"brief overload discarded a fresh FG pair instead of just the old frame");
 std::deque<Frame> old{{1,false},{1.1,true},{1.1,false}};require(trimNrQueue(old,2,1./30,1.11,recycle)==1&&old.size()==2,"stale source was not removed");
 std::deque<Frame> tail{{1,true},{1,false}};require(trimNrQueue(tail,2,1./30,1.2,recycle)==0,"last generated/original pair lost without a newer source");
 history.clear();require(history.needsReset(1,8),"source reset must initialize again");
 std::cout<<"PASS: 120 FG backlog cycles preserve NR history; cuts, rewind, reload and long gaps reset\n";
}
