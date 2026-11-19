#include "SubtitleSentence.hpp"
#include <stdexcept>
void require(bool ok){if(!ok)throw std::runtime_error("Subtitle sentence check failed");}
int main(){
 SubtitleSentence s;
 auto first=s.prepare("NARRATOR: Not since we moved to this city",10,1);
 require(!first.joined&&first.request.ends_with(" ..."));
 auto second=s.prepare("have I been this happy.",14,1);
 require(second.joined&&second.source=="NARRATOR: Not since we moved to this city have I been this happy."&&second.request==second.source);
 require(!s.prepare("this is a new sentence.",15,1).joined);
 s.clear();s.prepare("Because the streets were too crowded",10,1);
 require(!s.prepare("we turned back.",17,1).joined);
 s.clear();s.prepare("Because the streets were too crowded",10,1);
 require(!s.prepare("we turned back.",12,2).joined);
 s.clear();s.prepare("Because the streets were too crowded",10,1);
 require(!s.prepare("PERSON: We turned back.",12,1).joined);
 s.clear();s.prepare("Because the streets were too crowded",10,1);
 require(!s.prepare("we turned back.",12,1,false).joined);
 require(SubtitleSentence::clean("February/26.")=="February 26.");
 require(SubtitleSentence::clean("Route A/26 and 26/2")=="Route A/26 and 26/2");
 require(SubtitleSentence::complete("Don't open the door!\"")&&!SubtitleSentence::complete("Don't open..."));
}
