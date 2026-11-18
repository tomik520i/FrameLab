#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <cstdint>

struct SubtitleSettings {
 bool enabled=false,showRegion=false,showOriginal=false,coverSource=false;
 float left=.10f,top=.72f,right=.90f,bottom=.97f,fontPercent=5.5f,position=.88f,background=.8f;
 int intervalMs=300;
 bool operator==(const SubtitleSettings&)const=default;
 void sanitize();
 void load(const std::filesystem::path&);
 void save(const std::filesystem::path&)const;
};
struct SubtitleSnapshot {std::string source,translated,status;float milliseconds=0;double observedAt=0;uint64_t generation=0;};
class SubtitleTranslator {
public:
 explicit SubtitleTranslator(const std::filesystem::path& app);
 ~SubtitleTranslator();
 void configure(SubtitleSettings settings); // flushes old source and pending jobs
 void reset();
 void submit(const unsigned char* bgra,unsigned w,unsigned h,double captureTime);
 SubtitleSnapshot snapshot()const;
private:
 struct Impl;std::unique_ptr<Impl> impl;
};
