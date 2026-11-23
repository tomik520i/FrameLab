#include "NrHardware.hpp"
#include "Enhancer.hpp"
#include "Vfg.hpp"
#include <iostream>
int main(int argc,char** argv){try{
 Graphics g;g.initDevice(argc>1?std::stoi(argv[1]):-1);
 auto hw=NrHardware::query(g.device.Get(),Vfg::cudaArchitecture(g.device.Get()));
 std::cout<<g.adapterName<<" SM="<<hw.sm<<" community="<<hw.supports(0)<<" original="<<hw.supports(1)<<'\n';
 for(int profile=0;profile<2;++profile){
  auto folder=std::filesystem::temp_directory_path()/L"framelab-absent-hw-fixture";
  Enhancer nr(g,320,180,180,folder,folder/(profile?L"nr-original":L"nr-sf"));
  if(nr.state()!=Enhancer::State::Failed)throw std::runtime_error("Invalid NR initialized");
  auto expected=hw.supports(profile)?std::string("NR runtime missing"):std::string(hw.reason(profile,false));
  if(nr.error().find(expected)==std::string::npos)throw std::runtime_error("Hardware guard was not enforced before runtime load: "+nr.error());
  std::cout<<"profile "<<profile<<": "<<nr.error()<<'\n';
 }
 std::cout<<"PASS hardware eligibility enforced before loading DLLs\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
