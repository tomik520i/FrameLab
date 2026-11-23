#pragma once
#include <string>
#include <cctype>
#include <cstdint>
#include <regex>

// Small, explicit continuation heuristic. History never crosses a source
// generation, a six-second gap, a completed sentence or a new speaker label.
// It is a translation aid, not a claim to infer dialogue or speaker identity.
struct SubtitleSentence {
 struct Prepared{std::string source,request;bool joined=false;};
 std::string previous;double previousTime=0;uint64_t previousGeneration=0;
 void clear(){previous.clear();previousTime=0;previousGeneration=0;}
 static std::string clean(const std::string& text){
  static const std::regex monthSlash(R"(\b(January|February|March|April|May|June|July|August|September|October|November|December)/(?=[0-9]))",std::regex::icase);
  return std::regex_replace(text,monthSlash,"$1 ");
 }
 static bool complete(std::string text){
  while(!text.empty()&&(std::isspace(static_cast<unsigned char>(text.back()))||text.back()=='"'||text.back()=='\''||text.back()==')'||text.back()==']'))text.pop_back();
  if(text.empty()||text.ends_with("...")||text.ends_with("…"))return false;
  return text.back()=='.'||text.back()=='?'||text.back()=='!'||text.ends_with("。")||text.ends_with("！")||text.ends_with("？")||text.ends_with("؟")||text.ends_with("।");
 }
 Prepared prepare(const std::string& raw,double time,uint64_t generation,bool allowJoin=true){
  auto source=clean(raw);const bool lower=!source.empty()&&std::islower(static_cast<unsigned char>(source.front()));
  const bool join=allowJoin&&generation==previousGeneration&&time>=previousTime&&time-previousTime<=6&&previous.size()>=24&&!complete(previous)&&lower&&previous.size()+source.size()<360;
  if(join)source=previous+' '+source;
  previous=source;previousTime=time;previousGeneration=generation;
  // An ellipsis explicitly marks missing continuation to the MT model. It
  // prevents the observed free completion without delaying the first caption.
  return {source,source+(complete(source)?"":" ..."),join};
 }
};
