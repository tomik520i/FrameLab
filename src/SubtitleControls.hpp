#pragma once
#include "SubtitleTranslator.hpp"
#include "SubtitleLayout.hpp"
#include "SubtitleModels.hpp"
#include "OcrSetup.hpp"
#include "UiTheme.hpp"
#include "Localization.hpp"
#include <cmath>

inline const char* subText(const char* en,const char* cs){return czech?cs:en;}
inline bool subtitlePanel(SubtitleSettings& settings,const SubtitleSnapshot& state,SubtitleModels& models,float freeVramMB,OcrSetup& ocr,HWND window,bool rapidReady){
 const auto before=settings;
 ui::title(subText("LOCAL SUBTITLE TRANSLATION","LOKÁLNÍ PŘEKLAD TITULKŮ"));
 ImGui::TextWrapped("%s",subText("Reads English text already visible in the picture and overlays Czech. Processing stays on this PC.","Čte anglický text už zobrazený v obraze a překrývá ho českým překladem. Zpracování probíhá na tomto PC."));
 const auto capability=ocr.snapshot();
 ui::field(subText("Text recognition","Rozpoznávání textu"));
 ImGui::Combo("##ocr-engine",&settings.ocrEngine,"RapidOCR / PP-OCRv5 English\000Windows English OCR\000");
 if(settings.ocrEngine==0){
  ImGui::TextWrapped("%s",rapidReady?subText("Neural OCR bundled and ready. Offline; Windows OCR installation is unnecessary.","Neuronové OCR je přibalené a připravené. Offline; instalace OCR ve Windows není potřeba."):subText("RapidOCR files missing. Extract the complete package, or choose Windows OCR.","Chybí soubory RapidOCR. Rozbal kompletní balíček, nebo vyber OCR ve Windows."));
 }else{
 if(capability.available==1)ImGui::TextColored(ui::accent,"%s",subText("Windows English OCR is ready","Anglické OCR ve Windows je připravené"));
 else if(capability.installing)ImGui::TextWrapped("%s",subText("Installing English OCR through Windows. Confirm UAC if prompted; downloading and servicing can take several minutes. The picture keeps running.","Windows instalují anglické OCR. Potvrď případné UAC; stažení a instalace mohou trvat několik minut. Obraz běží dál."));
 else if(capability.available<0)ImGui::TextDisabled("%s",subText("Checking Windows English OCR...","Kontroluji anglické OCR ve Windows..."));
 else{
  ImGui::TextWrapped("%s",subText("English OCR is a Windows component. Install it once from Microsoft; administrator approval and Internet are required. Your Windows display language stays unchanged. Recognition then works offline.","Anglické OCR je součást Windows. Jednou se stáhne od Microsoftu; potřebuje internet a potvrzení správce. Jazyk Windows se nezmění. Rozpoznávání pak funguje offline."));
  if(ImGui::Button(subText("Install English OCR","Doplnit anglické OCR")))ocr.install(window);
  if(ImGui::Button(subText("Check again","Znovu ověřit")))ocr.refresh();
  if(capability.outcome=="cancelled")ImGui::TextWrapped("%s",subText("Administrator prompt was cancelled. You can retry.","Potvrzení správce bylo zrušeno. Můžeš to zkusit znovu."));
  else if(capability.outcome=="installed")ImGui::TextWrapped("%s",subText("Windows reports installation complete, but OCR is not available yet. Restart FrameLab; Windows may require a restart.","Windows hlásí dokončenou instalaci, ale OCR zatím není dostupné. Spusť FrameLab znovu; může být potřeba restart Windows."));
  else if(!capability.outcome.empty())ImGui::TextWrapped("%s",subText("Windows could not install or detect OCR. Check Windows Update or install English language features in Settings.","Instalace nebo kontrola OCR se nepodařila. Zkontroluj Windows Update nebo doplň anglické jazykové funkce v Nastavení."));
  if(ImGui::Button(subText("Windows language settings","Jazykové nastavení Windows")))ShellExecuteW(window,L"open",L"ms-settings:regionlanguage",nullptr,nullptr,SW_SHOWNORMAL);
 }
 }
 ui::field(subText("Translation model","Překladový model"));
 if(ImGui::BeginCombo("##translation-model",subtitleModelCatalog[settings.model].name)){
  for(int id:{0,1,3,2})if(ImGui::Selectable(subtitleModelCatalog[id].name,settings.model==id)){if(settings.model!=id){settings.model=id;settings.enabled=false;}}
  ImGui::EndCombo();
 }
 bool ready=settings.ocrEngine==0?rapidReady:capability.available==1;
 if(settings.model){
  const auto& item=subtitleModelCatalog[settings.model];const auto download=models.snapshot(settings.model);const bool modelReady=download.phase=="ready";ready=ready&&modelReady;
  ImGui::TextWrapped("%s",subText("CUNI is specialized for English to Czech. Q3 uses less memory; Q8 keeps more weight precision. Higher precision is not a guarantee of a better sentence.","CUNI je specializovaný na angličtinu do češtiny. Q3 šetří paměť, Q8 zachová větší přesnost vah. Vyšší přesnost nezaručuje lepší překlad každé věty."));
  if(settings.model==3)ImGui::TextWrapped("%s",subText("Q5 is available to try. No Q5 quality, speed or hardware benchmark has been run for this release.","Q5 je k vyzkoušení. Pro toto vydání nemáme test kvality, rychlosti ani běhu Q5 na hardwaru."));
  ImGui::Text(subText("Model: %.2f GB | estimated GPU: %.1f GiB + headroom","Model: %.2f GB | odhad GPU: %.1f GiB + rezerva"),item.downloadGB,item.requiredGiB);
  ImGui::TextWrapped("%s",subText("First GPU download also needs the 1.62 GB portable runtime. Allow about 8 GB extra disk space for its archive and extraction. Models stay on this PC.","První GPU model stáhne také přenosný runtime (1,62 GB). Pro archiv a rozbalení nech dalších asi 8 GB místa. Modely zůstávají na tomto PC."));
  const bool busy=download.phase=="downloading"||download.phase=="verifying"||download.phase=="extracting";
  bool otherBusy=false;for(int id=1;id<int(subtitleModelCatalog.size());++id)if(id!=settings.model){const auto other=models.snapshot(id);otherBusy|=other.phase=="downloading"||other.phase=="verifying"||other.phase=="extracting";}
  ImGui::Text("%s: %s",subText("Download","Stažení"),download.phase.c_str());
  if(busy){float fraction=download.total?float(double(download.done)/download.total):0;ImGui::ProgressBar(fraction,{-1,0});ImGui::Text("%.2f / %.2f GB",download.done/1.e9,download.total/1.e9);if(ImGui::Button(subText("Pause download","Pozastavit stahování")))models.cancel();}
  else if(!modelReady){ImGui::BeginDisabled(otherBusy);if(ImGui::Button(subText("Download / resume and verify","Stáhnout / pokračovat a ověřit")))models.download(settings.model);ImGui::EndDisabled();}
  if(otherBusy){ImGui::TextWrapped("%s",subText("The other model is downloading. Finish or pause that download first.","Stahuje se druhý model. Nech ho dokončit nebo stahování pozastav."));if(ImGui::Button(subText("Pause other download","Pozastavit druhé stahování")))models.cancel();}
  if(!download.detail.empty())ImGui::TextWrapped("%s",download.detail.c_str());
  if(state.activeModel==settings.model)ImGui::TextColored(ui::accent,"%s",subText("Model loaded on GPU","Model načtený na GPU"));
  else if(freeVramMB>=0){
   const double free=freeVramMB/1024.;ImGui::Text(subText("Free GPU memory now: %.2f GiB","Teď volná paměť GPU: %.2f GiB"),free);
   if(free<item.requiredGiB+.75)ImGui::TextWrapped("%s",subText("Low headroom: try Q3 or CPU, reduce enhancer resolution, or close another GPU app. Full GPU loading is checked; CPU offload is not silently accepted.","Malá rezerva: zkus Q3 nebo CPU, sniž rozlišení enhanceru nebo zavři jinou GPU aplikaci. Úplné načtení na GPU se ověřuje; přesun na CPU se tiše nepovolí."));
   else ImGui::TextWrapped("%s",subText("Enough estimated headroom with the effects currently running. Recheck after increasing resolution, FG or enhancer settings.","Odhadovaná rezerva se současně běžícími efekty stačí. Po zvýšení rozlišení nebo nastavení FG/enhanceru ji znovu zkontroluj."));
  }else ImGui::TextWrapped("%s",subText("Free GPU memory is unavailable; capacity cannot be recommended.","Volnou paměť GPU nelze zjistit; kapacitu nelze doporučit."));
 }
 if(!ready)ImGui::BeginDisabled();
 ImGui::Checkbox(subText("Translate subtitles to Czech","Překládat titulky do češtiny"),&settings.enabled);
 if(!ready)ImGui::EndDisabled();
 if(settings.enabled&&ImGui::Button(subText("Unload translation model","Uvolnit překladový model")))settings.enabled=false;
 ImGui::TextDisabled("English > Čeština | lokálně / offline");
 if(settings.enabled&&!state.status.empty())ImGui::TextWrapped("%s",state.status.c_str());
 ui::field(subText("Subtitle timing","Časování titulků"));
 ImGui::Checkbox(subText("Join split sentences","Spojovat rozdělené věty"),&settings.joinFragments);
 ImGui::TextWrapped("%s",subText("Marks unfinished text and re-translates it with the next lowercase continuation (up to 6 s). Disable if separate dialogue is joined incorrectly.","Označí nedokončený text a přeloží jej znovu s navazující částí začínající malým písmenem (do 6 s). Vypni, pokud chybně spojí různé repliky."));
 ui::field(subText("Minimum on screen (s)","Nejméně na obrazovce (s)"));ImGui::SliderFloat("##subtitle-hold",&settings.holdSeconds,.5f,4.f,"%.1f s");
 ui::field(subText("Keep while waiting (s)","Ponechat při čekání (s)"));ImGui::SliderFloat("##subtitle-linger",&settings.lingerSeconds,settings.holdSeconds,8.f,"%.1f s");
 ImGui::TextWrapped("%s",subText("Time starts when Czech text appears. The next line is translated in parallel. Long captions get extra reading time; the next ready line can replace them after the minimum. Old queued work expires after 6 s.","Čas běží až od zobrazení češtiny. Další věta se mezitím překládá. Dlouhý titulek má víc času na čtení; další hotový jej může nahradit po minimu. Rozpracované titulky starší 6 s se zahodí."));
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
 ui::field(subText("Czech overlay placement","Umístění českého překryvu"));
 ImGui::SetNextItemWidth(-1);ImGui::Combo("##subtitle-placement",&settings.placement,czech?"Vlastní poloha\000Pod originál\000Nad originál\000":"Custom position\000Below original\000Above original\000");
 if(settings.placement){
  ui::field(subText("Gap (% of image height)","Mezera (% výšky obrazu)"));ImGui::SliderFloat("##subtitle-gap",&settings.gapPercent,0,10,"%.1f %%");
  ui::field(subText("When space runs out","Když není místo"));ImGui::Combo("##subtitle-edge",&settings.edgeFallback,czech?"Přisunout k okraji (může se překrývat)\000Přesunout na opačnou stranu\000":"Clamp to edge (may overlap)\000Move to opposite side\000");
  ImGui::TextWrapped("%s",subText("Above/below uses the edges of the OCR area. Keep it tight around the original subtitle. If neither side fits, the overlay stays inside the image.","Nad/pod se řídí okraji oblasti OCR. Vymez ji těsně kolem originálních titulků. Když není místo ani na jedné straně, překryv zůstane uvnitř obrazu."));
 }
 ui::field(subText("Horizontal position","Vodorovná poloha"));ImGui::SliderFloat("##subtitle-x",&settings.horizontal,0,1,"%.2f");
 ui::field(subText("Horizontal anchor","Vodorovné ukotvení"));ImGui::Combo("##subtitle-anchor-x",&settings.horizontalAlign,czech?"Vlevo\000Střed\000Vpravo\000":"Left\000Center\000Right\000");
 if(!settings.placement){
  ui::field(subText("Vertical position","Svislá poloha"));ImGui::SliderFloat("##subtitle-y",&settings.position,0,1,"%.2f");
  ui::field(subText("Vertical anchor","Svislé ukotvení"));ImGui::Combo("##subtitle-anchor-y",&settings.verticalAlign,czech?"Nahoře\000Střed\000Dole\000":"Top\000Center\000Bottom\000");
 }
 ImGui::Checkbox(subText("Drag Czech overlay in preview","Přetahovat český překryv v náhledu"),&settings.editOverlay);
 if(settings.editOverlay)ImGui::TextWrapped("%s",subText("Drag text and background together. Dragging switches to Custom. Fullscreen always hides editing handles.","Přetáhni text společně s pozadím. Přetažení přepne na vlastní polohu. Ve fullscreenu jsou úchyty vždy skryté."));
 ui::field(subText("Background opacity","Krytí pozadí"));ImGui::SliderFloat("##subtitle-opacity",&settings.background,0,1,"%.2f");
 ImGui::Checkbox(subText("Cover original subtitle area","Zakryt oblast původních titulků"),&settings.coverSource);
 ImGui::Checkbox(subText("Include English in overlay","Přidat angličtinu do překryvu"),&settings.showOriginal);
 if(ImGui::CollapsingHeader(subText("Recognition details","Podrobnosti rozpoznání"))){
  ImGui::SliderInt(subText("Read interval (ms)","Interval čtení (ms)"),&settings.intervalMs,200,1500);
  ImGui::TextWrapped("%s",state.status.c_str());ImGui::Text("OCR: %.0f ms | OCR + MT: %.0f ms",state.ocrMilliseconds,state.milliseconds);
  if(settings.ocrEngine==0)ImGui::Text("OCR score: %.2f | queue: %d",state.confidence,state.pending);
  ImGui::TextWrapped("EN: %s",state.source.c_str());ImGui::TextWrapped("CS: %s",state.translated.c_str());
 }
 ImGui::TextWrapped("%s",subText("Uses a very confident reading or two matching readings. OCR can still misread obscured or stylized text. Czech stays visible while the next line is processed. Audio is not used.","Použije velmi jisté čtení nebo dvě shodná čtení. OCR může chybovat u zakrytého či stylizovaného textu. Čeština zůstává vidět při zpracování další věty. Zvuk se nepoužívá."));
 settings.sanitize();return !(settings==before);
}
inline bool drawSubtitleOverlay(ImVec2 origin,ImVec2 size,SubtitleSettings& settings,const SubtitleSnapshot& state,double now,bool cleanOutput=false){
 bool changed=false;const bool edit=settings.editOverlay&&!cleanOutput;
 if(size.x<=0||size.y<=0)return false;
 auto draw=ImGui::GetWindowDrawList();const ImVec2 end{origin.x+size.x,origin.y+size.y};
 // Fit/fill/crop can place parts of the input image outside the actual viewport.
 // Keep the full caption inside the visible image AND window, not just source bounds.
 const auto clipMin=draw->GetClipRectMin(),clipMax=draw->GetClipRectMax(),display=ImGui::GetIO().DisplaySize;
 const ImVec2 visibleMin{std::max({origin.x,clipMin.x,0.f}),std::max({origin.y,clipMin.y,0.f})};
 const ImVec2 visibleMax{std::min({end.x,clipMax.x,display.x}),std::min({end.y,clipMax.y,display.y})};
 const ImVec2 visibleSize{visibleMax.x-visibleMin.x,visibleMax.y-visibleMin.y};
 if(visibleSize.x<=0||visibleSize.y<=0)return false;
 draw->PushClipRect(visibleMin,visibleMax,true);
 if(settings.showRegion&&!cleanOutput){ImVec2 a{origin.x+size.x*settings.left,origin.y+size.y*settings.top},b{origin.x+size.x*settings.right,origin.y+size.y*settings.bottom};draw->AddRect(a,b,IM_COL32(170,230,70,255),0,0,2);draw->AddText({a.x+4,a.y+4},IM_COL32(170,230,70,255),"OCR: EN");}
 const double expires=state.expiresAt>0?state.expiresAt:state.observedAt+4.0;
 if(edit||(settings.enabled&&!state.translated.empty()&&now<expires)){
  if(settings.coverSource)draw->AddRectFilled({origin.x+size.x*settings.left,origin.y+size.y*settings.top},{origin.x+size.x*settings.right,origin.y+size.y*settings.bottom},IM_COL32(0,0,0,255));
  std::string text=state.translated;
  if(text.empty()||now>=expires)text="Ukázka českých titulků\nPřetáhni text i pozadí";
  else if(settings.showOriginal)text+='\n'+(state.displaySource.empty()?state.source:state.displaySource);
  float fs=std::round(std::max(16.f,visibleSize.y*settings.fontPercent/100)),wrap=visibleSize.x*.86f;
  auto font=ui::subtitleFont(fs);
  auto measured=font->CalcTextSizeA(fs,FLT_MAX,wrap,text.c_str());
  const float paddingScale=std::min(visibleSize.x,visibleSize.y),px=paddingScale*.009f,py=paddingScale*.006f;
  // Very long subtitles must remain visible at every aspect ratio.
  for(int i=0;i<48&&(measured.y+2*py>visibleSize.y*.97f||measured.x+2*px>visibleSize.x*.97f);++i){fs*=.8f;measured=font->CalcTextSizeA(fs,FLT_MAX,wrap,text.c_str());}
  auto layout=placeSubtitle(settings,size.x,size.y,measured.x+2*px,measured.y+2*py,{visibleMin.x-origin.x,visibleMin.y-origin.y,visibleMax.x-origin.x,visibleMax.y-origin.y});
  ImVec2 a{origin.x+layout.x,origin.y+layout.y},b{a.x+layout.width,a.y+layout.height};
  if(edit){
   const auto cursor=ImGui::GetCursorScreenPos();ImGui::SetCursorScreenPos(a);ImGui::InvisibleButton("##drag-czech-overlay",{layout.width,layout.height});
   if(ImGui::IsItemActive()&&ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
    const auto delta=ImGui::GetIO().MouseDelta;settings.placement=0;
    settings.horizontal=(layout.x+delta.x+layout.width*settings.horizontalAlign*.5f)/size.x;
    settings.position=(layout.y+delta.y+layout.height*settings.verticalAlign*.5f)/size.y;
    settings.sanitize();changed=true;
   }
   if(ImGui::IsItemHovered()||ImGui::IsItemActive())ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
   ImGui::SetCursorScreenPos(cursor);
  }
  ImVec2 p{a.x+px,a.y+py};
  p.x=std::round(p.x);p.y=std::round(p.y);
  draw->AddRectFilled(a,b,IM_COL32(0,0,0,int(settings.background*255)),5);
  draw->AddText(font,fs,{p.x+1,p.y+1},IM_COL32(0,0,0,255),text.c_str(),nullptr,wrap);
  draw->AddText(font,fs,p,IM_COL32(255,255,255,255),text.c_str(),nullptr,wrap);
  if(edit){draw->AddRect(a,b,IM_COL32(170,230,70,255),5,0,2);if(layout.usedOppositeSide||layout.clamped){
   const char* notice=layout.usedOppositeSide?(settings.placement==1?subText("No room below: moved above original","Dole není místo: přesunuto nad originál"):subText("No room above: moved below original","Nahoře není místo: přesunuto pod originál")):subText("Clamped inside image; may overlap original","Přisunuto do obrazu; může překrýt originál");
   draw->AddText({a.x,std::max(origin.y,a.y-20)},IM_COL32(255,190,70,255),notice);
  }}
 }
 draw->PopClipRect();
 return changed;
}
