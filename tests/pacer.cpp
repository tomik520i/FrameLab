#include "FramePacer.hpp"
#include <cmath>
#include <stdexcept>
void require(bool ok) { if (!ok) throw std::runtime_error("Pacing regression"); }
int main() {
 FramePacer p; p.setCadence(.025, 2);
 require(p.due(1)); p.presented(1);
 require(!p.due(1.003)); require(p.due(1.01251));
 // GPU completion after a missed deadline must not produce a rapid catch-up.
 p.presented(1.050); require(!p.due(1.051)); require(p.due(1.06251));
 for(int m=2;m<=4;++m) { p.reset(); p.setCadence(1./30,m); p.presented(2);
  require(!p.due(2 + .9 / (30*m))); require(p.due(2 + 1.01 / (30*m))); }
 p.reset(); require(p.due(0));
 p.setCadence(1./30,2);p.presented(0);
 for(int n=1;n<600;++n){double t=n/60.+.0008;require(p.due(t));p.presented(t);}
 require(std::abs(p.next-10.)<.00001); // no accumulated poll-jitter drift
}
