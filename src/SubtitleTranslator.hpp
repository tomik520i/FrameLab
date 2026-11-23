#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <cstdint>
#include <array>
#include <vector>

struct SubtitleStyleProfile {
 bool enabled=false;
 float height=0; // glyph height / full source height, independent of OCR crop
 std::array<float,3> rgb{0,0,0};
 float heightTolerance=.42f,colorTolerance=35.f;
 std::string sampleText;unsigned sampleFrameHeight=0;uint64_t sampleId=0;
 bool operator==(const SubtitleStyleProfile&)const=default;
};
struct SubtitleOcrLine {
 std::array<float,4> rect{};float confidence=0;int reason=0;std::string text;
 bool measured=false;float height=0,boxHeight=0;std::array<float,3> rgb{};
};
struct SubtitleStyleSample {
 uint64_t id=0;bool busy=false;SubtitleStyleProfile profile;
 std::vector<SubtitleOcrLine> lines;std::string text,error;float milliseconds=0;
};

struct SubtitleSettings {
 bool enabled=false,showRegion=false,showOriginal=false,coverSource=false;
 float left=.10f,top=.72f,right=.90f,bottom=.97f,fontPercent=5.5f,position=.88f,background=.8f;
 float horizontal=.5f,gapPercent=1.f;
 int placement=0,horizontalAlign=1,verticalAlign=1,edgeFallback=1;
 bool editOverlay=false; // preview controls only, never persisted or shown in fullscreen
 bool inspectOcr=false; // temporary diagnostics; does not change filtering
 int intervalMs=300;
 int model=0; // 0: CPU, 1/2/3: CUNI Q3/Q8/Q5; 4/5/6: Gemma 4B Q4/4B Q8/12B Q4; stable IDs
 int ocrEngine=0; // 0: bundled RapidOCR / PP-OCRv5; 1: Windows OCR for sourceLanguage
 std::string sourceLanguage="en",targetLanguage="cs";
 float holdSeconds=1.2f,lingerSeconds=3.f;
 bool joinFragments=true;
 SubtitleStyleProfile style;
 bool operator==(const SubtitleSettings&)const=default;
 void sanitize();
 void load(const std::filesystem::path&);
 void save(const std::filesystem::path&)const;
};
struct SubtitlePerformance {
 uint64_t submitted=0,ocrCompleted=0,ocrSuperseded=0,ocrErrors=0,translations=0,cacheHits=0,translationDrops=0,translationErrors=0,shown=0,captionDrops=0;
 double ocrQueueMs=0,translationMs=0,translationQueueMs=0,displayLatencyMs=0,modelLoadMs=0;
 bool ocrBusy=false,translationBusy=false;
};
struct SubtitleSnapshot {std::string source,translated,displaySource,status;float milliseconds=0,ocrMilliseconds=0,confidence=0;double observedAt=0,displayedAt=0,expiresAt=0;uint64_t generation=0;int activeModel=-1,pending=0;bool loading=false;SubtitlePerformance performance;std::vector<SubtitleOcrLine> ocrLines;unsigned sourceHeight=0;bool inspected=false;};
class Diagnostics;
class SubtitleTranslator {
public:
 explicit SubtitleTranslator(const std::filesystem::path& app);
 ~SubtitleTranslator();
 void configure(SubtitleSettings settings); // flushes old source and pending jobs
 void reset();
 void setDiagnostics(Diagnostics* recorder); // recorder must outlive this instance
 void submit(const unsigned char* bgra,unsigned w,unsigned h,double captureTime);
 SubtitleSnapshot snapshot(bool paused=false)const;
 uint64_t requestStyleSample(const unsigned char* bgra,unsigned w,unsigned h,std::array<float,4> word,std::array<float,4> area);
 SubtitleStyleSample styleSample()const;
 void cancelStyleSample();
 static bool rapidOcrAvailable(const std::filesystem::path& app,const std::string& language="en");
private:
 struct Impl;std::unique_ptr<Impl> impl;
};
