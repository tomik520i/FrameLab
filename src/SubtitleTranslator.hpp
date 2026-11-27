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
 bool conversationMemory=false; // opt-in source-only history, shared by all LLM providers
 bool useConversationMemory()const{return conversationMemory&&(provider!=0||model!=0);}
 int contentFilter=2; // 0 off, 1 obvious noise/plates, 2 conservative model-assisted EN filter
 bool selectedMemory=false; // same-model selection of earlier original evidence; experimental
 bool useSelectedMemory()const{return useConversationMemory()&&selectedMemory&&(provider!=0||model>=4);}
 int provider=0; // 0 local, 1 OpenAI BYOK, 2 user-configured AI endpoint
 std::string customUrl="http://localhost:8080/v1",customProtocol="openai",customModel,customParameters="{}",customInstruction;
 bool customPromptEnabled=false;
 int customResponseFormat=0; // 0 plain text; 1 a single <translation> block
 int customTimeout=2,customMaxTokens=1000;
 bool customTemperatureEnabled=false;float customTemperature=0;
 unsigned customKeyRevision=0; // credentials remain in the endpoint-specific Windows vault
 std::string contextTrace; // explicit diagnostic CLI only; never persisted
 std::string apiModel="gpt-6-luna",apiEffort="none";
 bool apiTemperatureEnabled=false;
 float apiTemperature=0;
 unsigned apiKeyRevision=0; // in-memory only; changing credentials restarts the worker
 std::string computeBackend="auto",computeDevice; // translation-only GPU selection by stable UUID
 bool allowCpuOffload=false; // explicit permission to split GGUF model across GPU/system RAM
 SubtitleStyleProfile style;
 bool operator==(const SubtitleSettings&)const=default;
 void sanitize();
 void load(const std::filesystem::path&);
 void save(const std::filesystem::path&)const;
};
struct SubtitlePerformance {
 uint64_t filtered=0;
 uint64_t submitted=0,ocrCompleted=0,ocrSuperseded=0,ocrErrors=0,translations=0,cacheHits=0,translationDrops=0,translationErrors=0,shown=0,captionDrops=0;
 double ocrQueueMs=0,translationMs=0,translationQueueMs=0,displayLatencyMs=0,modelLoadMs=0;
 bool ocrBusy=false,translationBusy=false;
};
struct SubtitleSnapshot {std::string source,translated,displaySource,status,filterBackend,computeDescription,contextStatus,contextDetails;float milliseconds=0,ocrMilliseconds=0,confidence=0;double observedAt=0,displayedAt=0,expiresAt=0;uint64_t generation=0;int activeModel=-1,pending=0;bool loading=false;SubtitlePerformance performance;std::vector<SubtitleOcrLine> ocrLines;unsigned sourceHeight=0;bool inspected=false;};
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
