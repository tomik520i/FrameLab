#pragma once
#include "imgui.h"
#include <algorithm>
#include <cfloat>
#include <string>
#include <vector>

struct SubtitleTextLine {std::string text;float width=0;};
struct SubtitleText {std::vector<SubtitleTextLine> lines;ImVec2 size{};};
// Share the exact wrapping between measurement and drawing. ImGui's default
// multiline AddText aligns every wrapped line to the left of the whole block.
inline SubtitleText wrapSubtitleText(ImFont* font,float size,float wrap,const std::string& text){
 SubtitleText result;const char* cursor=text.data();const char* end=cursor+text.size();
 while(cursor<end){
  const char* paragraphEnd=std::find(cursor,end,'\n');
  if(cursor==paragraphEnd)result.lines.push_back({{},0});
  while(cursor<paragraphEnd){
   const char* next=wrap>0?font->CalcWordWrapPositionA(size/font->FontSize,cursor,paragraphEnd,wrap):paragraphEnd;
   next=std::min(paragraphEnd,std::max(cursor+1,next));
   // A very narrow wrap may return inside a UTF-8 character. Keep its bytes.
   while(next<paragraphEnd&&(static_cast<unsigned char>(*next)&0xc0)==0x80)++next;
   auto visibleEnd=next;while(visibleEnd>cursor&&(visibleEnd[-1]==' '||visibleEnd[-1]=='\t'||visibleEnd[-1]=='\r'))--visibleEnd;
   SubtitleTextLine line{std::string(cursor,visibleEnd),font->CalcTextSizeA(size,FLT_MAX,0,cursor,visibleEnd).x};
   result.size.x=std::max(result.size.x,line.width);result.lines.push_back(std::move(line));
   cursor=next;while(cursor<paragraphEnd&&(*cursor==' '||*cursor=='\t'||*cursor=='\r'))++cursor;
  }
  cursor=paragraphEnd<end?paragraphEnd+1:end;
 }
 result.size.y=float(result.lines.size())*size;return result;
}
