#pragma once
#include "Enhancer.hpp"
#include "UiTheme.hpp"
#include "Localization.hpp"
#include "NrResolution.hpp"
#include <fstream>
#include <cmath>
#include <shellapi.h>
struct NrControls {
 bool enabled=false,motion=true;float strength=.65f,split=.5f;int view=2,height=720,runtime=0;
 bool operator==(const NrControls&)const=default;
 const char* text(const char* en,const char* cs)const{return czech?cs:en;}
 void load(const std::filesystem::path& path){std::ifstream in(path);int version=0;float s=.65f,x=.5f;int v=2,h=720,r=0,m=1;
  if(in>>version>>s>>x>>v>>h>>r>>m&&version==1&&std::isfinite(s)&&std::isfinite(x)){strength=std::clamp(s,0.f,1.f);split=std::clamp(x,.05f,.95f);view=std::clamp(v,0,2);height=(h==0||(h>=180&&h<=8192))?h:720;runtime=r==1?1:0;motion=m!=0;}}
 void save(const std::filesystem::path& path)const{std::ofstream(path)<<1<<' '<<strength<<' '<<split<<' '<<view<<' '<<height<<' '<<runtime<<' '<<motion<<'\n';}
 bool available(const std::filesystem::path& app)const{std::error_code ec;return std::filesystem::is_regular_file(app/L"runtime"/(runtime?L"nr-original":L"nr-sf")/L"nvngx_dlssnr.dll",ec);}
 bool panel(Enhancer* nr,const std::string& failure,unsigned sourceWidth=0,unsigned sourceHeight=0,unsigned monitorHeight=0,const std::filesystem::path& app={}){
  bool restart=false;
  ui::title(text("DLSS 5 / EXPERIMENT","DLSS 5 / EXPERIMENT"));
  ImGui::TextWrapped("%s",text("AI adjusts the finished image. Compare the result on the same frame.","AI upravuje hotový obraz. Výsledek porovnávej na stejném snímku."));
  const bool installed=available(app);
  if(!installed)ImGui::TextColored(ui::orange,"%s",text("External NR files are not installed for this profile.","Pro tento profil ještě nejsou doplněné externí NR soubory."));
  if(ImGui::Button(text("Set up / check enhancer","Doplnit / zkontrolovat enhancer"))){
   auto setup=app/L"FrameLab-Setup.exe";
   ShellExecuteW(nullptr,L"open",setup.c_str(),nullptr,app.c_str(),SW_SHOWNORMAL);
  }
  ImGui::SameLine();if(ImGui::Button(text("Guide","Návod")))ShellExecuteW(nullptr,L"open",L"https://opicebot.cz/framelab/guide/enhancer.html",nullptr,nullptr,SW_SHOWNORMAL);
  ImGui::BeginDisabled(!installed);ImGui::Checkbox(text("Enable enhancer (F6)","Zapnout enhancer (F6)"),&enabled);ImGui::EndDisabled();
  ui::field(text("Effect strength","Síla efektu"));float percent=strength*100;if(ImGui::SliderFloat("##nr-strength",&percent,0,100,"%.0f %%"))strength=percent/100;
  ImGui::TextDisabled("%s",text("0% = original, 100% = full AI change","0 % = původní obraz, 100 % = plná AI úprava"));
  const char* modes[]={text("Original","Původní"),text("Enhanced","Upravený"),text("Split comparison","Dělené porovnání")};
  ui::field(text("Compare (F7)","Porovnání (F7)"));ImGui::Combo("##nr-view",&view,modes,3);
  if(view==2)ImGui::SliderFloat(text("Divider","Dělicí čára"),&split,.05f,.95f,"%.2f");
  if(ImGui::CollapsingHeader(text("Quality and performance","Kvalita a výkon"))){
  auto label=[&](int value){std::string s=value?std::string(text("Up to ","Až "))+std::to_string(value)+"p":text("Source resolution","Rozlišení zdroje");if(sourceWidth&&sourceHeight){auto [x,y]=nrDimensions(sourceWidth,sourceHeight,unsigned(value));s+="  ("+std::to_string(x)+" x "+std::to_string(y)+")";}return s;};
  ui::field(text("AI processing limit","Limit výpočetního rozlišení AI"));
  if(ImGui::BeginCombo("##nr-height",label(height).c_str())){
   if(ImGui::Selectable(label(0).c_str(),height==0)){height=0;restart=true;}
   for(int value:nrHeightChoices(sourceHeight,monitorHeight,height))if(ImGui::Selectable(label(value).c_str(),height==value)){height=value;restart=true;}
   ImGui::EndCombo();
  }
  ImGui::TextWrapped("%s",text("The source keeps its size and aspect ratio. AI never exceeds it; dimensions align to 8 pixels. Higher limits need more GPU memory. Capture modes are in Input; monitor and image fit are in Display.","Zdroj zachovává rozměry i poměr stran. AI ho nezvětšuje; rozměry zarovnává na 8 pixelů. Vyšší limit potřebuje více paměti GPU. Režimy karty jsou ve Vstupu, monitor a měřítko v Zobrazení."));
  if(ImGui::Checkbox(text("Estimate motion (experimental)","Odhadovat pohyb (experiment)"),&motion))restart=true;
  }
  if(ImGui::CollapsingHeader(text("Runtime / diagnostics","Runtime / diagnostika"))){
   const char* runtimes[]={"ShortFuse NR 310.9.16 (community modified)","NVIDIA NR 310.8 (original / RTX 50)"};
   if(ImGui::Combo("##nr-runtime",&runtime,runtimes,2))restart=true;
   ImGui::BeginDisabled(!installed);if(ImGui::Button(text("Reload enhancer","Znovu načíst enhancer"))){restart=true;enabled=true;}ImGui::EndDisabled();
   ImGui::TextWrapped("%s",text("Runtime support is verified by initialization, not the GPU label. No automatic driver changes.","Podporu ověří až inicializace runtime. Ovladač se automaticky nemění."));
   ImGui::TextWrapped("%s",text("ShortFuse modifies NVIDIA's runtime; Veyra is the integration reference. Redistribution rights for these NR files have not been established. See LICENCE-NR.txt.","ShortFuse upravuje runtime NVIDIA; Veyra je vzor integrace. Právo dále šířit tyto NR soubory nemáme ověřené. Viz LICENCE-NR.txt."));
  }
  if(nr){const auto s=nr->state();if(s==Enhancer::State::Loading)ImGui::TextColored(ui::orange,"%s",text("Loading AI model... preview remains available","Načítání AI modelu... náhled zůstává dostupný"));
   else if(s!=Enhancer::State::Failed){ImGui::Text("AI %u x %u | %.1f ms",nr->inferenceWidth(),nr->inferenceHeight(),nr->processingMs());ImGui::TextDisabled("%s",text("Last completed operation, including GPU wait","Poslední dokončená operace včetně čekání na GPU"));}}
  if(!failure.empty())ImGui::TextWrapped("%s",failure.c_str());
  if(ImGui::CollapsingHeader(text("How it works / limitations","Jak to funguje / omezení"))){
   ImGui::TextWrapped("%s",text("Input frame > smaller AI image > neural rendering > add changes back to source > strength blend. The engine's depth and motion data are unavailable: motion is estimated, depth is flat. Changes can help or harm details and HUD. This is the Veyra community experiment, not an official game integration.","Snímek > menší obraz pro AI > neural rendering > změny zpět do původního rozlišení > přimíchání podle síly. Skutečná data enginu chybí: pohyb se odhaduje, hloubka je plochá. Změny mohou detailům i HUD pomoci nebo uškodit. Jde o komunitní experiment podle Veyry, nikoli oficiální integraci do hry."));
   ImGui::TextWrapped("%s",text("A/B uses exactly the same input frame. With FG on, Original means before enhancement. Analysis recordings contain RAW/FG only.","A/B porovnává přesně stejný snímek. Při FG znamená Původní obraz před enhancerem. Analytický záznam ukládá jen RAW/FG."));
  }
  if(!available(app))enabled=false;
  return restart;
 }
 void draw(ID3D11ShaderResourceView* base,ImVec2 size,Enhancer* nr,bool matched){
  auto p=ImGui::GetCursorScreenPos();ImGui::Image((ImTextureID)(intptr_t)base,size);
  if(!enabled||!matched||!nr||!nr->hasResult)return;
  auto d=ImGui::GetWindowDrawList();ImVec2 end{p.x+size.x,p.y+size.y};
  if(view!=0&&strength>0){if(view==2)d->PushClipRect({p.x+size.x*split,p.y},end,true);d->AddImage((ImTextureID)(intptr_t)nr->result.view.Get(),p,end,{0,0},{1,1},IM_COL32(255,255,255,int(strength*255+.5f)));if(view==2)d->PopClipRect();}
  if(view==2){float x=p.x+size.x*split;d->AddLine({x,p.y},{x,end.y},IM_COL32(193,255,94,255),2);}
  auto label=[&](const char* value,float x){auto s=ImGui::CalcTextSize(value);d->AddRectFilled({x-6,p.y+6},{x+s.x+6,p.y+s.y+16},IM_COL32(0,0,0,185),3);d->AddText({x,p.y+11},IM_COL32(255,255,255,255),value);};
  label(view==1?text("ENHANCED","UPRAVENÝ"):text("ORIGINAL","PŮVODNÍ"),p.x+12);
  if(view==2)label(text("ENHANCED","UPRAVENÝ"),p.x+size.x*split+12);
 }
};
