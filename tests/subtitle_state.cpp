#include "SubtitleState.hpp"
#include <stdexcept>
void require(bool condition){if(!condition)throw std::runtime_error("Subtitle stability check failed");}
int main(){
 SubtitleState s;require(!s.observe("Follow me."));require(s.observe("Follow me."));require(!s.observe("Follow me."));
 require(!s.observe("The door"));require(s.committed.empty());require(!s.observe("The door is locked."));require(s.observe("The door is locked."));
 require(!s.observe(""));require(s.committed.empty());s.clear();require(!s.observe("Follow me."));
 require(SubtitleState::normalize("  Follow\n me.  ")=="Follow me.");require(SubtitleState::normalize("100 / 200").empty());
}
