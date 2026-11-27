#pragma once
#include "CustomAiSettings.hpp"
#include "CustomPromptControls.hpp"
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
 customPromptEditor(s);
 ui::field(t("Tokens per response","Tokenů na odpověď"));
 static int tokenDraft=0,tokenSaved=0;
 if(tokenSaved!=s.customMaxTokens){tokenDraft=tokenSaved=s.customMaxTokens;}
 ImGui::SetNextItemWidth(-1);ImGui::InputInt("##custom-output-tokens",&tokenDraft,100,1000);
 const bool tokenEdited=ImGui::IsItemDeactivatedAfterEdit();
 ImGui::BeginDisabled(tokenDraft<1||tokenDraft==s.customMaxTokens);
 const bool applyTokens=ImGui::Button(t("Apply token limit","Použít limit tokenů"));ImGui::EndDisabled();
 if((tokenEdited||applyTokens)&&tokenDraft>0){s.customMaxTokens=tokenSaved=tokenDraft;}
 ImGui::TextWrapped("%s",t("Maximum generated tokens per response, separate from the model's context capacity. Saved automatically; changes reconnect the translator. The server may enforce its own limit.","Maximum vygenerovaných tokenů na odpověď, oddělené od kapacity kontextu modelu. Ukládá se automaticky; změna znovu připojí překladač. Server může mít vlastní limit."));
 if(tokenDraft<1)ImGui::TextWrapped("%s",t("Enter a positive token count.","Zadej kladný počet tokenů."));
 ui::field(t("Maximum wait per subtitle (seconds)","Nejdelší čekání na titulek (sekundy)"));
 static int waitDraft=0,waitSaved=0;if(waitSaved!=s.customTimeout){waitDraft=waitSaved=s.customTimeout;}
 ImGui::SetNextItemWidth(-1);ImGui::InputInt("##custom-caption-wait",&waitDraft,1,5);
 if(ImGui::IsItemDeactivatedAfterEdit()){s.customTimeout=waitSaved=waitDraft=std::clamp(waitDraft,1,120);}
 ImGui::TextWrapped("%s",t("Includes all speaker lines in one caption. If it takes longer, abandon that answer and continue with the latest subtitle; older queued captions are skipped.","Platí pro všechny řádky mluvčích v jednom titulku dohromady. Při překročení ukončí čekání a vezme nejnovější titulek; staré úlohy ve frontě přeskočí."));
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
  field(t("Additional JSON parameters","Další parametry JSON"),s.customParameters,8193,true);
  ImGui::TextWrapped("%s",t("Only send parameters your server supports. For a compatible llama.cpp model, try {\"chat_template_kwargs\":{\"enable_thinking\":false},\"reasoning_budget\":0}. Ollama uses an options object. Model, text, tools, streaming and output limits cannot be replaced here.","Posílej jen parametry podporované serverem. U kompatibilního modelu v llama.cpp zkus {\"chat_template_kwargs\":{\"enable_thinking\":false},\"reasoning_budget\":0}. Ollama používá objekt options. Model, text, nástroje, streamování a výstupní limit zde nelze přepsat."));
 }
 if(!api.error.empty())ImGui::TextWrapped("%s",api.error.c_str());
 bool valid=!s.customModel.empty()&&s.sourceLanguage!=s.targetLanguage;
 try{CustomAiSettings::config(s);}catch(...){valid=false;ImGui::TextWrapped("%s",t("Check the URL and JSON parameter syntax.","Ověř adresu a zápis parametrů JSON."));}
 auto connectionBefore=before;connectionBefore.customMaxTokens=s.customMaxTokens;connectionBefore.customTimeout=s.customTimeout;
 connectionBefore.customPromptEnabled=s.customPromptEnabled;connectionBefore.customResponseFormat=s.customResponseFormat;connectionBefore.customInstruction=s.customInstruction;
 if(connectionBefore!=s)s.enabled=false;
 if(ImGui::SmallButton(t("Own server setup guide","Návod k vlastnímu serveru")))ShellExecuteW(window,L"open",czech?L"https://opicebot.cz/framelab/guide/custom-ai.html":L"https://opicebot.cz/framelab/guide/custom-ai-en.html",nullptr,nullptr,SW_SHOWNORMAL);
 return valid;
}
