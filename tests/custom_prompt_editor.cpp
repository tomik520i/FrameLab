#include "CustomPromptControls.hpp"
#include "CustomAiSettings.hpp"
#include "imgui_internal.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int wmain(int argc,wchar_t** argv){try{
 require(argc==3,"Usage: custom_prompt_editor_test EXISTING_SETTINGS OUTPUT_DIR");
 const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
 SubtitleSettings legacy;legacy.load(argv[1]);
 require(legacy.customPromptEnabled==!legacy.customInstruction.empty(),"Legacy prompt migration differs");
 auto s=legacy;s.enabled=true;custom_prompt::Draft draft;draft.load(s);draft.example();draft.format=1;
 require(s==([&]{auto x=legacy;x.enabled=true;return x;})(),"Draft changed live settings");
 require(draft.preview(s).find("Czech")!=std::string::npos,"Language replacement missing");
 draft.apply(s);require(s.enabled&&s.customPromptEnabled&&s.customResponseFormat==1,"Applying draft disabled translation");
 s.save(output/L"prompt-settings.txt");SubtitleSettings loaded;loaded.load(output/L"prompt-settings.txt");
 auto expected=s;expected.enabled=false;
 require(loaded==expected,"Prompt settings do not round-trip");
 const auto json=CustomAiSettings::config(loaded);
 require(json.find("translation_tags")!=std::string::npos&&json.find("prompt_enabled")!=std::string::npos,"Worker config is missing prompt settings");
 draft.load(s);draft.enabled=false;draft.apply(s);
 require(!s.customPromptEnabled&&!s.customInstruction.empty(),"Disabling discarded the prompt text");
 draft.defaults();draft.apply(s);require(!s.customPromptEnabled&&s.customInstruction.empty()&&s.customResponseFormat==0,"Reset is not default");
 require(s.customModel==legacy.customModel&&s.customUrl==legacy.customUrl&&s.customMaxTokens==legacy.customMaxTokens&&s.selectedMemory==legacy.selectedMemory&&s.conversationMemory==legacy.conversationMemory,"Unrelated options changed");
 // Exercise the actual modal at both a narrow and large viewport, without a window or screen capture.
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DeltaTime=1.f/60;
 unsigned char* pixels;int fw,fh;io.Fonts->GetTexDataAsRGBA32(&pixels,&fw,&fh);
 for(bool cz:{false,true})for(const auto size:{ImVec2{600,600},ImVec2{1280,900}}){
  czech=cz;io.DisplaySize=size;
  for(int i=0;i<3;++i){ImGui::NewFrame();ImGui::SetNextWindowSize({500,500});ImGui::Begin("Fixture");if(i==0)ImGui::OpenPopup("Translation prompt###custom-prompt-editor");customPromptEditor(s);ImGui::End();ImGui::Render();}
  auto modal=ImGui::FindWindowByName("Translation prompt###custom-prompt-editor");
  require(modal&&modal->Active,"Editor modal was not rendered");
  require(modal->Size.x<=size.x&&modal->Size.y<=size.y,"Editor exceeds viewport");
  require(ImGui::GetDrawData()->TotalVtxCount>0,"No editor rendered");
  ImGui::ClosePopupToLevel(0,true);
 }
 ImGui::DestroyContext();
 std::cout<<"PASS: v14 migration, v15 multiline persistence, opt-in/draft/reset/config, unrelated settings, EN/CZ modal at 600px and 1280px. No screen capture.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
