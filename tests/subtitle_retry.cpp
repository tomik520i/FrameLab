#include "SubtitleRetry.hpp"
#include <stdexcept>
#include <iostream>
void require(bool value){if(!value)throw std::runtime_error("Subtitle retry regression");}
int main(){
 SubtitleRetry r;require(!r.pending()&&!r.exhausted());
 require(r.fail(0)==2&&!r.due(1.9)&&r.due(2));
 r.loaded(3);require(!r.pending()&&r.failures==1);
 require(r.fail(4)==5&&r.due(9));r.loaded(10);
 require(r.fail(11)==15&&r.due(26));r.loaded(27);
 require(r.fail(28)==-1&&r.exhausted()&&!r.due(10000));
 r.reset();require(!r.exhausted()&&r.fail(100)==2);
 r.loaded(103);require(r.fail(170)==2); // New fault after healthy idle is a new episode.
 std::cout<<"PASS: 2/5/15-second retry backoff, bounded failures, reset and healthy-idle recovery\n";
}
