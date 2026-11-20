#include "LocalVideoPlayer.hpp"
#include <Windows.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <cmath>

int wmain(int argc,wchar_t** argv){try{
 if(argc!=3)throw std::runtime_error("Usage: local_video_tracks_test VLC_RUNTIME multi-track.mkv");
 LocalVideoPlayer player(argv[1]);std::atomic<uint64_t> frames=0,hash=0;std::atomic<unsigned> whites=0;std::atomic<bool> bad=false,seekMarker=false;
 auto handler=[&](const DirectShowCapture::Frame& f,uint64_t){
  if(f.width!=320||f.height!=180||f.bottomUp||f.stride<1280){if(!bad.exchange(true))std::cerr<<"Unexpected decoded geometry "<<f.width<<" x "<<f.height<<" stride "<<f.stride<<std::endl;return;}
  auto top=f.data+10*f.stride+10*4,bottom=f.data+170*f.stride+10*4;
  if(top[2]<180||top[0]>60||bottom[0]<180||bottom[2]>60)bad=true;
  auto marker=f.data+10*f.stride+300*4;seekMarker=marker[1]>180&&marker[2]<60;
  unsigned count=0;uint64_t sum=1469598103934665603ull;
  for(unsigned y=100;y<f.height;++y)for(unsigned x=0;x<f.width;++x){auto p=f.data+size_t(y)*f.stride+x*4;if(p[0]>200&&p[1]>200&&p[2]>200)++count;for(int c=0;c<3;++c){sum^=p[c];sum*=1099511628211ull;}}
  whites=count;hash=sum;++frames;
 };player.open(argv[2],L"",0,true,handler);
 auto wait=[&](auto predicate,const char* message){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(35);for(;;){auto s=player.snapshot();if(!s.error.empty())throw std::runtime_error(s.error);if(predicate(s))return s;if(std::chrono::steady_clock::now()>end)throw std::runtime_error(std::string(message)+" backend="+s.backend+" frames="+std::to_string(frames.load())+" audio="+std::to_string(s.audioTracks.size())+" subtitles="+std::to_string(s.subtitleTracks.size())+" opening="+std::to_string(s.opening)+" ended="+std::to_string(s.ended)+" pos="+std::to_string(s.position));Sleep(10);}};
 auto s=wait([&](auto s){return frames>10&&s.audioTracks.size()>=3&&s.subtitleTracks.size()>=3;},"MKV tracks or frames missing");
 if(bad||!s.audio||s.backend.find("LibVLC")!=0||std::abs(s.fps-24)>.05||std::abs(s.duration-8)>.15)throw std::runtime_error("Wrong MKV metadata or pixel format");
 std::vector<int> audio,subs;for(auto& t:s.audioTracks){std::cout<<"Audio "<<t.id<<": "<<t.name<<'\n';if(t.id>=0)audio.push_back(t.id);}for(auto& t:s.subtitleTracks){std::cout<<"Subtitle "<<t.id<<": "<<t.name<<'\n';if(t.id>=0)subs.push_back(t.id);}
 for(auto id:audio){player.selectAudioTrack(id);wait([&](auto v){return v.audioTrack==id;},"Audio track did not switch");}
 std::cout<<"Audio switching passed"<<std::endl;
 player.selectSubtitleTrack(subs[0]);wait([&](auto v){return v.subtitleTrack==subs[0]&&whites>30;},"Selected embedded subtitles were not rendered into video");auto firstHash=hash.load();std::cout<<"First subtitle rendered"<<std::endl;
 player.selectSubtitleTrack(subs[1]);wait([&](auto v){return v.subtitleTrack==subs[1]&&whites>30&&hash!=firstHash;},"Second subtitle track has no changed pixels");
 std::cout<<"Second subtitle rendered"<<std::endl;player.selectSubtitleTrack(-1);wait([&](auto v){return v.subtitleTrack==-1&&whites==0;},"Subtitles Off did not remove pixels");std::cout<<"Subtitles Off passed"<<std::endl;
 player.pause(true);wait([](auto v){return v.paused;},"Pause failed");Sleep(150);auto before=frames.load();Sleep(300);if(frames!=before)throw std::runtime_error("Video advanced while paused");
 player.seek(4.2);wait([&](auto v){return v.paused&&v.position>=4&&v.generation>1&&seekMarker;},"Paused seek did not show the requested frame");
 player.pause(false);wait([](auto v){return v.playing&&v.position>4.3;},"Resume failed");
 player.seek(7.7);wait([](auto v){return v.ended;},"MKV EOF failed");player.loop(true);wait([](auto v){return v.playing&&v.position<2;},"MKV loop failed");
 Sleep(500);s=player.snapshot();if(s.subtitleTrack!=-1||s.audioTrack!=audio.back())throw std::runtime_error("Loop lost selected tracks");
 player.stop();before=frames;Sleep(150);if(frames!=before)throw std::runtime_error("Callback after stop");
 seekMarker=false;player.open(argv[2],L"",0,true,handler);player.selectAudioTrack(audio.back());player.selectSubtitleTrack(-1);player.seek(4.2);player.pause(true);
 wait([&](auto v){return v.paused&&v.position>=4&&seekMarker&&v.audioTrack==audio.back()&&v.subtitleTrack==-1;},"Early seek/pause/track requests were lost while reopening");player.stop();
 std::cout<<"PASS: MKV, native dimensions/cadence, BGRA orientation, 2 audio selections, 2 subtitle tracks verified in pixels, Off, pause, seek, EOF, loop, stop (audio muted)\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
