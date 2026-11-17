#pragma once
#include "../third_party/lz4/lz4.h"
#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

// Background, per-frame lossless compression. No disk writes and no GPU work.
// Call waitIdle before reading a block; release each block after writing it.
class LosslessRamBuffer {
public:
 struct Block {std::vector<unsigned char> bytes;size_t rawBytes=0;bool compressed=false,rgbDelta=false;unsigned char alpha=255;};
 using Handle=std::shared_ptr<Block>;
private:
 mutable std::mutex mutex;
 std::condition_variable ready,idle;
 std::deque<Handle> jobs;
 std::vector<std::jthread> workers;
 size_t budget=0,usedBytes=0,peakBytes=0,pending=0,totalRaw=0;
 bool stopping=true;
 void work(){
  std::vector<char> scratch;std::vector<unsigned char> planar;
  for(;;){
   Handle block;
   {std::unique_lock lock(mutex);ready.wait(lock,[&]{return stopping||!jobs.empty();});
    if(jobs.empty())return;block=std::move(jobs.front());jobs.pop_front();}
   // Allocation/compression failure retains the original pixels, never loses a frame.
   try{
    // Constant alpha is stored once. Planar byte deltas are reversible modulo 256.
    const size_t count=block->rawBytes/4;bool rgb=count>0&&block->rawBytes%4==0;const auto alpha=rgb?block->bytes[3]:0;
    if(rgb)for(size_t i=0;i<count;++i)if(block->bytes[i*4+3]!=alpha){rgb=false;break;}
    const unsigned char* input=block->bytes.data();size_t inputBytes=block->rawBytes;
    if(rgb){planar.resize(count*3);unsigned char previous[3]{};
     for(size_t i=0;i<count;++i)for(size_t c=0;c<3;++c){auto value=block->bytes[i*4+c];planar[c*count+i]=static_cast<unsigned char>(value-previous[c]);previous[c]=value;}
     input=planar.data();inputBytes=planar.size();}
    scratch.resize(LZ4_compressBound(int(inputBytes)));
    int bytes=LZ4_compress_default(reinterpret_cast<const char*>(input),scratch.data(),int(inputBytes),int(scratch.size()));
    bool compressed=bytes>0&&size_t(bytes)<inputBytes;
    if(compressed||rgb){
     std::vector<unsigned char> packed=compressed?std::vector<unsigned char>(scratch.begin(),scratch.begin()+bytes):std::vector<unsigned char>(input,input+inputBytes);
     std::lock_guard lock(mutex);usedBytes-=block->bytes.size()-packed.size();block->bytes.swap(packed);block->compressed=compressed;block->rgbDelta=rgb;block->alpha=alpha;
    }
   }catch(...){/* Store incompressible / allocation-failed blocks verbatim. */}
   {std::lock_guard lock(mutex);--pending;if(!pending)idle.notify_all();}
  }
 }
 void stop(){
  {std::lock_guard lock(mutex);stopping=true;ready.notify_all();}
  workers.clear();
 }
public:
 ~LosslessRamBuffer(){stop();}
 void reset(size_t bytes,unsigned threads=4){
  stop();
  {std::lock_guard lock(mutex);jobs.clear();budget=bytes;usedBytes=peakBytes=pending=totalRaw=0;stopping=false;}
  for(unsigned i=0;i<std::max(1u,threads);++i)workers.emplace_back([this]{work();});
 }
 bool canFit(size_t bytes)const{std::lock_guard lock(mutex);return !stopping&&bytes<=budget&&usedBytes<=budget-bytes;}
 Handle submit(std::vector<unsigned char>&& pixels){
  if(pixels.empty()||pixels.size()>LZ4_MAX_INPUT_SIZE)throw std::runtime_error("Invalid RAM recording block size");
  std::lock_guard lock(mutex);
  if(stopping||pixels.size()>budget||usedBytes>budget-pixels.size())return {};
  auto block=std::make_shared<Block>();block->rawBytes=pixels.size();block->bytes=std::move(pixels);
  jobs.push_back(block);++pending;usedBytes+=block->rawBytes;totalRaw+=block->rawBytes;peakBytes=std::max(peakBytes,usedBytes);ready.notify_one();return block;
 }
 void waitIdle(){std::unique_lock lock(mutex);idle.wait(lock,[&]{return pending==0;});}
 size_t used()const{std::lock_guard lock(mutex);return usedBytes;}
 size_t peak()const{std::lock_guard lock(mutex);return peakBytes;}
 size_t rawTotal()const{std::lock_guard lock(mutex);return totalRaw;}
 static void unpack(const Handle& block,std::vector<unsigned char>& target){
  target.resize(block->rawBytes);
  std::vector<unsigned char> planar;unsigned char* decoded=target.data();size_t decodedBytes=target.size();
  if(block->rgbDelta){planar.resize(block->rawBytes/4*3);decoded=planar.data();decodedBytes=planar.size();}
  if(block->compressed){
   int n=LZ4_decompress_safe(reinterpret_cast<const char*>(block->bytes.data()),reinterpret_cast<char*>(decoded),int(block->bytes.size()),int(decodedBytes));
   if(n!=int(decodedBytes))throw std::runtime_error("Invalid compressed recording block");
  }else std::copy(block->bytes.begin(),block->bytes.end(),decoded);
  if(block->rgbDelta){const size_t count=block->rawBytes/4;unsigned char previous[3]{};
   for(size_t i=0;i<count;++i){for(size_t c=0;c<3;++c){previous[c]=static_cast<unsigned char>(previous[c]+planar[c*count+i]);target[i*4+c]=previous[c];}target[i*4+3]=block->alpha;}}
 }
 void release(Handle& block){if(!block)return;std::lock_guard lock(mutex);usedBytes-=block->bytes.size();block.reset();}
};
