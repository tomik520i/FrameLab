#pragma once
#include "SubtitleTranslator.hpp"
#include "UiTheme.hpp"
#include "Localization.hpp"
#include <cmath>

inline const char* subText(const char* en,const char* cs){return czech?cs:en;}
inline bool subtitlePanel(SubtitleSettings& settings,const SubtitleSnapshot& state){
 const auto before=settings;
 ui::title(subText("LOCAL SUBTITLE TRANSLATION","LOKÁLNÍ PŘEKLAD TITULKŮ"));
 ImGui::TextWrapped("%s",subText("Reads English text already visible in the picture and overlays Czech. Processing stays on this PC.","Čte anglický text už zobrazený v obraze a překrývá ho českým překladem. Zpracování probíhá na tomto PC."));
 ImGui::Checkbox(subText("Translate subtitles to Czech","Překládat titulky do češtiny"),&settings.enabled);
 ImGui::TextDisabled("English > Čeština | lokálně / offline");
 if(settings.enabled&&!state.status.empty())ImGui::TextWrapped("%s",state.status.c_str());
 ImGui::Checkbox(subText("Show reading area","Zobrazit oblast čtení"),&settings.showRegion);
 if(ImGui::CollapsingHeader(subText("Subtitle reading area","Oblast anglických titulků"),ImGuiTreeNodeFlags_DefaultOpen)){
  ImGui::TextWrapped("%s",subText("Select just the subtitle band. Exclude chat, minimap and quest text.","Vyber jen pruh s titulky. Vynech chat, minimapu a úkoly."));
  ImGui::SliderFloat(subText("Left","Zleva"),&settings.left,0,.95f,"%.2f");
  ImGui::SliderFloat(subText("Right","Zprava"),&settings.right,.02f,1,"%.2f");
  ImGui::SliderFloat(subText("Top","Shora"),&settings.top,0,.95f,"%.2f");
  ImGui::SliderFloat(subText("Bottom","Zdola"),&settings.bottom,.02f,1,"%.2f");
  if(ImGui::Button(subText("Bottom subtitles preset","Předvolba: titulky dole"))){settings.left=.1f;settings.right=.9f;settings.top=.72f;settings.bottom=.97f;}
 }
 ui::field(subText("Czech subtitle size","Velikost českých titulků"));ImGui::SliderFloat("##subtitle-size",&settings.fontPercent,2,8,"%.1f %%");
 ImGui::SliderFloat(subText("Vertical position","Svislá poloha"),&settings.position,.1f,.96f,"%.2f");
 ImGui::SliderFloat(subText("Background opacity","Krytí pozadí"),&settings.background,0,1,"%.2f");
 ImGui::Checkbox(subText("Cover original subtitle area","Zakryt oblast původních titulků"),&settings.coverSource);
 ImGui::Checkbox(subText("Include English in overlay","Přidat angličtinu do překryvu"),&settings.showOriginal);
 if(ImGui::CollapsingHeader(subText("Recognition details","Podrobnosti rozpoznání"))){
  ImGui::SliderInt(subText("Read interval (ms)","Interval čtení (ms)"),&settings.intervalMs,200,1500);
  ImGui::TextWrapped("%s",state.status.c_str());ImGui::Text("OCR + MT: %.0f ms",state.milliseconds);
  ImGui::TextWrapped("EN: %s",state.source.c_str());ImGui::TextWrapped("CS: %s",state.translated.c_str());
 }
 ImGui::TextWrapped("%s",subText("Needs two stable readings. Small, moving or stylized text can be misread; translation follows with a delay. Audio is not used.","Čeká na dvě shodná čtení. Malý, pohyblivý nebo stylizovaný text se může přečíst chybně; překlad má zpoždění. Zvuk se nepoužívá."));
 settings.sanitize();return !(settings==before);
}
inline void drawSubtitleOverlay(ImVec2 origin,ImVec2 size,const SubtitleSettings& settings,const SubtitleSnapshot& state,double now,bool cleanOutput=false){
 auto draw=ImGui::GetWindowDrawList();const ImVec2 end{origin.x+size.x,origin.y+size.y};draw->PushClipRect(origin,end,true);
 if(settings.showRegion&&!cleanOutput){ImVec2 a{origin.x+size.x*settings.left,origin.y+size.y*settings.top},b{origin.x+size.x*settings.right,origin.y+size.y*settings.bottom};draw->AddRect(a,b,IM_COL32(170,230,70,255),0,0,2);draw->AddText({a.x+4,a.y+4},IM_COL32(170,230,70,255),"OCR: EN");}
 if(settings.enabled&&!state.translated.empty()&&now-state.observedAt<4.0){
  if(settings.coverSource)draw->AddRectFilled({origin.x+size.x*settings.left,origin.y+size.y*settings.top},{origin.x+size.x*settings.right,origin.y+size.y*settings.bottom},IM_COL32(0,0,0,255));
  std::string text=state.translated;if(settings.showOriginal)text+='\n'+state.source;
  float fs=std::round(std::max(16.f,std::min(size.y,ImGui::GetIO().DisplaySize.y)*settings.fontPercent/100)),wrap=size.x*.86f;
  auto font=ui::subtitleFont(fs);
  auto measured=font->CalcTextSizeA(fs,FLT_MAX,wrap,text.c_str());
  ImVec2 p{origin.x+(size.x-measured.x)/2,origin.y+size.y*settings.position-measured.y/2};
  p.y=std::clamp(p.y,origin.y+8,std::max(origin.y+8,end.y-measured.y-8));
  p.x=std::round(p.x);p.y=std::round(p.y);
  draw->AddRectFilled({p.x-10,p.y-7},{p.x+measured.x+10,p.y+measured.y+7},IM_COL32(0,0,0,int(settings.background*255)),5);
  draw->AddText(font,fs,{p.x+1,p.y+1},IM_COL32(0,0,0,255),text.c_str(),nullptr,wrap);
  draw->AddText(font,fs,p,IM_COL32(255,255,255,255),text.c_str(),nullptr,wrap);
 }
 draw->PopClipRect();
}
