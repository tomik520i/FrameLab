#pragma once
#include "CustomAiSettings.hpp"
#include "UiTheme.hpp"
#include "Localization.hpp"

inline bool customAiPanel(SubtitleSettings& s,CustomAiSettings& api,HWND window){
 auto t=[](const char* en,const char* cs){return czech?cs:en;};
 auto field=[](const char* label,std::string& value,size_t capacity,bool multiline=false){std::vector<char> buffer(capacity);memcpy(buffer.data(),value.data(),std::min(value.size(),capacity-1));ui::field(label);auto id=std::string("##custom-")+label;ImGui::SetNextItemWidth(-1);const bool changed=multiline?ImGui::InputTextMultiline(id.c_str(),buffer.data(),buffer.size(),{-1,96}):ImGui::InputText(id.c_str(),buffer.data(),buffer.size());if(changed)value=buffer.data();return changed;};
 const auto before=s;
 ImGui::TextWrapped("%s",t("Optional: use an AI server on this PC or another computer. OCR stays here; recognized text is sent to your chosen address. The model uses the server's hardware.","Volitelně: použij AI server na tomto nebo jiném počítači. OCR běží tady; rozpoznaný text se posílá na zadanou adresu. Model využívá hardware serveru."));
 int protocol=s.customProtocol=="ollama"?1:0;ui::field(t("Server API","API serveru"));ImGui::SetNextItemWidth(-1);if(ImGui::Combo("##custom-protocol",&protocol,"OpenAI-compatible (llama.cpp / LM Studio)\000Ollama\000"))s.customProtocol=protocol?"ollama":"openai";
 field(t("Base URL / LAN address","Základní URL / adresa v síti"),s.customUrl,2049);
 ImGui::TextDisabled("%s",s.customProtocol=="ollama"?"http://192.168.1.100:11434":"http://192.168.1.100:8080/v1");
 ImGui::BeginDisabled(api.busy());if(ImGui::Button(t("Connect / refresh models","Připojit / načíst modely")))api.refresh(s);ImGui::EndDisabled();
 auto ids=api.models(s);
 ui::field(t("Server model","Model na serveru"));ImGui::SetNextItemWidth(-1);if(ImGui::BeginCombo("##custom-model",s.customModel.empty()?t("Select or enter below","Vyber nebo zadej níže"):s.customModel.c_str())){for(auto& id:ids)if(ImGui::Selectable(id.c_str(),s.customModel==id))s.customModel=id;ImGui::EndCombo();}
 field(t("Exact model ID","Přesné ID modelu"),s.customModel,1025);
 ImGui::TextWrapped("%s",api.status().c_str());
 const bool hasKey=api.hasKey(s);static char key[4097]{};static std::string keyAddress;
 if(keyAddress!=s.customUrl+"|"+s.customProtocol){SecureZeroMemory(key,sizeof(key));keyAddress=s.customUrl+"|"+s.customProtocol;}
 if(ImGui::CollapsingHeader(t("Optional server key","Volitelný klíč serveru"))){
  ImGui::TextWrapped("%s",t("Leave empty if your server needs no authentication. This key is stored for this address only, separately from OpenAI.","Pokud server nevyžaduje přihlášení, nech prázdné. Klíč se ukládá jen pro tuto adresu, odděleně od OpenAI."));
  ImGui::InputText(t("Bearer key","Bearer klíč"),key,sizeof(key),ImGuiInputTextFlags_Password);
  ImGui::BeginDisabled(!key[0]);if(ImGui::Button(t("Save server key","Uložit klíč serveru"))){if(api.saveKey(s,key)){s.enabled=false;++s.customKeyRevision;}SecureZeroMemory(key,sizeof(key));}ImGui::EndDisabled();
  if(hasKey){ImGui::SameLine();if(ImGui::Button(t("Remove server key","Odstranit klíč serveru"))){api.forgetKey(s);++s.customKeyRevision;s.enabled=false;}}
 }
 ImGui::TextDisabled("%s",hasKey?t("A key is saved for this server.","Pro tento server je uložený klíč."):t("No server key; anonymous connection.","Bez klíče serveru; anonymní spojení."));
 if(ImGui::CollapsingHeader(t("Request parameters","Parametry požadavku"))){
  ImGui::Checkbox(t("Send temperature","Posílat teplotu"),&s.customTemperatureEnabled);if(s.customTemperatureEnabled)ImGui::SliderFloat(t("Temperature","Teplota"),&s.customTemperature,0,2,"%.2f");
  ImGui::SliderInt(t("Output token limit","Limit výstupních tokenů"),&s.customMaxTokens,32,2048);
  ImGui::SliderInt(t("Timeout (seconds)","Časový limit (sekundy)"),&s.customTimeout,5,120);
  field(t("Additional JSON parameters","Další parametry JSON"),s.customParameters,8193,true);
  ImGui::TextWrapped("%s",t("Only send parameters your server supports. For a compatible llama.cpp model, try {\"chat_template_kwargs\":{\"enable_thinking\":false},\"reasoning_budget\":0}. Ollama uses an options object. Model, text, tools, streaming and output limits cannot be replaced here.","Posílej jen parametry podporované serverem. U kompatibilního modelu v llama.cpp zkus {\"chat_template_kwargs\":{\"enable_thinking\":false},\"reasoning_budget\":0}. Ollama používá objekt options. Model, text, nástroje, streamování a výstupní limit zde nelze přepsat."));
  field(t("Custom instruction (optional)","Vlastní instrukce (volitelné)"),s.customInstruction,4097,true);
  ImGui::TextWrapped("%s",t("Blank = the default translation instruction. Available placeholders: {source_language}, {target_language}. Ask for the translated subtitle only.","Prázdné = výchozí překladová instrukce. Zástupné hodnoty: {source_language}, {target_language}. Požaduj jen přeložený titulek."));
 }
 if(!api.error.empty())ImGui::TextWrapped("%s",api.error.c_str());
 bool valid=!s.customModel.empty()&&s.sourceLanguage!=s.targetLanguage;
 try{CustomAiSettings::config(s);}catch(...){valid=false;ImGui::TextWrapped("%s",t("Check the URL and JSON parameter syntax.","Ověř adresu a zápis parametrů JSON."));}
 if(before!=s)s.enabled=false;
 if(ImGui::SmallButton(t("Own server setup guide","Návod k vlastnímu serveru")))ShellExecuteW(window,L"open",czech?L"https://opicebot.cz/framelab/guide/custom-ai.html":L"https://opicebot.cz/framelab/guide/custom-ai-en.html",nullptr,nullptr,SW_SHOWNORMAL);
 return valid;
}
