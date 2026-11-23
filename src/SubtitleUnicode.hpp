#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <string>
#include <vector>

inline bool subtitleHasLetters(const std::string& text){
 const int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0);
 if(size<=0)return false;
 std::wstring wide(size,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),wide.data(),size);
 std::vector<WORD> types(size);if(!GetStringTypeW(CT_CTYPE1,wide.data(),size,types.data()))return false;
 unsigned latin=0;bool other=false;
 for(int i=0;i<size;++i)if(types[i]&C1_ALPHA){if(wide[i]<128)++latin;else other=true;}
 return latin>=2||other; // Single CJK characters and short non-Latin commands are valid subtitles.
}
