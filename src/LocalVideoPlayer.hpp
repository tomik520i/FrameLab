#pragma once
#include "DirectShowCapture.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

// Windows Media Foundation: clocked video sample sink and audio renderer in
// one media session. Only decoded video enters the existing FG/NR pipeline.
class LocalVideoPlayer {
public:
 struct AudioOutput {std::wstring id;std::string name;};
 struct Snapshot {
  bool opening=false,playing=false,paused=false,ended=false,audio=false,loop=false;
  double position=0,duration=0,fps=0;unsigned width=0,height=0;
  uint64_t frames=0,generation=0;std::string error;
 };
 using Handler=std::function<void(const DirectShowCapture::Frame&,uint64_t)>;
 LocalVideoPlayer();~LocalVideoPlayer();
 void open(const std::filesystem::path&,const std::wstring& endpoint,int volume,bool muted,Handler);
 void stop();void pause(bool);void seek(double seconds);void loop(bool);
 void volume(int percent,bool muted);
 Snapshot snapshot()const;
 static std::vector<AudioOutput> audioOutputs();
private:
 struct Impl;std::unique_ptr<Impl> impl;
};
