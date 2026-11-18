#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cstdint>

// Offline test source. Only reads original R pixels and their recorded arrival
// times; it never reads generated frames or touches a capture device.
struct RecordedReplay {
 struct Frame { double time; std::vector<unsigned char> pixels; };
 unsigned width=0,height=0;
 double duration=3;
 std::vector<Frame> frames;
 void load(const std::filesystem::path& folder,double from,double length){
  frames.clear();width=height=0;
  if(from<0||length<=0||length>15)throw std::runtime_error("Invalid replay time range");
  std::ifstream csv(folder/L"frames.csv"),raw(folder/L"raw.bgra",std::ios::binary);
  if(!csv||!raw)throw std::runtime_error("Replay needs frames.csv and raw.bgra");
  std::string line;std::getline(csv,line);size_t loaded=0;duration=length;
  while(std::getline(csv,line)){
   if(line.empty()||line[0]!='R')continue;
   std::replace(line.begin(),line.end(),',',' ');std::istringstream row(line);
   char kind;uint64_t id,a,b,offset,bytes;double time,position;unsigned w,h;
   if(!(row>>kind>>id>>a>>b>>time>>position>>w>>h>>offset>>bytes))throw std::runtime_error("Invalid replay manifest row");
   if(time<from||time>=from+length)continue;
   if(!w||!h||w>8192||h>8192||bytes!=uint64_t(w)*h*4||loaded+bytes>8ull*1024*1024*1024)throw std::runtime_error("Replay exceeds image/memory bounds");
   if(!frames.empty()&&(width!=w||height!=h||time-from<frames.back().time))throw std::runtime_error("Replay changes dimensions or time order");
   width=w;height=h;Frame f{time-from,std::vector<unsigned char>(size_t(bytes))};
   raw.seekg(offset);if(!raw.read((char*)f.pixels.data(),bytes))throw std::runtime_error("Replay source truncated");
   loaded+=size_t(bytes);frames.push_back(std::move(f));
  }
  if(frames.size()<2)throw std::runtime_error("Replay contains fewer than two frames");
 }
};
