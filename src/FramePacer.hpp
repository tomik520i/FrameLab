#pragma once
#include <algorithm>

// Presentation clock, independent of input arrival and GPU completion.
// Late work rebases the clock instead of emitting a catch-up burst.
struct FramePacer {
 double last = -1;
 double next = -1;
 double interval = 1.0 / 60;
 void reset() { last = next = -1; interval = 1.0 / 60; }
 void setCadence(double sourceInterval, int multiplier) {
  interval = std::clamp(sourceInterval / std::max(1, multiplier), 1.0 / 480, .1);
 }
 bool due(double time) const { return next < 0 || time >= next; }
 void presented(double time) {
  // Retain phase for normal polling jitter; rebase genuinely late work.
  next = next < 0 || time-next > interval*.5 ? time+interval : next+interval;
  last = time;
 }
};
