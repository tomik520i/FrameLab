#pragma once
// FrameLab's parameter storage for the public NGX parameter ABI. GPL-3.0.
// Functional ABI reference: NVIDIA/DLSS 9981c635, include/nvsdk_ngx_params.h.
// No NVIDIA SDK implementation or static library is included or linked.
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <type_traits>
struct ID3D11Resource;
struct ID3D12Resource;
namespace nrabi {
using Result=std::uint32_t;
inline constexpr Result success=1, invalid=0xbad00005, platformError=0xbad00002;
struct Handle;
class Parameters {
 using Value=std::variant<unsigned long long,float,double,unsigned,int,void*>;
 std::unordered_map<std::string,Value> values;
 template<class T> void put(const char* name,T value){if(name)values.insert_or_assign(name,value);}
 template<class T> Result get(const char* name,T* out)const{
  if(!name||!out)return invalid;
  auto i=values.find(name);if(i==values.end())return invalid;
  return std::visit([&](const auto& value)->Result{
   using V=std::decay_t<decltype(value)>;
   if constexpr(std::is_arithmetic_v<T>&&std::is_arithmetic_v<V>){*out=static_cast<T>(value);return success;}
   else if constexpr(std::is_pointer_v<T>&&std::is_pointer_v<V>){*out=static_cast<T>(value);return success;}
   else return invalid;
  },i->second);
 }
public:
 // Virtual slot order is an external ABI. Do not add a virtual destructor.
 virtual void Set(const char* n,unsigned long long v){put(n,v);}
 virtual void Set(const char* n,float v){put(n,v);}
 virtual void Set(const char* n,double v){put(n,v);}
 virtual void Set(const char* n,unsigned v){put(n,v);}
 virtual void Set(const char* n,int v){put(n,v);}
 virtual void Set(const char* n,ID3D11Resource* v){put(n,static_cast<void*>(v));}
 virtual void Set(const char* n,ID3D12Resource* v){put(n,static_cast<void*>(v));}
 virtual void Set(const char* n,void* v){put(n,v);}
 virtual Result Get(const char* n,unsigned long long* v)const{return get(n,v);}
 virtual Result Get(const char* n,float* v)const{return get(n,v);}
 virtual Result Get(const char* n,double* v)const{return get(n,v);}
 virtual Result Get(const char* n,unsigned* v)const{return get(n,v);}
 virtual Result Get(const char* n,int* v)const{return get(n,v);}
 virtual Result Get(const char* n,ID3D11Resource** v)const{return get(n,v);}
 virtual Result Get(const char* n,ID3D12Resource** v)const{return get(n,v);}
 virtual Result Get(const char* n,void** v)const{return get(n,v);}
 virtual void Reset(){values.clear();}
};
}
