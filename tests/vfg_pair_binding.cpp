#include "VfgPairBinding.hpp"
#include <stdexcept>
#include <iostream>
void require(bool v,const char* why){if(!v)throw std::runtime_error(why);}
int main(){try{
 VfgPairBinding b;
 require(b.needs(2,1,false,4,1),"First pair was not bound");b.bound(2,1,false,4);
 require(!b.needs(2,1,false,4,2)&&!b.needs(2,1,false,4,3),"Later phases reset the same pair");
 require(b.needs(4,1,false,4,2),"New pixels in unchanged pointers need rebinding, including a non-first phase");
 b.bound(4,1,false,4);require(b.needs(4,0,false,4,2),"Parity change was ignored");
 require(b.needs(4,1,true,4,2),"Manual shot change was ignored");
 require(b.needs(4,1,false,3,2),"Multiplier change was ignored");
 require(b.needs(4,1,false,4,1),"Restarting a pair must invalidate SDK state");
 std::cout<<"VFG pair lifetime checks passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
