#include "LosslessRamBuffer.hpp"
#include <iostream>
#include <random>
#include <fstream>
#include <chrono>
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 LosslessRamBuffer ram;const size_t bytes=256*1024;ram.reset(bytes*4,4);
 std::vector<unsigned char> original(bytes,42),result;
 auto repeated=ram.submit(std::vector<unsigned char>(original));require(bool(repeated),"Rejected valid block");ram.waitIdle();
 require(repeated->compressed&&ram.used()<bytes/10,"Repetitive pixels did not compress");
 LosslessRamBuffer::unpack(repeated,result);require(result==original,"Compressed roundtrip changed pixels");ram.release(repeated);require(ram.used()==0,"Leaked accounting");
 std::mt19937 random(173);for(auto& x:original)x=static_cast<unsigned char>(random());
 ram.reset(bytes,2);auto noise=ram.submit(std::vector<unsigned char>(original));ram.waitIdle();
 require(bool(noise)&&!noise->compressed,"Noise should stay verbatim");require(!ram.submit(std::vector<unsigned char>(bytes)),"Budget was not enforced");
 LosslessRamBuffer::unpack(noise,result);require(result==original,"Noise roundtrip changed pixels");ram.release(noise);
 ram.reset(bytes*20,4);std::vector<LosslessRamBuffer::Handle> blocks;
 for(int i=0;i<20;++i)blocks.push_back(ram.submit(std::vector<unsigned char>(bytes,static_cast<unsigned char>(i))));
 ram.waitIdle();for(int i=0;i<20;++i){require(bool(blocks[i]),"Lost queued frame");LosslessRamBuffer::unpack(blocks[i],result);require(result==std::vector<unsigned char>(bytes,static_cast<unsigned char>(i)),"Worker mixed frames");ram.release(blocks[i]);}require(ram.used()==0,"Queue did not drain");
 // Optional real-frame stress input: exact lossless roundtrip, without creating a capture recording.
 if(argc>1){
  constexpr size_t frameBytes=2560ull*1440*4;std::ifstream input(argv[1],std::ios::binary);require(bool(input),"Cannot open stress input");
  ram.reset(2ull*1024*1024*1024,4);blocks.clear();std::vector<std::vector<unsigned char>> originals;
  auto start=std::chrono::steady_clock::now();
  for(int i=0;i<60;++i){std::vector<unsigned char> p(frameBytes);input.read(reinterpret_cast<char*>(p.data()),p.size());require(input.gcount()==p.size(),"Stress input truncated");originals.push_back(p);blocks.push_back(ram.submit(std::move(p)));require(bool(blocks.back()),"Stress queue full");}
  ram.waitIdle();double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  auto stored=ram.used();for(int i=0;i<60;++i){LosslessRamBuffer::unpack(blocks[i],result);require(result==originals[i],"Real pixels changed");ram.release(blocks[i]);}
  std::cout<<"Real 1440p frames: 60, raw bytes="<<frameBytes*60<<", stored bytes="<<stored<<", ratio="<<double(frameBytes*60)/stored<<", read+copy+compress seconds="<<seconds<<"\n";
 }
 std::cout<<"Lossless compression, incompressible fallback, memory limit and concurrent frame ordering passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
