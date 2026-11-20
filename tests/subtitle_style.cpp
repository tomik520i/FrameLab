#include "SubtitleTranslator.hpp"
#include "Diagnostics.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <stdexcept>

double tick(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int wmain(int argc,wchar_t** argv){try{
 require(argc==3,"Usage: subtitle_style_test APP FIXTURE_DIR");const std::filesystem::path app=argv[1],fixtures=argv[2];
 constexpr unsigned w=1280,h=720;std::vector<unsigned char> pixels(size_t(w)*h*4);std::ifstream file(fixtures/L"mixed-styles.bgra",std::ios::binary);require(bool(file.read(reinterpret_cast<char*>(pixels.data()),pixels.size())),"Fixture missing");
 std::array<float,4> word,area{0,.35f,1,.98f};std::ifstream selection(fixtures/L"selection.txt");for(auto& f:word)selection>>f;require(bool(selection),"Selection missing");
 Diagnostics diagnostics;diagnostics.start(app/L"logs","{\"test\":\"ocr_style\"}",false,true,0);
 SubtitleTranslator worker(app);worker.setDiagnostics(&diagnostics);
 auto wait=[&](uint64_t id){const auto deadline=tick()+45;SubtitleStyleSample sample;while(tick()<deadline){sample=worker.styleSample();if(sample.id==id&&!sample.busy){require(sample.error.empty(),sample.error.c_str());return sample;}std::this_thread::sleep_for(std::chrono::milliseconds(20));}throw std::runtime_error("Style sample timed out");};
 auto id=worker.requestStyleSample(pixels.data(),w,h,word,area);auto sample=wait(id);
 const std::string expected="Follow the road to the station. Do not open the red door. Wait for me here.";
 require(sample.profile.enabled&&sample.profile.rgb[0]>220&&sample.profile.height>.02f&&sample.profile.height<.06f,"Wrong word appearance");
 require(sample.profile.sampleText=="Follow"&&sample.profile.sampleFrameHeight==h,"Selected word OCR/height not carried through IPC");
 for(const auto& line:sample.lines){require(line.measured&&line.height>0&&line.boxHeight>0,"Inspector measurement missing");if(line.text.find("DANGER")!=std::string::npos)require(line.reason==3&&line.rgb[0]>200&&line.rgb[1]<100,"Rejected red text lost its real colour");}
 require(sample.text==expected,"Sample preview included a sign/plate or lost a subtitle row");
 unsigned rejected=0;for(const auto& line:sample.lines)if(line.reason)++rejected;require(rejected>=4,"Missing rejected text decisions");
 auto old=worker.requestStyleSample(pixels.data(),w,h,word,area);worker.cancelStyleSample();std::this_thread::sleep_for(std::chrono::milliseconds(50));require(worker.styleSample().id!=old,"Cancelled result survived");
 // Empty background is recoverable; a subsequent valid sample must still work.
 auto bad=worker.requestStyleSample(pixels.data(),w,h,{.01f,.01f,.12f,.05f},area);
 const auto badDeadline=tick()+30;while(worker.styleSample().busy&&tick()<badDeadline)std::this_thread::sleep_for(std::chrono::milliseconds(20));
 require(worker.styleSample().id==bad&&!worker.styleSample().error.empty(),"Blank sample accepted");
 sample=wait(worker.requestStyleSample(pixels.data(),w,h,word,area));
 SubtitleSettings settings;settings.enabled=true;settings.inspectOcr=true;settings.intervalMs=200;settings.left=area[0];settings.top=area[1];settings.right=area[2];settings.bottom=area[3];settings.style=sample.profile;settings.joinFragments=false;
 settings.save(app/L"style-test-settings.txt");SubtitleSettings saved;saved.load(app/L"style-test-settings.txt");require(saved.style==settings.style,"Style persistence lost precision");
 worker.configure(settings);SubtitleSnapshot result;const auto deadline=tick()+60;
 while(tick()<deadline){worker.submit(pixels.data(),w,h,tick());result=worker.snapshot();if(result.source==expected&&!result.translated.empty())break;if(result.status.find("ERROR:")!=std::string::npos)throw std::runtime_error(result.status);std::this_thread::sleep_for(std::chrono::milliseconds(50));}
 require(result.source==expected,"Live cropped pipeline did not preserve three matching lines");require(!result.translated.empty(),"Filtered text was not translated");
 require(result.inspected&&result.sourceHeight==h,"Live inspector lost source height");for(const auto& line:result.ocrLines)require(line.measured&&line.height>0,"Live rejected/accepted measurements missing");
 auto inspectedGeneration=result.generation;settings.inspectOcr=false;worker.configure(settings);require(worker.snapshot().generation==inspectedGeneration,"Closing inspector resets subtitle pipeline");
 std::cout<<"Filtered OCR: "<<result.source<<"\nTranslation: "<<result.translated<<"\n";
 // A valid but absent colour must yield no source, with no all-text fallback.
 settings.style.rgb={20,250,40};worker.configure(settings);
 const auto noMatchDeadline=tick()+15;while(tick()<noMatchDeadline){worker.submit(pixels.data(),w,h,tick());result=worker.snapshot();if(result.observedAt>0)break;std::this_thread::sleep_for(std::chrono::milliseconds(50));}
 require(result.observedAt>0&&result.source.empty()&&result.translated.empty(),"Absent style fell back to unrelated text");
 settings.style.enabled=false;worker.configure(settings);
 const auto autoDeadline=tick()+15;while(tick()<autoDeadline){worker.submit(pixels.data(),w,h,tick());result=worker.snapshot();if(result.source.find("DANGER")!=std::string::npos)break;std::this_thread::sleep_for(std::chrono::milliseconds(50));}
 require(result.source.find("DANGER")!=std::string::npos,"Reset to automatic did not restore unfiltered OCR");
 worker.reset();require(worker.snapshot().source.empty(),"Reset retained an OCR result");
 diagnostics.stop();while(diagnostics.busy())std::this_thread::sleep_for(std::chrono::milliseconds(10));
 std::ifstream events(diagnostics.folder()/L"events.jsonl");std::string log{std::istreambuf_iterator<char>(events),{}};require(log.find("ocr_style_line")!=std::string::npos&&log.find("ocr_style_sample")!=std::string::npos,"Style diagnostic events missing");
 require(log.find("glyph_height_source_px")!=std::string::npos,"Measured values missing in debug logs");
 std::cout<<"PASS: sample IPC, multiline filtering, rejection reasons, cancellation, blank sample recovery, persistence, live ROI translation, no fallback, automatic reset\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
