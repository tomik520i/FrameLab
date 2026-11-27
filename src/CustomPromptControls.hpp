#pragma once
#include "SubtitleTranslator.hpp"
#include "SubtitleLanguages.hpp"
#include "Localization.hpp"
#include "imgui.h"
#include <algorithm>
#include <array>
#include <cstring>

namespace custom_prompt {
inline constexpr char defaultInstruction[]="Translate game subtitles from {source_language} to {target_language}. Return only the translated subtitle, without explanations or reasoning. Preserve meaning, tone, numbers and names.";
inline constexpr char exampleInstruction[]=R"(Translate the current subtitle from {source_language} to {target_language}.
Preserve who does what to whom, negatives, conditions, slang and intensity.
If a line starts with a speaker label such as Alex:, keep that label unchanged; it is not an addressee. Keep separate speakers on separate lines.
Use supplied original dialogue history only to resolve ambiguity. Do not translate the history or invent missing events, relationships or dialogue.
Return only the translation in the response format requested below. Do not explain your answer.)";
struct Draft {
 bool enabled=false;int format=0;std::array<char,4097> text{};
 void setText(const std::string& value){text.fill(0);memcpy(text.data(),value.data(),std::min(value.size(),text.size()-1));}
 void load(const SubtitleSettings& s){enabled=s.customPromptEnabled;format=s.customResponseFormat;setText(s.customInstruction);}
 void example(){enabled=true;setText(exampleInstruction);}
 void defaults(){enabled=false;format=0;setText("");}
 void apply(SubtitleSettings& s)const{s.customPromptEnabled=enabled;s.customResponseFormat=format;s.customInstruction=text.data();}
 std::string preview(const SubtitleSettings& s)const{
  std::string result=enabled&&text[0]?text.data():defaultInstruction;
  auto replace=[&](const char* token,const char* value){size_t i=0;while((i=result.find(token,i))!=std::string::npos){result.replace(i,strlen(token),value);i+=strlen(value);}};
  if(auto lang=subtitleLanguage(s.sourceLanguage))replace("{source_language}",lang->name);
  if(auto lang=subtitleLanguage(s.targetLanguage))replace("{target_language}",lang->name);
  return result;
 }
};
}

// Editing is a draft; only the explicit Apply button changes/reloads a translator.
inline void customPromptEditor(SubtitleSettings& s){
 auto t=[](const char* en,const char* cs){return czech?cs:en;};
 static custom_prompt::Draft draft;
 if(ImGui::Button(t("Edit translation prompt...","Upravit překladový prompt..."))){draft.load(s);ImGui::OpenPopup("Translation prompt###custom-prompt-editor");}
 ImGui::TextDisabled("%s",s.customPromptEnabled?t("Custom prompt enabled","Vlastní prompt zapnutý"):t("Default prompt","Výchozí prompt"));
 const auto work=ImGui::GetMainViewport()->WorkSize;
 ImGui::SetNextWindowSize({std::min(760.f,work.x-24),std::min(720.f,work.y-24)},ImGuiCond_Appearing);
 bool open=true;
 if(ImGui::BeginPopupModal(t("Translation prompt###custom-prompt-editor","Překladový prompt###custom-prompt-editor"),&open)){
  ImGui::TextWrapped("%s",t("Optional instructions for your custom AI server. Changes are applied only when saved. Memory settings and its separate prompt are unchanged.","Volitelné instrukce pro tvůj vlastní AI server. Změny se projeví až po uložení. Nastavení paměti i její samostatný prompt zůstávají beze změny."));
  ImGui::Checkbox(t("Use custom prompt","Použít vlastní prompt"),&draft.enabled);
  ImGui::BeginDisabled(!draft.enabled);
  ImGui::TextWrapped("%s",t("Instruction (blank uses the default)","Instrukce (prázdné použije výchozí)"));
  ImGui::InputTextMultiline("##translation-instruction",draft.text.data(),draft.text.size(),{-1,190},ImGuiInputTextFlags_AllowTabInput);
  ImGui::TextWrapped("%s",t("Language placeholders: {source_language}, {target_language}. The current OCR text and enabled original dialogue history are attached automatically; do not paste them into this field.","Značky jazyka: {source_language}, {target_language}. Aktuální OCR text a zapnutá historie původních dialogů se připojí automaticky; sem je nevkládej."));
  ImGui::SetNextItemWidth(-1);
  ImGui::Combo("##translation-response-format",&draft.format,t("Plain text\000<translation>...</translation>\000","Prostý text\000<translation>...</translation>\000"));
  ImGui::TextWrapped("%s",t("Response format. Tag mode adds its format instruction automatically. Malformed answers are skipped without stopping the translator. Plain mode displays returned text directly.","Formát odpovědi. Režim značek přidá instrukci k formátu automaticky. Chybně zapsané odpovědi přeskočí, překladač pokračuje. Prostý režim zobrazuje vrácený text přímo."));
  ImGui::EndDisabled();
  if(ImGui::Button(t("Load example","Načíst ukázku")))draft.example();
  ImGui::SameLine();if(ImGui::Button(t("Restore default","Obnovit výchozí")))draft.defaults();
  ImGui::TextWrapped("%s",t("The example is a starting point, not a verified quality improvement for every model.","Ukázka je výchozí bod pro ladění, ne ověřené zlepšení kvality pro každý model."));
  if(ImGui::CollapsingHeader(t("Preview instruction and format","Náhled instrukce a formátu"))){
   ImGui::TextWrapped("%s",draft.preview(s).c_str());
   ImGui::TextWrapped("%s",t("FrameLab also adds its OCR-data, active filtering and context guidance. This preview shows your instruction, not a live request.","FrameLab přidává také pravidla pro OCR data, aktivní filtr a kontext. Náhled ukazuje tvoji instrukci, ne živý požadavek."));
   ImGui::Separator();ImGui::TextWrapped("%s","EN: Alex: Don't leave me here!");
   ImGui::TextWrapped("%s",draft.enabled&&draft.format==1?"<translation>Alex: Nenechávej mě tady!</translation>":"Alex: Nenechávej mě tady!");
   ImGui::TextWrapped("%s",t("Example for EN > CS. Only the subtitle inside the tags is displayed.","Příklad pro EN > CZ. Zobrazí se pouze titulek uvnitř značek."));
  }
  ImGui::Separator();
  if(ImGui::Button(t("Save and apply","Uložit a použít"))){draft.apply(s);ImGui::CloseCurrentPopup();}
  ImGui::SameLine();if(ImGui::Button(t("Cancel","Zrušit")))ImGui::CloseCurrentPopup();
  ImGui::TextWrapped("%s",t("Saved with app settings. Applying a change reconnects an active translator and clears its current session history.","Uloží se s nastavením aplikace. Použití změny znovu připojí aktivní překladač a vymaže jeho aktuální historii relace."));
  ImGui::EndPopup();
 }
}
