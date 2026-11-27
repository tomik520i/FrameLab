#pragma once
#include <algorithm>
#include <string>
#include <cctype>
#include "SubtitleUnicode.hpp"

// A confident neural reading can start translation immediately. Otherwise two
// equivalent readings are required. Ignore typography, never changed letters,
// numbers or negations. Presentation lifetime is independent of OCR lifetime.
struct SubtitleState {
 std::string candidate,committed,lastAccepted;unsigned repeats=0;
 void clear(){candidate.clear();committed.clear();lastAccepted.clear();repeats=0;}
 static std::string key(const std::string& text){
  std::string out;for(size_t i=0;i<text.size();++i){unsigned char c=text[i];if(c=='\n'){out+='\n';continue;}if(std::isspace(c))continue;
   const bool numericSeparator=i>0&&i+1<text.size()&&std::isdigit(static_cast<unsigned char>(text[i-1]))&&std::isdigit(static_cast<unsigned char>(text[i+1]));
   if(c=='!'||c=='?'||c=='\''||c=='"'||c==';'||((c=='.'||c==','||c==':'||c=='/')&&!numericSeparator))continue;out+=char(std::tolower(c));}return out;
 }
 static bool equivalent(const std::string& a,const std::string& b){return key(a)==key(b);}
 // A slow local model may finish after the queue's normal six-second limit.
 // Keep that result only while a fresh OCR reading confirms the same text.
 static bool fresh(double now,double captured,const std::string& source,double observed,const std::string& current,double observationWindow){
  return now-captured<=6||(!source.empty()&&now-observed<=observationWindow&&equivalent(source,current));
 }
 static bool oneEdit(const std::string& a,const std::string& b){
  if(std::min(a.size(),b.size())<15||std::max(a.size(),b.size())-std::min(a.size(),b.size())>1)return false;
  size_t i=0,j=0;unsigned edits=0;while(i<a.size()&&j<b.size()){
   if(a[i]==b[j]){++i;++j;continue;}if(++edits>1)return false;if(a.size()>=b.size())++i;if(b.size()>=a.size())++j;
  }return edits+(i<a.size()||j<b.size())<=1;
 }
 static std::string normalize(const std::string& input,bool preserveLines=false){
  std::string out;bool space=false,line=false;
  for(unsigned char c:input){if(std::isspace(c)){space=!out.empty();line|=preserveLines&&(c=='\n'||c=='\r');continue;}if(space)out+=line?'\n':' ';space=line=false;out+=char(c);}
  // Keep short commands/negations such as "No!", "Go!" and "OK".
  if(out.size()>1500||!subtitleHasLetters(out))return {};return out;
 }
 bool observe(const std::string& text,float confidence=0){
  if(text.empty()){clear();return false;}
  if(equivalent(text,lastAccepted)){candidate=text;committed=lastAccepted;repeats=0;return false;}
  if(!equivalent(text,candidate)){candidate=text;repeats=1;committed.clear();}
  else ++repeats;
  if(text.empty()||equivalent(text,committed))return false;
  // A one-letter glitch must persist for three readings before it becomes a
  // correction. Do not merge text: genuine changes, including negation, win
  // once confirmed. Returning to the accepted line does not requeue it.
  const bool correction=oneEdit(key(text),key(lastAccepted));
  if(repeats>=(correction?3u:2u)||(!correction&&confidence>=.97f)){committed=lastAccepted=text;return true;}return false;
 }
};
