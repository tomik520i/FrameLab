#include "LosslessRamBuffer.hpp"
#include <cstring>
// Used by the portable exporter. The archive retains the exact BGRA bytes;
// videos are convenient previews, not substitutes for this source data.
extern "C" __declspec(dllexport) int __cdecl unpack_recording(
 const unsigned char* input,unsigned stored,unsigned raw,unsigned flags,unsigned alpha,unsigned char* output){
 try{
  if(!input||!output||!raw||raw>LZ4_MAX_INPUT_SIZE||stored>raw||flags>3||alpha>255||((flags&2)&&raw%4))return -1;
  const unsigned expected=(flags&2)?raw/4*3:raw;
  if(!(flags&1)&&stored!=expected)return -2;
  auto block=std::make_shared<LosslessRamBuffer::Block>();block->bytes.assign(input,input+stored);block->rawBytes=raw;
  block->compressed=(flags&1)!=0;block->rgbDelta=(flags&2)!=0;block->alpha=static_cast<unsigned char>(alpha);
  std::vector<unsigned char> decoded;LosslessRamBuffer::unpack(block,decoded);memcpy(output,decoded.data(),raw);return 0;
 }catch(...){return -3;}
}
