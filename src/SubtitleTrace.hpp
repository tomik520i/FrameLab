#pragma once
#include "SubtitleTranslator.hpp"
#include <fstream>
#include <iomanip>
#include <stdexcept>
// Explicit --subtitle-trace only: no text or video is logged by default.
class SubtitleTrace {
 std::ofstream file;double lastObserved=-1;std::string lastDisplayed;
 static std::string quoted(const std::string& text){
  std::string out="\"";for(unsigned char c:text){switch(c){case '"':out+="\\\"";break;case '\\':out+="\\\\";break;case '\n':out+="\\n";break;case '\r':out+="\\r";break;case '\t':out+="\\t";break;default:if(c>=32)out+=char(c);}}return out+'"';
 }
public:
 void open(const std::filesystem::path& path){file.open(path);if(!file)throw std::runtime_error("Cannot open subtitle trace");}
 void write(double time,const SubtitleSnapshot& s,const std::string& source="en",const std::string& target="cs"){
  if(!file.is_open()||(lastObserved==s.observedAt&&lastDisplayed==s.translated))return;
  file<<std::fixed<<std::setprecision(4)<<"{\"time\":"<<time<<",\"source\":"<<quoted(s.source)<<",\"display_source\":"<<quoted(s.displaySource)<<",\"source_language\":"<<quoted(source)<<",\"target_language\":"<<quoted(target)<<",\"translated\":"<<quoted(s.translated);
  if(target=="cs")file<<",\"cs\":"<<quoted(s.translated); // older Czech report readers
  file<<",\"ocr_ms\":"<<s.ocrMilliseconds<<",\"ocr_mt_ms\":"<<s.milliseconds<<",\"score\":"<<s.confidence<<",\"queue\":"<<s.pending<<"}\n";file.flush();lastObserved=s.observedAt;lastDisplayed=s.translated;
 }
};
