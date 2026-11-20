#pragma once
#include "Graphics.hpp"
#include "SubtitleControls.hpp"
#include "SubtitleInspector.hpp"
#include <array>
#include <vector>

// Freeze one existing SOURCE packet on demand, never the desktop or AI output.
class SubtitleStylePicker {
public:
 bool requested=false;
 bool draw(Graphics& g,SubtitleTranslator& worker,SubtitleSettings& settings,const std::vector<unsigned char>& frame,unsigned width,unsigned height,bool cleanOutput=false,const std::filesystem::path& root={}){
  const char* title=subText("Select subtitle appearance","Vybrat vzhled titulků");
  if(cleanupPending){pixels.clear();texture={};cleanupPending=false;}
  if(cleanOutput){requested=false;if(ImGui::BeginPopupModal(title)){ImGui::CloseCurrentPopup();ImGui::EndPopup();worker.cancelStyleSample();pixels.clear();texture={};}return false;}
  if(requested){
   requested=false;worker.cancelStyleSample();sampleId=0;hasWord=false;dragging=false;areaMode=false;error.clear();
   if(width&&height&&frame.size()==size_t(width)*height*4){pixels=frame;w=width;h=height;texture.create(g.device.Get(),w,h);g.context->UpdateSubresource(texture.texture.Get(),0,nullptr,pixels.data(),w*4,0);area={settings.left,settings.top,settings.right,settings.bottom};}
   else{pixels.clear();error=subText("Start a source or open a video first.","Nejdřív spusť vstup nebo otevři video.");}
   ImGui::OpenPopup(title);
  }
  const auto display=ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowSize({std::min(1200.f,display.x*.94f),std::min(900.f,display.y*.94f)},ImGuiCond_Appearing);
  ImGui::SetNextWindowPos({display.x*.5f,display.y*.5f},ImGuiCond_Appearing,{.5f,.5f});
  bool keepOpen=true,applied=false;
  if(ImGui::BeginPopupModal(title,&keepOpen,ImGuiWindowFlags_NoSavedSettings)){
   ImGui::TextWrapped("%s",subText("Draw around one clear word. It supplies colour and letter size; longer captions and multiple lines still fit inside the reading area.","Obtáhni jedno dobře čitelné slovo. Určuje barvu a velikost písmen; delší titulky i více řádků se dál čtou v celé oblasti."));
   ImGui::TextDisabled("%s",subText("Frozen source frame. Playback and audio continue in the background.","Zmrazený vstupní snímek. Přehrávání a zvuk pokračují na pozadí."));
   if(ImGui::Button(subText("Select word","Vybrat slovo")))areaMode=false;
   ImGui::SameLine();if(ImGui::Button(subText("Adjust reading area","Upravit oblast čtení")))areaMode=true;
   ImGui::SameLine();ImGui::TextColored(ui::accent,"%s",areaMode?subText("Drag the whole subtitle band","Obtáhni celou oblast titulků"):subText("Drag one word","Obtáhni jedno slovo"));
   auto sample=worker.styleSample();
   if(!pixels.empty()){
    const auto avail=ImGui::GetContentRegionAvail();const float scale=std::min(avail.x/w,std::max(80.f,avail.y-360.f)/h);
    const ImVec2 origin=ImGui::GetCursorScreenPos(),size{w*scale,h*scale};
    ImGui::InvisibleButton("##subtitle-style-canvas",size);auto draw=ImGui::GetWindowDrawList();
    draw->AddImage((ImTextureID)texture.view.Get(),origin,{origin.x+size.x,origin.y+size.y});
    auto mouse=[&]{auto p=ImGui::GetIO().MousePos;return ImVec2{std::clamp((p.x-origin.x)/size.x,0.f,1.f),std::clamp((p.y-origin.y)/size.y,0.f,1.f)};};
    if(ImGui::IsItemHovered())ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    if(ImGui::IsItemHovered()&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)){start=mouse();dragging=true;}
    auto rectangle=[&](const std::array<float,4>& r,ImU32 color,float thick){draw->AddRect({origin.x+r[0]*size.x,origin.y+r[1]*size.y},{origin.x+r[2]*size.x,origin.y+r[3]*size.y},color,0,0,thick);};
    rectangle(area,IM_COL32(230,200,70,255),2);
    if(sample.id==sampleId&&!sample.busy)for(const auto& line:sample.lines)rectangle(line.rect,line.reason==0?IM_COL32(150,250,75,255):IM_COL32(255,110,90,220),1.5f);
    if(hasWord)rectangle(word,IM_COL32(60,200,255,255),3);
    if(dragging){
     auto end=mouse();std::array<float,4> selected{std::min(start.x,end.x),std::min(start.y,end.y),std::max(start.x,end.x),std::max(start.y,end.y)};
     rectangle(selected,IM_COL32(60,200,255,255),3);
     if(ImGui::IsMouseReleased(ImGuiMouseButton_Left)){dragging=false;if((selected[2]-selected[0])*w>=6&&(selected[3]-selected[1])*h>=6){if(areaMode){area=selected;areaMode=false;}else{word=selected;hasWord=true;}if(hasWord){sampleId=worker.requestStyleSample(pixels.data(),w,h,word,area);sample=worker.styleSample();error.clear();}}}
    }
   }
   ImGui::TextWrapped("%s",subText("Blue: sample word. Yellow: reading area. Green: accepted text. Red: ignored text.","Modře: vzor slova. Žlutě: oblast čtení. Zeleně: přijatý text. Červeně: ignorovaný text."));
   const bool current=sampleId&&sample.id==sampleId;
   const auto accepted=std::count_if(sample.lines.begin(),sample.lines.end(),[](auto& line){return line.reason==0;});
   if(current&&sample.busy)ImGui::TextWrapped("%s",subText("Checking appearance and matching text...","Zjišťuji vzhled a odpovídající text…"));
   else if(current&&!sample.error.empty())ImGui::TextWrapped("%s",subText("Could not learn this sample. Select a clear word with several letters, including the full letter height.","Tento vzor se nepodařilo určit. Vyber čitelné slovo s několika písmeny a celou výškou písmen."));
   else if(current&&sample.profile.enabled){
    if(hasWord){const float sw=std::max(1.f,(word[2]-word[0])*w),sh=std::max(1.f,(word[3]-word[1])*h),scale=std::min({2.f,ImGui::GetContentRegionAvail().x/sw,60.f/sh});ImGui::Image((ImTextureID)texture.view.Get(),{sw*scale,sh*scale},{word[0],word[1]},{word[2],word[3]});}
    subtitleProfileDescription(sample.profile,h);ImGui::Text(subText("Accepted lines: %d","Přijatých řádků: %d"),int(accepted));if(!accepted)ImGui::TextWrapped("%s",subText("No matching text in the reading area. Adjust the area or select another word.","V oblasti čtení není odpovídající text. Uprav oblast nebo vyber jiné slovo."));}
   else if(sampleId&&!current)ImGui::TextWrapped("%s",subText("The source changed. Select the word again.","Vstup se změnil. Vyber slovo znovu."));
   if(!error.empty())ImGui::TextWrapped("%s",error.c_str());
   if(ImGui::CollapsingHeader(subText("Recognized text / measured values / reasons","Rozpoznané texty / naměřené hodnoty / důvody"))&&current)subtitleOcrRows(sample.lines,h);
   ImGui::BeginDisabled(!current||sample.busy||!sample.error.empty()||!sample.profile.enabled||!accepted);
   if(ImGui::Button(subText("Use this style","Použít tento vzhled"))){
    auto profile=sample.profile;
    if(!root.empty()&&!saveSubtitleStyleEvidence(root,pixels,w,h,word,profile))error=subText("Could not save the sample image. Check free space and write access to the app folder.","Nelze uložit obrázek vzoru. Ověř volné místo a zápis do složky aplikace.");
    else{settings.style=std::move(profile);settings.left=area[0];settings.top=area[1];settings.right=area[2];settings.bottom=area[3];settings.sanitize();applied=true;keepOpen=false;ImGui::CloseCurrentPopup();}}
   ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button(subText("Cancel","Zrušit"))){keepOpen=false;ImGui::CloseCurrentPopup();}
   ImGui::EndPopup();
  }
  // ImGui draw commands still reference the texture until this frame renders.
  if(!keepOpen||(!pixels.empty()&&!ImGui::IsPopupOpen(title))){worker.cancelStyleSample();cleanupPending=true;}
  return applied;
 }
private:
 Texture texture;std::vector<unsigned char> pixels;unsigned w=0,h=0;uint64_t sampleId=0;
 bool hasWord=false,dragging=false,areaMode=false,cleanupPending=false;ImVec2 start{};
 std::array<float,4> word{},area{};std::string error;
};
