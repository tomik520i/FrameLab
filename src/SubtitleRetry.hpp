#pragma once

// Three automatic retries, then wait for explicit user reconfiguration.
// A successful load disarms the timer; a successful translation clears failures.
struct SubtitleRetry {
 unsigned failures=0;double retryAt=-1,readyAt=-1;
 void reset(){*this={};}
 bool pending()const{return retryAt>=0;}
 bool due(double now)const{return pending()&&now>=retryAt;}
 bool exhausted()const{return failures>3;}
 double fail(double now){
  if(readyAt>=0&&now-readyAt>=60)failures=0;
  readyAt=-1;++failures;const double delays[]{2,5,15};
  const double delay=exhausted()?-1:delays[failures-1];retryAt=delay<0?-1:now+delay;return delay;
 }
 void loaded(double now){retryAt=-1;readyAt=now;}
};
