#pragma once
#include "OpenAiSettings.hpp"
#include "SubtitleTranslator.hpp"
#include "UiTheme.hpp"
#include "Localization.hpp"

inline bool openAiPanel(SubtitleSettings& s,OpenAiSettings& api,HWND window){
 auto t=[](const char* en,const char* cs){return czech?cs:en;};
 ImGui::TextWrapped("%s",t("Paid translation with your own API key. Only recognized text is sent to OpenAI; OCR stays on this PC. Local models remain available.","Placený překlad vlastním API klíčem. Do OpenAI odchází pouze rozpoznaný text; OCR běží na tomto PC. Lokální modely zůstávají dostupné."));
 static char key[513]{};
 const bool hasKey=api.hasKey();
 if(!hasKey||ImGui::CollapsingHeader(t("Change / remove API key","Změnit / odstranit API klíč"))){
 ImGui::InputText(t("API key","API klíč"),key,sizeof(key),ImGuiInputTextFlags_Password);
 ImGui::BeginDisabled(!key[0]);
 if(ImGui::Button(t("Save key securely","Bezpečně uložit klíč"))){if(api.saveKey(key)){s.enabled=false;++s.apiKeyRevision;api.refresh();}SecureZeroMemory(key,sizeof(key));}
 ImGui::EndDisabled();
 if(hasKey){ImGui::SameLine();if(ImGui::Button(t("Remove saved key","Odstranit uložený klíč"))){s.enabled=false;++s.apiKeyRevision;api.forgetKey();SecureZeroMemory(key,sizeof(key));}}
 }
 ImGui::TextWrapped("%s",hasKey?t("Key saved in Windows Credential Manager for this Windows user.","Klíč je uložený ve Správci pověření Windows pro tohoto uživatele."):t("No key saved. Paste it here; it is never written into the portable package.","Klíč není uložený. Vlož jej sem; do přenosného balíčku se neukládá."));
 ImGui::BeginDisabled(!hasKey||api.busy());if(ImGui::Button(t("Refresh models from my account","Načíst modely z mého účtu")))api.refresh();ImGui::EndDisabled();
 if(api.busy())ImGui::TextDisabled("%s",t("Loading model list...","Načítám seznam modelů…"));
 auto message=api.status();if(!message.empty())ImGui::TextWrapped("%s",message.c_str());
 if(!api.error.empty())ImGui::TextWrapped("%s",api.error.c_str());
 auto models=api.models();
 if(ImGui::BeginCombo(t("API model","Model API"),s.apiModel.c_str())){
  // Available IDs are live. A reviewed profile is required before selection.
  for(const auto& id:models){auto p=api.profile(id);if(p&&ImGui::Selectable(id.c_str(),s.apiModel==id)){s.apiModel=id;s.apiEffort=p->defaultEffort;s.apiTemperatureEnabled=false;s.enabled=false;}}
  if(models.empty())ImGui::TextDisabled("%s",t("Refresh models first","Nejprve načti modely"));ImGui::EndCombo();
 }
 auto p=api.profile(s.apiModel);
 ImGui::TextWrapped(t("Reasoning: %s | default: GPT-6 Luna / none","Thinking: %s | výchozí: GPT-6 Luna / none"),s.apiEffort.c_str());
 if(ImGui::CollapsingHeader(t("Model settings, price and data","Nastavení modelu, cena a data"))){
 if(p){
  if(ImGui::BeginCombo(t("Reasoning / thinking","Uvažování / thinking"),s.apiEffort.c_str())){for(auto& effort:p->efforts)if(ImGui::Selectable(effort.c_str(),effort==s.apiEffort)){s.apiEffort=effort;if(!p->temperature(effort))s.apiTemperatureEnabled=false;}ImGui::EndCombo();}
  if(p->temperature(s.apiEffort)){ImGui::Checkbox(t("Set temperature","Nastavit teplotu"),&s.apiTemperatureEnabled);if(s.apiTemperatureEnabled)ImGui::SliderFloat("##api-temp",&s.apiTemperature,0,2,"%.2f");}
  else{ s.apiTemperatureEnabled=false;ImGui::TextDisabled("%s",t("Temperature is not sent with this configuration.","U této konfigurace se teplota neposílá."));}
  if(p->inputPrice>=0)ImGui::TextWrapped(t("Standard reference price: $%.2f input / $%.2f output per 1M tokens. Profile checked %s; current account billing may differ.","Referenční cena Standard: $%.2f vstup / $%.2f výstup za 1M tokenů. Profil ověřen %s; aktuální účtování se může lišit."),p->inputPrice,p->outputPrice,api.reviewed.c_str());
 }
 ImGui::TextWrapped("%s",t("Start with GPT-6 Luna + none for low cost. More reasoning can increase delay and cost. Grey models have no reviewed translation profile. Availability does not describe capabilities.","Začni GPT-6 Luna + none pro nízkou cenu. Vyšší thinking může zvýšit prodlevu i cenu. Šedé modely nemají ověřený překladový profil. Dostupnost modelu nepopisuje jeho schopnosti."));
 ImGui::TextWrapped("%s",t("store=false disables saved response state, not all provider logging. Check API billing and data controls. Errors stop API translation until you retry; no paid automatic retry loop.","store=false vypíná ukládání stavu odpovědí, nikoli veškeré logování poskytovatele. Zkontroluj účtování a pravidla dat v API. Po chybě se překlad zastaví do ručního opakování; žádná placená smyčka pokusů."));
 if(ImGui::TreeNode(t("Other available IDs (unreviewed)","Další dostupná ID (bez ověřeného profilu)"))){for(auto& id:models)if(!api.profile(id))ImGui::TextDisabled("%s",id.c_str());ImGui::TreePop();}
 }
 if(ImGui::SmallButton(t("API key / billing / setup guide","API klíč / platby / návod")))ShellExecuteW(window,L"open",L"https://opicebot.cz/framelab/guide/remote-subtitles.html#openai",nullptr,nullptr,SW_SHOWNORMAL);
 return hasKey&&p&&api.available(s.apiModel)&&s.sourceLanguage!=s.targetLanguage;
}
