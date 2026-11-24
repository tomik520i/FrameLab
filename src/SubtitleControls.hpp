#pragma once
#include "SubtitleTranslator.hpp"
#include "SubtitleLayout.hpp"
#include "SubtitleText.hpp"
#include "SubtitleModels.hpp"
#include "SubtitleLanguages.hpp"
#include "OcrSetup.hpp"
#include "UiTheme.hpp"
#include "Localization.hpp"
#include "SubtitleProfiles.hpp"
#include "SubtitleRaster.hpp"
#include "OpenAiControls.hpp"
#include "CustomAiControls.hpp"
#include "SubtitleComputeControls.hpp"
#include <cmath>

inline const char* subText(const char* en,const char* cs){return czech?cs:en;}
inline void subtitleUnicodeLabel(const std::string& text,const std::string& language="en"){
 if(text.empty())return;
 auto lang=subtitleLanguage(language);auto image=subtitleRaster.get(text,17.f,std::max(32.f,ImGui::GetContentRegionAvail().x-4),language,lang&&lang->rtl,ImGui::GetFrameCount());
 if(image)ImGui::Image((ImTextureID)(intptr_t)image->texture.view.Get(),{float(image->width),float(image->height)});
 else ImGui::TextWrapped("%s",text.c_str());
}
inline bool subtitlePanel(SubtitleSettings& settings,const SubtitleSnapshot& state,SubtitleModels& models,OpenAiSettings& api,CustomAiSettings& customApi,float freeVramMB,OcrSetup& ocr,HWND window,bool rapidReady,bool* selectStyle=nullptr,SubtitleProfiles* profiles=nullptr,bool* inspector=nullptr){
 const auto before=settings;
 ui::title(subText("SUBTITLE TRANSLATION","PŘEKLAD TITULKŮ"));
 ImGui::TextWrapped("%s",subText("Reads subtitles in the picture and overlays a translation. Choose local or optional API translation.","Čte titulky v obraze a zobrazuje překlad. Vyber místní nebo volitelný API překlad."));
 if(!subtitleRaster.error.empty())ImGui::TextWrapped("%s",subtitleRaster.error.c_str());
 const int previousProvider=settings.provider;
 ImGui::Combo(subText("Translation provider","Způsob překladu"),&settings.provider,"Local / offline\000OpenAI API / BYOK\000Custom AI server / LAN\000");
 if(settings.provider!=previousProvider){settings.enabled=false;if(settings.provider==1&&api.hasKey())api.refresh();}
 auto languageCombo=[&](const char* id,std::string& value){
  const auto current=subtitleLanguage(value);bool changed=false;
  if(ImGui::BeginCombo(id,current?(czech?current->cs:current->name):value.c_str())){
   for(const auto& item:subtitleLanguages)if(ImGui::Selectable(czech?item.cs:item.name,value==item.code)){changed=value!=item.code;value=item.code;}
   ImGui::EndCombo();
  }return changed;
 };
 ui::field(subText("Language in the picture / OCR","Jazyk v obraze / OCR"));
 bool languageChanged=languageCombo("##subtitle-source",settings.sourceLanguage);
 ui::field(subText("Translate into","Přeložit do jazyka"));
 languageChanged=languageCombo("##subtitle-target",settings.targetLanguage)||languageChanged;
 if(languageChanged&&!settings.provider&&settings.model<4&&(settings.sourceLanguage!="en"||settings.targetLanguage!="cs")){settings.model=4;settings.enabled=false;}
 if(settings.sourceLanguage!=before.sourceLanguage){rapidReady=false;ocr.select(settings.sourceLanguage);}
 ImGui::TextWrapped("%s",subText("Choose the actual subtitle language. This is not automatic language detection; the OCR alphabet follows this choice.","Vyber skutečný jazyk titulků. Nejde o automatickou detekci jazyka; podle této volby se přepíná i abeceda OCR."));
 const auto capability=ocr.snapshot();
 ui::field(subText("Text recognition","Rozpoznávání textu"));
 ImGui::Combo("##ocr-engine",&settings.ocrEngine,"RapidOCR / PP-OCRv5 multilingual\000Windows OCR\000");
 if(settings.ocrEngine==0){
  ImGui::TextWrapped("%s",rapidReady?subText("Neural OCR bundled and ready. Offline; Windows OCR installation is unnecessary.","Neuronové OCR je přibalené a připravené. Offline; instalace OCR ve Windows není potřeba."):subText("RapidOCR files missing. Extract the complete package, or choose Windows OCR.","Chybí soubory RapidOCR. Rozbal kompletní balíček, nebo vyber OCR ve Windows."));
 }else{
 if(capability.available==1)ImGui::TextColored(ui::accent,"%s",subText("Windows OCR is ready for this language","OCR ve Windows je pro tento jazyk připravené"));
 else if(capability.installing)ImGui::TextWrapped("%s",subText("Windows is installing English OCR. Recognition will be offline after installation.","Windows instalují anglické OCR. Po instalaci rozpoznávání funguje offline."));
 else if(capability.available<0)ImGui::TextDisabled("%s",subText("Checking Windows OCR...","Kontroluji OCR ve Windows…"));
 else{
  ImGui::TextWrapped("%s",subText("This Windows OCR language is not installed or not supported by Windows. Choose bundled RapidOCR, or add the language's OCR feature in Windows Settings.","Tento jazyk OCR není ve Windows nainstalovaný nebo jej Windows nepodporují. Vyber přibalené RapidOCR, nebo doplň funkci OCR daného jazyka v Nastavení Windows."));
  if(settings.sourceLanguage=="en"&&ImGui::Button(subText("Install English Windows OCR","Doplnit anglické OCR ve Windows")))ocr.install(window);
  if(ImGui::Button(subText("Windows language settings","Jazykové nastavení Windows")))ShellExecuteW(window,L"open",L"ms-settings:regionlanguage",nullptr,nullptr,SW_SHOWNORMAL);
  if(ImGui::Button(subText("Check again","Znovu ověřit")))ocr.refresh();
  if(!capability.outcome.empty())ImGui::TextWrapped("%s",capability.outcome.c_str());
 }
 }
 bool ready=settings.ocrEngine==0?rapidReady:capability.available==1;
 if(settings.provider==1)ready=openAiPanel(settings,api,window)&&ready;
 else if(settings.provider==2)ready=customAiPanel(settings,customApi,window)&&ready;
 else{
 ui::field(subText("Translation model","Překladový model"));
 if(ImGui::BeginCombo("##translation-model",subtitleModelCatalog[settings.model].name)){
  for(int id:{0,1,3,2,4,5,6})if(ImGui::Selectable(subtitleModelCatalog[id].name,settings.model==id)){if(settings.model!=id){settings.model=id;settings.enabled=false;}}
  ImGui::EndCombo();
 }
 ready=ready&&subtitlePairSupported(settings.model,settings.sourceLanguage,settings.targetLanguage);
 if(!subtitlePairSupported(settings.model,settings.sourceLanguage,settings.targetLanguage))ImGui::TextWrapped("%s",subText("Choose different languages. OPUS-MT / CUNI support EN > CS only; other pairs need TranslateGemma.","Vyber dva různé jazyky. OPUS-MT / CUNI umějí jen EN > CS; ostatní páry vyžadují TranslateGemma."));
 if(settings.model){
  ready=subtitleComputePanel(settings,models,state)&&ready;
  const auto& item=subtitleModelCatalog[settings.model];const auto download=models.snapshot(settings.model);const bool modelReady=download.phase=="ready";ready=ready&&modelReady;
  ImGui::Text("%.2f GB | %s",item.downloadGB,modelReady?subText("Ready offline","Připraveno offline"):subText("Download needed","Je potřeba stáhnout"));
  const bool busy=download.phase=="downloading"||download.phase=="verifying"||download.phase=="extracting";
  bool otherBusy=false;for(int id=1;id<int(subtitleModelCatalog.size());++id)if(id!=settings.model){const auto other=models.snapshot(id);otherBusy|=other.phase=="downloading"||other.phase=="verifying"||other.phase=="extracting";}
  ImGui::Text("%s: %s",subText("Download","Stažení"),download.phase.c_str());
  if(busy){float fraction=download.total?float(double(download.done)/download.total):0;ImGui::ProgressBar(fraction,{-1,0});ImGui::Text("%.2f / %.2f GB",download.done/1.e9,download.total/1.e9);if(ImGui::Button(subText("Pause download","Pozastavit stahování")))models.cancel();}
  else if(!modelReady){ImGui::BeginDisabled(otherBusy);if(ImGui::Button(subText("Download / resume and verify","Stáhnout / pokračovat a ověřit")))models.download(settings.model);ImGui::EndDisabled();}
  if(otherBusy){ImGui::TextWrapped("%s",subText("The other model is downloading. Finish or pause that download first.","Stahuje se druhý model. Nech ho dokončit nebo stahování pozastav."));if(ImGui::Button(subText("Pause other download","Pozastavit druhé stahování")))models.cancel();}
  if(!download.detail.empty())ImGui::TextWrapped("%s",download.detail.c_str());
  if(ImGui::CollapsingHeader(subText("Model details / GPU requirements","Podrobnosti modelu / nároky na GPU"))){
  if(settings.model<4)ImGui::TextWrapped("%s",subText("CUNI is specialized for English to Czech. Q3 uses less memory; Q8 keeps more weight precision. Higher precision is not a guarantee of a better sentence.","CUNI je specializovaný na angličtinu do češtiny. Q3 šetří paměť, Q8 zachová větší přesnost vah. Vyšší přesnost nezaručuje lepší překlad každé věty."));
  if(settings.model>=4){
   ImGui::TextWrapped("%s",subText("TranslateGemma translates between the selected languages. Start with 4B Q4 on 6 GB GPUs. Q8 needs more free VRAM; 12B is intended for larger GPUs. Leave memory for frame generation and enhancement.","TranslateGemma překládá mezi vybranými jazyky. Na 6GB GPU začni 4B Q4. Q8 potřebuje více volné VRAM; 12B je pro větší GPU. Nech rezervu pro frame gen a enhancer."));
   ImGui::TextWrapped("%s",subText("Google Gemma license; community GGUF quantization. The download is optional and verified. No account or cloud inference is used by FrameLab.","Licence Google Gemma; komunitní kvantizace GGUF. Stažení je volitelné a ověřuje se. FrameLab nepoužívá účet ani cloudový překlad."));
   if(ImGui::SmallButton(subText("Gemma license / model information","Licence Gemma / informace o modelu")))ShellExecuteW(window,L"open",L"https://ai.google.dev/gemma/terms",nullptr,nullptr,SW_SHOWNORMAL);
  }
  if(settings.model==3)ImGui::TextWrapped("%s",subText("Q5 is used in the published live-caption examples. Compare quality on your own material; higher precision does not guarantee a better translation.","Q5 je použitý ve zveřejněných ukázkách živých titulků. Kvalitu porovnej na vlastním obsahu; vyšší přesnost nezaručuje lepší překlad."));
  ImGui::Text(subText("Model: %.2f GB | estimated GPU: %.1f GiB + headroom","Model: %.2f GB | odhad GPU: %.1f GiB + rezerva"),item.downloadGB,item.requiredGiB);
  ImGui::TextWrapped("%s",subText("First GPU download also needs the 1.62 GB portable runtime. Allow about 8 GB extra disk space for its archive and extraction. Models stay on this PC.","První GPU model stáhne také přenosný runtime (1,62 GB). Pro archiv a rozbalení nech dalších asi 8 GB místa. Modely zůstávají na tomto PC."));
  ImGui::TextWrapped("%s",subText("Memory is checked on the selected translation device when loading. Vulkan can use shared memory on integrated GPUs; this is not dedicated VRAM. GPU + RAM offload is optional.","Paměť se při načítání kontroluje na zvolené GPU překladače. Vulkan může na integrované grafice používat sdílenou paměť; nejde o vyhrazenou VRAM. GPU + RAM je volitelný režim."));
  }
 }
 }
 ui::field(subText("Conversation memory","Paměť rozhovoru"));
 ImGui::BeginDisabled(!settings.provider&&settings.model==0);
 ImGui::Checkbox(subText("Use recent dialogue","Použít poslední repliky rozhovoru"),&settings.conversationMemory);
 ImGui::EndDisabled();
 ImGui::TextWrapped("%s",subText("Off by default. Sends up to six earlier source captions from the last 60 seconds, with times. Resets when the source, language or model changes, or translation stops. It does not identify characters or infer their gender.","Ve výchozím stavu vypnuto. Přidává nejvýše šest předchozích originálních titulků z posledních 60 sekund včetně časů. Maže se při změně zdroje, jazyka či modelu nebo vypnutí překladu. Postavy ani jejich rod samo nerozpoznává."));
 if(!settings.provider&&settings.model==0)ImGui::TextDisabled("%s",subText("OPUS-MT does not support dialogue history.","OPUS-MT historii rozhovoru nepodporuje."));
 else if(!settings.provider&&settings.model<4)ImGui::TextWrapped("%s",subText("CUNI: recommended OFF. Q3 tests repeated earlier dialogue and scored worse with history. Other quantizations have not been verified for this feature.","CUNI: doporučeno VYPNOUT. Q3 v testu opakovalo starší dialog a s historií dopadlo hůře. Ostatní kvantizace pro tuto funkci ověřené nejsou."));
 else if(settings.provider==1)ImGui::TextWrapped("%s",subText("Optional: Luna sometimes resolved ambiguity better. History adds input tokens, latency and API cost. Other models can behave differently.","Volitelné: Luna někdy lépe vyřešila dvojznačnost. Historie přidává vstupní tokeny, prodlevu a cenu API. Jiné modely se mohou chovat jinak."));
 else ImGui::TextWrapped("%s",subText("Try on your model. History can help or cause invented/repeated lines, especially on weaker models. Turn it off if the current line is lost.","Vyzkoušej na svém modelu. Historie může pomoci, ale hlavně slabší modely mohou domýšlet nebo opakovat repliky. Pokud se ztrácí aktuální věta, vypni ji."));
 if(settings.conversationMemory){
  if(!state.contextStatus.empty()&&ImGui::CollapsingHeader(subText("Context used for the last translation","Kontext použitý pro poslední překlad"))){ImGui::TextWrapped("%s",state.contextStatus.c_str());subtitleUnicodeLabel(state.contextDetails,settings.sourceLanguage);}
  ImGui::BeginDisabled(!settings.provider&&settings.model<4);
  ImGui::Checkbox(subText("Remember important dialogue (experimental)","Uchovat důležité repliky (experiment)"),&settings.selectedMemory);
  ImGui::EndDisabled();
  ImGui::TextWrapped("%s",subText("The selected model periodically chooses original dialogue to retain for up to 10 minutes. No other AI is used. Extra inference adds delay and API cost. Can select wrong context and worsen translation; off by default. Intended for general instruction-following LLMs; CUNI cannot build this memory. Clear it by stopping translation when changing conversations.","Vybraný model průběžně vybírá původní repliky k uchování až na 10 minut. Žádné jiné AI se nepoužívá. Další výpočet přidává prodlevu a cenu API. Může vybrat špatný kontext a překlad zhoršit; výchozí stav je vypnuto. Určeno pro obecné LLM schopné plnit instrukce; CUNI tuto paměť vytvářet neumí. Při změně rozhovoru ji smaž zastavením překladu."));
 }
 if(ImGui::SmallButton(subText("History comparison and examples","Porovnání historie a příklady")))ShellExecuteW(window,L"open",czech?L"https://opicebot.cz/framelab/runs/2026-11-24-context-memory/?lang=cs":L"https://opicebot.cz/framelab/runs/2026-11-24-context-memory/?lang=en",nullptr,nullptr,SW_SHOWNORMAL);
 if(!ready)ImGui::BeginDisabled();
 ImGui::Checkbox(settings.provider==1?subText("Enable paid API translation","Zapnout placený API překlad"):subText("Translate subtitles","Překládat titulky"),&settings.enabled);
 if(!ready)ImGui::EndDisabled();
 if(settings.enabled&&ImGui::Button(settings.provider?subText("Stop API translation","Zastavit API překlad"):subText("Unload translation model","Uvolnit překladový model")))settings.enabled=false;
 ImGui::TextDisabled("%s > %s | %s",settings.sourceLanguage.c_str(),settings.targetLanguage.c_str(),settings.provider==1?"OpenAI API":settings.provider==2?"Custom AI server":"offline");
 ui::field(subText("Subtitle appearance","Vzhled originálních titulků"));
 const char* filters[]={subText("Off / translate all OCR text","Vypnuto / překládat všechno z OCR"),subText("Basic / obvious noise and plates","Základní / zjevné chyby a SPZ"),settings.provider?subText("Smart / API content filter","Chytrý / posouzení přes API"):subText("Smart / conservative EN filter","Chytrý / konzervativní filtr EN")};
 ImGui::Combo(subText("Text filter","Filtr textu"),&settings.contentFilter,filters,3);
 if(settings.provider&&settings.contentFilter==2)ImGui::TextWrapped("%s",subText("The API checks for clear non-dialogue in the same request as translation. Uncertain speech should be kept. Region and appearance filtering still happen locally.","API posuzuje zjevný text mimo dialog ve stejném požadavku jako překlad. Nejisté repliky má ponechat. Filtr oblasti a vzhledu dál běží místně."));
 if(!settings.provider&&settings.contentFilter==2){ImGui::TextWrapped("%s",subText("English: a small CPU model recognizes standalone names. Gemma also checks label-shaped text; CUNI suppresses unchanged names after translation. Other languages use the basic filter. Select the subtitle region/style too: uncertain text is kept to protect dialogue.","Angličtina: malý model na CPU poznává samostatná jména. Gemma navíc posuzuje nápisy; CUNI potlačí i jména ponechaná překladem beze změny. Jiné jazyky používají základní filtr. Vyber také oblast a vzhled titulků: nejistý text se ponechá, aby nemizel dialog."));
  if(!settings.provider&&settings.enabled&&state.activeModel>=0&&settings.sourceLanguage=="en")ImGui::TextWrapped("%s",state.filterBackend=="ner"?subText("Name model active on CPU (no additional GPU memory)","Model jmen běží na CPU (bez další paměti GPU)"):subText("Name model missing / unavailable; conservative fallback active. Use the complete package.","Model jmen chybí / není dostupný; běží konzervativní náhrada. Použij kompletní balíček."));}
 if(state.performance.filtered)ImGui::Text(subText("Filtered: %llu","Odfiltrováno: %llu"),(unsigned long long)state.performance.filtered);
 if(selectStyle){
  ImGui::BeginDisabled(settings.ocrEngine!=0||!rapidReady);
  if(ui::button(subText("Select subtitle word + reading area...","Označit slovo titulku + oblast čtení..."),true))*selectStyle=true;
  ImGui::EndDisabled();
  ImGui::TextWrapped("%s",subText("Mark one word to learn its color and size. Set a larger reading area for multiple lines; save the result as a profile below.","Označ jedno slovo pro barvu a velikost. Oblast čtení nech větší pro více řádků; výběr můžeš níže uložit jako profil."));
 }
 if(inspector){if(ui::button(subText("OCR inspector / saved sample...","Inspektor OCR / uložený vzor…"),true))*inspector=true;ImGui::TextWrapped("%s",subText("See the sampled word, colour, letter height and accepted/rejected text.","Zobraz vzor slova, barvu, výšku písmen a přijaté/odmítnuté texty."));}
 if(profiles){
  const auto active=profiles->find(profiles->selected);std::string label=active?active->name:subText("Current settings","Aktuální nastavení");if(active&&!SubtitleProfiles::matches(*active,settings))label+=subText(" (modified)"," (upraveno)");
  if(ImGui::BeginCombo("##subtitle-profile",label.c_str())){for(auto& p:profiles->entries)if(ImGui::Selectable(p.name.c_str(),p.id==profiles->selected))profiles->choose(p.id,settings);if(profiles->entries.empty())ImGui::TextDisabled("%s",subText("No saved profiles yet","Zatím nejsou uložené profily"));ImGui::EndCombo();}
  static char profileName[160]{};
  if(ImGui::Button(subText("Save as...","Uložit jako…"))){profileName[0]=0;ImGui::OpenPopup("##name-subtitle-profile");}
  if(active){ImGui::SameLine();if(ImGui::Button(subText("Save changes","Uložit změny")))profiles->update(settings);}
  if(ImGui::BeginPopup("##name-subtitle-profile")){ImGui::TextUnformatted(subText("Profile name","Název profilu"));ImGui::InputText("##profile-name",profileName,sizeof(profileName));if(ImGui::Button(subText("Save profile","Uložit profil"))&&profiles->add(profileName,settings))ImGui::CloseCurrentPopup();ImGui::EndPopup();}
  if(active&&ImGui::SmallButton(subText("Delete profile...","Smazat profil…")))ImGui::OpenPopup("##delete-subtitle-profile");
  if(ImGui::BeginPopup("##delete-subtitle-profile")){ImGui::TextWrapped("%s",subText("Delete this saved profile? Current settings will stay active.","Smazat tento uložený profil? Aktuální nastavení zůstane aktivní."));if(ImGui::Button(subText("Delete","Smazat"))){profiles->removeSelected();ImGui::CloseCurrentPopup();}ImGui::EndPopup();}
  if(!profiles->error.empty())ImGui::TextWrapped("%s",subText("Profiles could not be saved or loaded. Check that the application folder is writable and enter a valid name.","Profily se nepodařilo uložit nebo načíst. Ověř zápis do složky aplikace a vyplň platný název."));
 }
 ImGui::BeginDisabled(!rapidReady||settings.ocrEngine!=0||!selectStyle);
 if(ImGui::Button(subText("Select text sample...","Vybrat vzor textu…"))&&selectStyle)*selectStyle=true;
 ImGui::EndDisabled();
 if(settings.style.enabled){
  ImGui::SameLine();if(ImGui::Button(subText("Reset to automatic","Obnovit automatiku")))settings.style.enabled=false;
  ImGui::TextWrapped("%s",settings.ocrEngine==0?subText("Matching colour and letter size. The reading area still allows longer and multiline captions.","Čte odpovídající barvu a velikost písmen. Oblast čtení dál umožňuje delší i víceřádkové titulky."):subText("Saved appearance filter is inactive with Windows OCR. Select RapidOCR to use it.","Uložený filtr vzhledu se ve Windows OCR nepoužívá. Aktivuješ jej výběrem RapidOCR."));
 }else ImGui::TextWrapped("%s",subText("Reading signs or number plates? Select one subtitle word as a colour and size sample.","Čte nápisy nebo SPZ? Vyber jedno slovo titulku jako vzor barvy a velikosti."));
 if(settings.ocrEngine!=0)ImGui::TextDisabled("%s",subText("Appearance selection requires bundled RapidOCR.","Výběr vzhledu vyžaduje přibalené RapidOCR."));
 if(settings.style.enabled&&ImGui::CollapsingHeader(subText("Appearance details","Podrobnosti vzhledu"))){
  ImGui::SliderFloat(subText("Size tolerance","Tolerance velikosti"),&settings.style.heightTolerance,.15f,.8f,"%.2f");
  ImGui::SliderFloat(subText("Colour tolerance","Tolerance barvy"),&settings.style.colorTolerance,10.f,65.f,"%.0f");
  ImGui::TextWrapped("%s",subText("Similar-looking signs can still match. Narrow the reading area if needed. No match means waiting, never falling back to all text.","Podobně vypadající nápis může stále projít. Pomůže zúžení oblasti čtení. Když nic neodpovídá, čeká se; filtr se sám nevypíná."));
  for(const auto& line:state.ocrLines){const char* en[]={"accepted","outside area","size","colour"};const char* cs[]={"přijato","mimo oblast","velikost","barva"};ImGui::TextWrapped("%s: %s",(czech?cs:en)[std::clamp(line.reason,0,3)],line.text.c_str());}
 }
 if(settings.enabled&&!state.status.empty())ImGui::TextWrapped("%s",state.status.c_str());
 ui::field(subText("Subtitle timing","Časování titulků"));
 ImGui::BeginDisabled(settings.provider||settings.model>=4||settings.useConversationMemory());
 ImGui::Checkbox(subText("Join split sentences","Spojovat rozdělené věty"),&settings.joinFragments);
 ImGui::TextWrapped("%s",subText("Marks unfinished text and re-translates it with the next lowercase continuation (up to 6 s). Disable if separate dialogue is joined incorrectly.","Označí nedokončený text a přeloží jej znovu s navazující částí začínající malým písmenem (do 6 s). Vypni, pokud chybně spojí různé repliky."));
 ImGui::EndDisabled();
 if(!settings.provider&&settings.model>=4)ImGui::TextWrapped("%s",subText("TranslateGemma receives complete OCR readings without the English continuation heuristic.","TranslateGemma dostává celé čtení OCR bez anglické heuristiky spojování vět."));
 ui::field(subText("Minimum on screen (s)","Nejméně na obrazovce (s)"));ImGui::SliderFloat("##subtitle-hold",&settings.holdSeconds,.5f,4.f,"%.1f s");
 ui::field(subText("Keep while waiting (s)","Ponechat při čekání (s)"));ImGui::SliderFloat("##subtitle-linger",&settings.lingerSeconds,settings.holdSeconds,8.f,"%.1f s");
 ImGui::TextWrapped("%s",subText("Time starts when translated text appears. The next line is translated in parallel. Long captions get extra reading time; the next ready line can replace them after the minimum. Old queued work expires after 6 s.","Čas běží až od zobrazení překladu. Další věta se mezitím překládá. Dlouhý titulek má víc času na čtení; další hotový jej může nahradit po minimu. Rozpracované titulky starší 6 s se zahodí."));
 ImGui::Checkbox(subText("Show reading area","Zobrazit oblast čtení"),&settings.showRegion);
 if(ImGui::CollapsingHeader(subText("Subtitle reading area","Oblast původních titulků"),ImGuiTreeNodeFlags_DefaultOpen)){
  ImGui::TextWrapped("%s",subText("Select just the subtitle band. Exclude chat, minimap and quest text.","Vyber jen pruh s titulky. Vynech chat, minimapu a úkoly."));
  ImGui::SliderFloat(subText("Left","Zleva"),&settings.left,0,.95f,"%.2f");
  ImGui::SliderFloat(subText("Right","Zprava"),&settings.right,.02f,1,"%.2f");
  ImGui::SliderFloat(subText("Top","Shora"),&settings.top,0,.95f,"%.2f");
  ImGui::SliderFloat(subText("Bottom","Zdola"),&settings.bottom,.02f,1,"%.2f");
  if(ImGui::Button(subText("Bottom subtitles preset","Předvolba: titulky dole"))){settings.left=.1f;settings.right=.9f;settings.top=.72f;settings.bottom=.97f;}
 }
 ui::field(subText("Translated subtitle size","Velikost přeložených titulků"));ImGui::SliderFloat("##subtitle-size",&settings.fontPercent,2,8,"%.1f %%");
 ui::field(subText("Translated overlay placement","Umístění přeložených titulků"));
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
 ImGui::Checkbox(subText("Drag translated overlay in preview","Přetahovat přeložené titulky v náhledu"),&settings.editOverlay);
 if(settings.editOverlay)ImGui::TextWrapped("%s",subText("Drag text and background together. Dragging switches to Custom. Fullscreen always hides editing handles.","Přetáhni text společně s pozadím. Přetažení přepne na vlastní polohu. Ve fullscreenu jsou úchyty vždy skryté."));
 ui::field(subText("Background opacity","Krytí pozadí"));ImGui::SliderFloat("##subtitle-opacity",&settings.background,0,1,"%.2f");
 ImGui::Checkbox(subText("Cover original subtitle area","Zakryt oblast původních titulků"),&settings.coverSource);
 ImGui::Checkbox(subText("Include original text in overlay","Přidat původní text do překryvu"),&settings.showOriginal);
 if(ImGui::CollapsingHeader(subText("Recognition details","Podrobnosti rozpoznání"))){
  ImGui::SliderInt(subText("Read interval (ms)","Interval čtení (ms)"),&settings.intervalMs,200,1500);
  ImGui::TextWrapped("%s",state.status.c_str());ImGui::Text("OCR: %.0f ms | OCR + MT: %.0f ms",state.ocrMilliseconds,state.milliseconds);
  if(settings.ocrEngine==0)ImGui::Text("OCR score: %.2f | queue: %d",state.confidence,state.pending);
  ImGui::TextUnformatted(settings.sourceLanguage.c_str());subtitleUnicodeLabel(state.source,settings.sourceLanguage);ImGui::TextUnformatted(settings.targetLanguage.c_str());subtitleUnicodeLabel(state.translated,settings.targetLanguage);
 }
 ImGui::TextWrapped("%s",subText("Uses a very confident reading or two matching readings. OCR can still misread obscured or stylized text. The translation stays visible while the next line is processed. Audio is not used.","Použije velmi jisté čtení nebo dvě shodná čtení. OCR může chybovat u zakrytého či stylizovaného textu. Překlad zůstává vidět při zpracování další věty. Zvuk se nepoužívá."));
 settings.sanitize();return !(settings==before);
}
inline bool drawSubtitleOverlay(ImVec2 origin,ImVec2 size,SubtitleSettings& settings,const SubtitleSnapshot& state,double now,bool cleanOutput=false,ImDrawList* recordingDraw=nullptr){
 bool changed=false;const bool edit=settings.editOverlay&&!cleanOutput;
 if(size.x<=0||size.y<=0)return false;
 auto draw=recordingDraw?recordingDraw:ImGui::GetWindowDrawList();const ImVec2 end{origin.x+size.x,origin.y+size.y};
 // Fit/fill/crop can place parts of the input image outside the actual viewport.
 // Keep the full caption inside the visible image AND window, not just source bounds.
 const auto clipMin=draw->GetClipRectMin(),clipMax=draw->GetClipRectMax(),display=recordingDraw?end:ImGui::GetIO().DisplaySize;
 const ImVec2 visibleMin{std::max({origin.x,clipMin.x,0.f}),std::max({origin.y,clipMin.y,0.f})};
 const ImVec2 visibleMax{std::min({end.x,clipMax.x,display.x}),std::min({end.y,clipMax.y,display.y})};
 const ImVec2 visibleSize{visibleMax.x-visibleMin.x,visibleMax.y-visibleMin.y};
 if(visibleSize.x<=0||visibleSize.y<=0)return false;
 draw->PushClipRect(visibleMin,visibleMax,true);
 if(settings.showRegion&&!cleanOutput){ImVec2 a{origin.x+size.x*settings.left,origin.y+size.y*settings.top},b{origin.x+size.x*settings.right,origin.y+size.y*settings.bottom};draw->AddRect(a,b,IM_COL32(170,230,70,255),0,0,2);draw->AddText({a.x+4,a.y+4},IM_COL32(170,230,70,255),("OCR: "+settings.sourceLanguage).c_str());}
 const double expires=state.expiresAt>0?state.expiresAt:state.observedAt+4.0;
 if(edit||(settings.enabled&&!state.translated.empty()&&now<expires)){
  if(settings.coverSource)draw->AddRectFilled({origin.x+size.x*settings.left,origin.y+size.y*settings.top},{origin.x+size.x*settings.right,origin.y+size.y*settings.bottom},IM_COL32(0,0,0,255));
  std::string text=state.translated;
  if(text.empty()||now>=expires)text="Ukázka přeložených titulků\nPřetáhni text i pozadí";
  else if(settings.showOriginal)text+='\n'+(state.displaySource.empty()?state.source:state.displaySource);
  float fs=std::round(std::max(16.f,visibleSize.y*settings.fontPercent/100)),wrap=visibleSize.x*.86f;
  auto font=ui::subtitleFont(fs);
  auto lines=wrapSubtitleText(font,fs,wrap,text);auto measured=lines.size;
  const auto language=subtitleLanguage(settings.targetLanguage);
  auto raster=subtitleRaster.get(text,fs,wrap,settings.targetLanguage,language&&language->rtl,ImGui::GetFrameCount());
  if(raster)measured={float(raster->width),float(raster->height)};
  const float paddingScale=std::min(visibleSize.x,visibleSize.y),px=paddingScale*.009f,py=paddingScale*.006f;
  // Very long subtitles must remain visible at every aspect ratio.
  for(int i=0;i<48&&(measured.y+2*py>visibleSize.y*.97f||measured.x+2*px>visibleSize.x*.97f);++i){fs*=.8f;lines=wrapSubtitleText(font,fs,wrap,text);measured=lines.size;raster=subtitleRaster.get(text,fs,wrap,settings.targetLanguage,language&&language->rtl,ImGui::GetFrameCount());if(raster)measured={float(raster->width),float(raster->height)};}
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
  if(raster)draw->AddImage((ImTextureID)(intptr_t)raster->texture.view.Get(),p,{p.x+raster->width,p.y+raster->height});
  else for(size_t i=0;i<lines.lines.size();++i){const auto& line=lines.lines[i];
   const ImVec2 pos{std::round(a.x+(layout.width-line.width)*.5f),std::round(p.y+float(i)*fs)};
   draw->AddText(font,fs,{pos.x+1,pos.y+1},IM_COL32(0,0,0,255),line.text.c_str());
   draw->AddText(font,fs,pos,IM_COL32(255,255,255,255),line.text.c_str());
  }
  if(edit){draw->AddRect(a,b,IM_COL32(170,230,70,255),5,0,2);if(layout.usedOppositeSide||layout.clamped){
   const char* notice=layout.usedOppositeSide?(settings.placement==1?subText("No room below: moved above original","Dole není místo: přesunuto nad originál"):subText("No room above: moved below original","Nahoře není místo: přesunuto pod originál")):subText("Clamped inside image; may overlap original","Přisunuto do obrazu; může překrýt originál");
   draw->AddText({a.x,std::max(origin.y,a.y-20)},IM_COL32(255,190,70,255),notice);
  }}
 }
 draw->PopClipRect();
 return changed;
}
