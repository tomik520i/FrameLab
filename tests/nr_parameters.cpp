#include "NrParameters.hpp"
#include <stdexcept>
void require(bool b){if(!b)throw std::runtime_error("NR parameter ABI storage failed");}
int main(){
 nrabi::Parameters p;unsigned w=7;float f=0;void* ptr=nullptr;
 require(p.Get("missing",&w)==nrabi::invalid&&w==7);
 p.Set("width",1920u);require(p.Get("width",&f)==nrabi::success&&f==1920);
 p.Set("width",960);require(p.Get("width",&w)==nrabi::success&&w==960);
 p.Set("pointer",static_cast<void*>(&p));require(p.Get("pointer",&ptr)==nrabi::success&&ptr==&p);
 require(p.Get("pointer",&w)==nrabi::invalid);
 p.Set("null",static_cast<void*>(nullptr));require(p.Get("null",&ptr)==nrabi::success&&ptr==nullptr);
 require(p.Get("width",static_cast<unsigned*>(nullptr))==nrabi::invalid);
 p.Reset();require(p.Get("width",&w)==nrabi::invalid);
}
