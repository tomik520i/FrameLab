#pragma once
#include "SubtitleControls.hpp"
#include "SubtitleStyleEvidence.hpp"

inline void subtitleProfileDescription(const SubtitleStyleProfile& p,unsigned sourceHeight){
 ImGui::ColorButton("##profile-colour",{p.rgb[0]/255,p.rgb[1]/255,p.rgb[2]/255,1},ImGuiColorEditFlags_NoTooltip,{30,30});ImGui::SameLine();
 ImGui::Text("RGB %.0f / %.0f / %.0f  #%02X%02X%02X",p.rgb[0],p.rgb[1],p.rgb[2],int(std::round(p.rgb[0])),int(std::round(p.rgb[1])),int(std::round(p.rgb[2])));
 if(sourceHeight)ImGui::Text(subText("Letter height: ~%.1f px at %u px source height","Výška písmen: ~%.1f px při výšce zdroje %u px"),p.height*sourceHeight,sourceHeight);
 ImGui::Text(subText("Height: %.2f%% of image | tolerance +/- %.0f%%","Výška: %.2f %% obrazu | tolerance +/- %.0f %%"),p.height*100,p.heightTolerance*100);
 ImGui::Text(subText("Colour tolerance: %.0f (OpenCV Lab distance)","Tolerance barvy: %.0f (vzdálenost v OpenCV Lab)"),p.colorTolerance);
 if(!p.sampleText.empty())ImGui::TextWrapped(subText("Sample OCR: %s","OCR vzoru: %s"),p.sampleText.c_str());
 else ImGui::TextWrapped("%s",subText("No saved sample text. Older profiles keep their colour/size; select a word again to add evidence.","Chybí uložený text vzoru. Starší profily mají barvu a velikost; pro doplnění vzorku znovu označ slovo."));
 ImGui::TextWrapped("%s",subText("The word illustrates the style, not a required phrase. Font family and point size are not detected.","Slovo slouží jako vzor vzhledu, ne jako povinná fráze. Název fontu ani typografickou velikost nezjišťujeme."));
}

inline void subtitleOcrRows(const std::vector<SubtitleOcrLine>& lines,unsigned sourceHeight){
 const char* en[]={"ACCEPTED","OUTSIDE AREA","WRONG SIZE","WRONG COLOUR"};const char* cs[]={"PŘIJATO","MIMO OBLAST","JINÁ VELIKOST","JINÁ BARVA"};
 int index=0;for(const auto& row:lines){ImGui::PushID(index++);ImGui::Separator();
  ImGui::TextColored(row.reason?ui::orange:ui::accent,"%s",(czech?cs:en)[std::clamp(row.reason,0,3)]);ImGui::SameLine();ImGui::Text("OCR %.2f",row.confidence);ImGui::TextWrapped("%s",row.text.c_str());
  if(row.measured){ImGui::ColorButton("##observed-colour",{row.rgb[0]/255,row.rgb[1]/255,row.rgb[2]/255,1},ImGuiColorEditFlags_NoTooltip,{18,18});ImGui::SameLine();ImGui::Text("RGB %.0f / %.0f / %.0f",row.rgb[0],row.rgb[1],row.rgb[2]);
   if(sourceHeight)ImGui::Text(subText("Letters ~%.1f px | OCR box %.1f px high","Písmena ~%.1f px | výška OCR rámečku %.1f px"),row.height*sourceHeight,row.boxHeight*sourceHeight);
  }else ImGui::TextWrapped("%s",subText("Colour / letter height unavailable for this row.","Barvu / výšku písmen se u tohoto řádku nepodařilo určit."));
  ImGui::PopID();
 }
}

class SubtitleInspector {
 SubtitleStyleEvidence evidence;SubtitleSnapshot frozen;SubtitleStyleProfile frozenStyle;bool freeze=false;uint64_t generation=0;
public:
 bool open=false;
 bool draw(Graphics& g,const std::filesystem::path& root,SubtitleTranslator& worker,SubtitleSettings& settings,unsigned sourceHeight,bool cleanOutput,bool paused=false){
  const bool before=settings.inspectOcr;if(cleanOutput)open=false;
  if(open){auto live=worker.snapshot(paused);if(live.generation!=generation){freeze=false;generation=live.generation;}
   const auto display=ImGui::GetIO().DisplaySize;ImGui::SetNextWindowSize({std::min(720.f,display.x*.92f),std::min(800.f,display.y*.9f)},ImGuiCond_FirstUseEver);
   if(ImGui::Begin(subText("OCR inspector###ocr-inspector","Inspektor OCR###ocr-inspector"),&open)){
    if(ImGui::Checkbox(subText("Freeze these readings","Zmrazit tyto hodnoty"),&freeze)&&freeze){frozen=live;frozenStyle=settings.style;}
    ImGui::SameLine();ImGui::TextColored(freeze?ui::orange:ui::accent,"%s",freeze?subText("FROZEN / video continues","ZMRAZENO / video pokračuje"):"LIVE");
    const auto& state=freeze?frozen:live;const auto& style=freeze?frozenStyle:settings.style;
    ImGui::TextWrapped("%s",subText("Your saved sample / what the filter looks for","Uložený vzor / co filtr hledá"));
    if(style.height>0){if(evidence.load(g,root,style.sampleId))evidence.draw(ImGui::GetContentRegionAvail().x);subtitleProfileDescription(style,state.sourceHeight?state.sourceHeight:sourceHeight);}
    else ImGui::TextWrapped("%s",subText("No learned style. Select one subtitle word first.","Zatím není naučený vzhled. Nejdřív označ jedno slovo titulku."));
    ImGui::TextWrapped("%s",settings.ocrEngine!=0?subText("Windows OCR: appearance measurements are unavailable. Select RapidOCR.","Windows OCR: měření vzhledu není dostupné. Vyber RapidOCR."):style.enabled?subText("Filter enabled: only accepted rows go to translation.","Filtr zapnutý: do překladu jdou jen přijaté řádky."):subText("Filter off: all detected rows in the reading area are eligible.","Filtr vypnutý: lze překládat všechny rozpoznané řádky v oblasti čtení."));
    ImGui::Separator();ImGui::Text(subText("Latest OCR: %d rows | %.1f ms","Poslední OCR: %d řádků | %.1f ms"),int(state.ocrLines.size()),state.ocrMilliseconds);
    if(state.observedAt>0){const double now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();ImGui::Text(subText("Reading age: %.1f s","Stáří čtení: %.1f s"),std::max(0.,now-state.observedAt));}
    if(!settings.enabled)ImGui::TextWrapped("%s",subText("Subtitle reading is off. Enable translation to receive live readings, or use the word-selection preview.","Čtení titulků je vypnuté. Pro živé výsledky zapni překlad nebo použij náhled při označení slova."));
    if(!state.inspected&&settings.ocrEngine==0)ImGui::TextDisabled("%s",subText("Waiting for a detailed reading...","Čekám na podrobné čtení…"));
    subtitleOcrRows(state.ocrLines,state.sourceHeight);
    ImGui::Separator();ImGui::TextWrapped("%s",subText("Estimated from OCR input pixels, before enhancement. OCR score is not a probability of correctness. Opening this view adds bounded CPU analysis for rejected rows. Debug recording also saves measurements when enabled; this view does not start recording.","Odhad z obrazového vstupu OCR před enhancem. OCR skóre není pravděpodobnost správnosti. Otevřený inspektor přidává omezený výpočet na CPU pro odmítnuté řádky. Zapnutý debug ukládá i tyto hodnoty; inspektor sám záznam nespouští."));
   }ImGui::End();
  }else freeze=false;
  settings.inspectOcr=open&&settings.ocrEngine==0;return before!=settings.inspectOcr;
 }
};
