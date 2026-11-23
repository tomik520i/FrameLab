#include "SubtitleState.hpp"
#include <stdexcept>
void require(bool condition){if(!condition)throw std::runtime_error("Subtitle stability check failed");}
int main(){
 // Slow GPU results remain displayable only if current OCR confirms them.
 require(SubtitleState::fresh(30,1,"No!",29.8,"No!",2));
 require(!SubtitleState::fresh(30,1,"No!",29.8,"Go!",2));
 require(!SubtitleState::fresh(30,1,"No!",20,"No!",2));
 require(!SubtitleState::fresh(30,1,"No!",29.8,"",2));
 require(!SubtitleState::fresh(30,1,"",29.8,"",2));
 require(SubtitleState::fresh(5,1,"No!",4.8,"Go!",2));
 SubtitleState s;require(!s.observe("Follow me."));require(s.observe("Follow me."));require(!s.observe("Follow me."));
 require(!s.observe("The door"));require(s.committed.empty());require(!s.observe("The door is locked."));require(s.observe("The door is locked."));
 require(!s.observe(""));require(s.committed.empty());s.clear();require(!s.observe("Follow me."));
 require(SubtitleState::normalize("  Follow\n me.  ")=="Follow me.");require(SubtitleState::normalize("100 / 200").empty());
 require(SubtitleState::normalize("No!")=="No!");require(SubtitleState::normalize("Go!")=="Go!");require(SubtitleState::normalize("OK")=="OK");require(SubtitleState::normalize("+").empty());
 s.clear();require(!s.observe("How come you never mentioned she was coming to this class?"));require(s.observe("How come you never mentioned she was coming to thisclass?"));require(!s.observe("How come YOU never mentioned she was coming to this class."));
 require(!SubtitleState::equivalent("I like surprises.","I don't like surprises."));require(!SubtitleState::equivalent("February 26", "February 6"));require(!SubtitleState::equivalent("50%", "50"));
 s.clear();require(s.observe("Follow me.",.99f));require(!s.observe("Follow me",.99f));
 require(!s.observe(""));require(s.observe("Follow me.",.99f));
 require(!SubtitleState::equivalent("1.2", "12"));require(SubtitleState::equivalent("February/26", "February 26."));
 s.clear();require(s.observe("I see you two have met.",.99f));require(!s.observe("see you two have met.",.99f));require(!s.observe("see you two have met.",.99f));require(!s.observe("I see you two have met.",.99f));
 s.clear();require(s.observe("We can finish this tonight.",.99f));require(!s.observe("We can't finish this tonight.",.99f));require(!s.observe("We can't finish this tonight.",.99f));require(s.observe("We can't finish this tonight.",.99f));
}
