#pragma once
#include <algorithm>
#include <string>
#include <cctype>

// Two equal OCR readings prevent partial/fading lines from being translated.
// A changed/empty line clears the old caption immediately; source resets clear
// all state so work from a previous video can never become a new subtitle.
struct SubtitleState {
 std::string candidate,committed;unsigned repeats=0;
 void clear(){candidate.clear();committed.clear();repeats=0;}
 static std::string normalize(const std::string& input){
  std::string out;bool space=false;unsigned letters=0;
  for(unsigned char c:input){if(std::isspace(c)){space=!out.empty();continue;}if(space)out+=' ';space=false;out+=char(c);if(std::isalpha(c))++letters;}
  // Keep short commands/negations such as "No!", "Go!" and "OK".
  if(letters<2||out.size()>1500)return {};return out;
 }
 bool observe(const std::string& text){
  if(text!=candidate){candidate=text;repeats=1;committed.clear();return false;}
  ++repeats;if(text.empty()||text==committed)return false;
  if(repeats>=2){committed=text;return true;}return false;
 }
};
