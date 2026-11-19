#include "SubtitleLayout.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
bool near(float a,float b){return std::abs(a-b)<.01f;}
int main(){try{
 SubtitleSettings s;s.placement=1;s.top=.7f;s.bottom=.8f;
 auto below=placeSubtitle(s,1920,1080,800,100);
 require(near(below.y,874.8f)&&!below.usedOppositeSide,"Below original gap");
 auto large=placeSubtitle(s,3840,2160,1600,200);
 require(near(large.x,below.x*2)&&near(large.y,below.y*2),"Normalized placement at 4K");
 s.bottom=.99f;auto above=placeSubtitle(s,1920,1080,800,100);
 require(above.usedOppositeSide&&above.y+above.height<1080*s.top,"Fallback above near bottom");
 s.edgeFallback=0;auto clamped=placeSubtitle(s,1920,1080,800,100);
 require(!clamped.usedOppositeSide&&clamped.clamped&&clamped.y+clamped.height<1080,"Explicit clamp fallback");
 s.placement=0;s.horizontal=1;s.position=1;s.horizontalAlign=s.verticalAlign=2;
 auto custom=placeSubtitle(s,1080,1920,500,600);
 require(custom.x>=0&&custom.y>=0&&custom.x+custom.width<=1080&&custom.y+custom.height<=1920,"Portrait multiline stays in image");
 auto overflow=placeSubtitle(s,100,50,200,100);
 require(overflow.width<100&&overflow.height<50&&overflow.x>=0&&overflow.y>=0,"Oversized box bounds");
 auto path=std::filesystem::temp_directory_path()/"framelab-subtitle-layout-test.txt";
 s.model=2;s.save(path);SubtitleSettings loaded;loaded.load(path);s.enabled=false;s.showRegion=false;s.editOverlay=false;require(s==loaded,"Placement and model selection roundtrip");
 std::ofstream(path)<<"2 0.1 0.72 0.9 0.97 5.5 0.8 0.8 300 0 1\n";SubtitleSettings old;old.load(path);
 require(old.placement==0&&old.horizontal==.5f&&old.position==.8f&&old.coverSource,"Preview12 migration");
 std::filesystem::remove(path);s.horizontal=std::numeric_limits<float>::quiet_NaN();s.gapPercent=100;s.placement=-5;s.sanitize();require(s.horizontal==.5f&&s.gapPercent==10&&s.placement==0,"Invalid settings sanitized");
 std::cout<<"PASS: placement, multiline edges, 4K/portrait, persistence and migration\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
