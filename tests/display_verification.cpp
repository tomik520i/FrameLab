#include <Windows.h>
#include "DisplayVerification.hpp"
#include <iostream>
void require(bool x,const char* text){if(!x)throw std::runtime_error(text);}
int main(){try{
 DisplayVerification v;DXGI_FRAME_STATISTICS f{};
 v.record({1,false,10});f.PresentCount=1;require(bool(v.accept(f))&&v.content==1,"First known ID not counted");
 require(!v.accept(f)&&v.content==1,"Repeated statistics inflated FPS");
 v.record({2,false,10});f.PresentCount=2;require(!v.accept(f)&&v.content==1,"Repeated content inflated FPS");
 v.record({3,true,11});v.record({4,false,12});v.record({100,true,13});f.PresentCount=100;
 require(bool(v.accept(f))&&v.content==2&&v.generated==1,"Skipped present IDs were counted as displayed");
 f.PresentCount=101;require(!v.accept(f)&&v.content==2&&v.unresolved==1,"Unknown present ID inflated FPS");
 v.record({102,false,13});f.PresentCount=102;require(bool(v.accept(f))&&v.content==3,"Raw/generated namespaces collided");
 v.clearHistory();require(!v.available&&!v.haveImage,"Disjoint history retained");
 std::cout<<"Display confirmation deduplication, dropped-ID gaps and unresolved IDs passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
