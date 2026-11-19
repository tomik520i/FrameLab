#include "NrHistory.hpp"
#include <deque>
#include <stdexcept>
#include <iostream>
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){
 NrHistory history;
 require(history.needsReset(1,1),"first frame needs initialization");
 history.submitted(1,1,true);
 // Reproduce the FG backlog: three waiting phases, retain the newest. A
 // 50-ms forward jump must not repeatedly reset NR's tone/detail history.
 for(int cycle=1;cycle<=120;++cycle){
  const double t=1+cycle*.05;std::deque<double> queue{t-.033,t-.016,t};
  unsigned recycled=0;auto dropped=trimNrQueue(queue,[&](auto&){++recycled;});
  require(dropped==2&&recycled==2&&queue.size()==1,"bounded queue recycles both skipped frames");
  require(!history.needsReset(1,queue.front()),"ordinary forward drop reset history");
  history.submitted(1,queue.front(),false);
  require(!history.needsReset(1,queue.front()),"FG partner with the same capture timestamp is continuous");
 }
 require(history.frames==121&&history.resets==1,"backlog caused repeated reset pulses");
 require(history.needsReset(2,7.01),"scene cut must reset");
 require(history.needsReset(1,6.9),"rewind must reset");
 require(history.needsReset(1,7.2),"large gap must reset");
 require(history.needsReset(1,7.01,true),"explicit reload must reset");
 std::deque<double> normal{7.01,7.02};require(trimNrQueue(normal,[](auto&){})==0&&normal.size()==2,"normal FG pair retained");
 history.clear();require(history.needsReset(1,8),"source reset must initialize again");
 std::cout<<"PASS: 120 FG backlog cycles preserve NR history; cuts, rewind, reload and long gaps reset\n";
}
