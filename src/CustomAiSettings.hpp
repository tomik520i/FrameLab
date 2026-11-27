#pragma once
#include "OpenAiSettings.hpp"
#include "SubtitleTranslator.hpp"

// Server credentials live in a distinct, endpoint-bound Windows vault entry.
// Changing the endpoint can never forward the OpenAI account key.
class CustomAiSettings {
 struct Internet {HINTERNET h=nullptr;~Internet(){if(h)WinHttpCloseHandle(h);}operator HINTERNET()const{return h;}};
 std::mutex mutex;std::thread worker;std::atomic<bool> busy_=false;
 std::vector<std::string> models_;std::string fetched_,status_,keyTarget_;int keyState_=-1;
 static std::wstring wide(const std::string& s){return std::wstring(winrt::to_hstring(s));}
 static std::string secret(const std::string& target){
  PCREDENTIALW p=nullptr;auto name=wide(target);
  if(!CredReadW(name.c_str(),CRED_TYPE_GENERIC,0,&p)){if(GetLastError()==ERROR_NOT_FOUND)return {};throw std::runtime_error("Cannot read custom server credential.");}
  std::string key(reinterpret_cast<char*>(p->CredentialBlob),p->CredentialBlobSize);
  SecureZeroMemory(p->CredentialBlob,p->CredentialBlobSize);CredFree(p);
  if(key.empty()||key.size()>4096||std::any_of(key.begin(),key.end(),[](unsigned char c){return c<33||c>126;})){SecureZeroMemory(key.data(),key.size());throw std::runtime_error("Invalid custom server key.");}
  return key;
 }
 void fetch(std::string address,std::string protocol){
  try{
   winrt::init_apartment(winrt::apartment_type::multi_threaded);struct Apartment{~Apartment(){winrt::uninit_apartment();}} apartment;
   const auto normalized=normalize(address,protocol);auto url=wide(normalized);URL_COMPONENTS parts{};parts.dwStructSize=sizeof(parts);parts.dwHostNameLength=parts.dwUrlPathLength=DWORD(-1);
   if(!WinHttpCrackUrl(url.c_str(),DWORD(url.size()),0,&parts))throw std::runtime_error("Invalid custom server address.");
   std::wstring host(parts.lpszHostName,parts.dwHostNameLength),path=parts.dwUrlPathLength?std::wstring(parts.lpszUrlPath,parts.dwUrlPathLength):std::wstring{};path+=protocol=="ollama"?L"/api/tags":L"/models";
   Internet session{WinHttpOpen(L"FrameLab-custom-subtitles/1",WINHTTP_ACCESS_TYPE_NO_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
   if(!session.h)throw std::runtime_error("Cannot initialize server connection.");WinHttpSetTimeouts(session,8000,8000,8000,8000);
   Internet connection{WinHttpConnect(session,host.c_str(),parts.nPort,0)};
   Internet request{connection.h?WinHttpOpenRequest(connection,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,parts.nScheme==INTERNET_SCHEME_HTTPS?WINHTTP_FLAG_SECURE:0):nullptr};
   auto key=secret("FrameLab/CustomAI/"+normalized);std::wstring header=key.empty()?L"":L"Authorization: Bearer "+wide(key);SecureZeroMemory(key.data(),key.size());
   struct Guard{std::wstring& h;~Guard(){SecureZeroMemory(h.data(),h.size()*sizeof(wchar_t));}} guard{header};
   DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
   if(!request.h||!WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects))||!WinHttpSendRequest(request,header.c_str(),DWORD(header.size()),nullptr,0,0,0)||!WinHttpReceiveResponse(request,nullptr))throw std::runtime_error("Server network/TLS error. Check the address and firewall.");
   DWORD status=0,size=sizeof(status);WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX);
   if(status!=200)throw std::runtime_error("Custom server HTTP "+std::to_string(status)+". Check the URL and optional key.");
   std::string body;char buffer[8192];DWORD got=0;
   for(;;){if(!WinHttpReadData(request,buffer,sizeof(buffer),&got))throw std::runtime_error("Incomplete model list.");if(!got)break;body.append(buffer,got);if(body.size()>2*1024*1024)throw std::runtime_error("Model list is too large.");}
   auto json=winrt::Windows::Data::Json::JsonObject::Parse(winrt::to_hstring(body));std::vector<std::string> values;
   for(auto item:json.GetNamedArray(protocol=="ollama"?L"models":L"data")){
    auto object=item.GetObject();auto id=winrt::to_string(object.GetNamedString(protocol=="ollama"?L"name":L"id",L""));
    if(!id.empty()&&id.size()<=1024&&std::none_of(id.begin(),id.end(),[](unsigned char c){return c<32;}))values.push_back(id);
   }
   std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end()),values.end());
   std::lock_guard lock(mutex);models_=std::move(values);fetched_=normalized+"|"+protocol;status_="Server connected. Models refreshed.";
  }catch(const winrt::hresult_error&){std::lock_guard lock(mutex);status_="Server returned an unsupported model list. You can enter the model ID manually.";}
   catch(const std::exception& e){std::lock_guard lock(mutex);status_=e.what();}
  busy_=false;
 }
public:
 std::string error;
 ~CustomAiSettings(){if(worker.joinable())worker.join();}
 static std::string normalize(std::string value,const std::string& protocol){
  const auto first=value.find_first_not_of(" \t\r\n"),last=value.find_last_not_of(" \t\r\n");value=first==std::string::npos?"":value.substr(first,last-first+1);
  while(!value.empty()&&value.back()=='/')value.pop_back();
  if(value.empty()||value.size()>2048||value.find_first_of("?#")!=std::string::npos||std::any_of(value.begin(),value.end(),[](unsigned char c){return c<33;}))throw std::runtime_error("Enter a server base URL without query, fragment or credentials.");
  if(protocol!="openai"&&protocol!="ollama")throw std::runtime_error("Unknown custom server protocol.");
  auto url=wide(value);URL_COMPONENTS parts{};parts.dwStructSize=sizeof(parts);parts.dwHostNameLength=parts.dwUrlPathLength=parts.dwUserNameLength=parts.dwPasswordLength=DWORD(-1);
  if(!WinHttpCrackUrl(url.c_str(),DWORD(url.size()),0,&parts)||!parts.dwHostNameLength||parts.dwUserNameLength||parts.dwPasswordLength||(parts.nScheme!=INTERNET_SCHEME_HTTP&&parts.nScheme!=INTERNET_SCHEME_HTTPS))throw std::runtime_error("Use http:// or https:// and a valid server address.");
  if(protocol=="openai"&&!parts.dwUrlPathLength)value+="/v1";
  return value;
 }
 static std::string config(const SubtitleSettings& s){
  using namespace winrt::Windows::Data::Json;
  auto extra=JsonObject::Parse(winrt::to_hstring(s.customParameters));JsonObject value;
  value.Insert(L"url",JsonValue::CreateStringValue(winrt::to_hstring(normalize(s.customUrl,s.customProtocol))));
  value.Insert(L"protocol",JsonValue::CreateStringValue(winrt::to_hstring(s.customProtocol)));
  value.Insert(L"model",JsonValue::CreateStringValue(winrt::to_hstring(s.customModel)));
  value.Insert(L"timeout_s",JsonValue::CreateNumberValue(s.customTimeout));value.Insert(L"max_tokens",JsonValue::CreateNumberValue(s.customMaxTokens));
  if(s.customTemperatureEnabled)value.Insert(L"temperature",JsonValue::CreateNumberValue(s.customTemperature));
  value.Insert(L"parameters",extra);value.Insert(L"instruction",JsonValue::CreateStringValue(winrt::to_hstring(s.customInstruction)));
  value.Insert(L"prompt_enabled",JsonValue::CreateBooleanValue(s.customPromptEnabled));
  value.Insert(L"response_format",JsonValue::CreateStringValue(s.customResponseFormat==1?L"translation_tags":L"plain"));
  return winrt::to_string(value.Stringify());
 }
 bool hasKey(const SubtitleSettings& s){
  try{auto target="FrameLab/CustomAI/"+normalize(s.customUrl,s.customProtocol);if(target==keyTarget_&&keyState_>=0)return keyState_!=0;keyTarget_=target;auto key=secret(target);keyState_=key.empty()?0:1;SecureZeroMemory(key.data(),key.size());return keyState_!=0;}catch(...){return false;}
 }
 bool saveKey(const SubtitleSettings& s,const char* key){
  try{const size_t n=strlen(key);if(!n||n>4096||std::any_of(key,key+n,[](unsigned char c){return c<33||c>126;}))throw std::runtime_error("Invalid bearer key format.");
   auto target=wide("FrameLab/CustomAI/"+normalize(s.customUrl,s.customProtocol));CREDENTIALW c{};c.Type=CRED_TYPE_GENERIC;c.TargetName=target.data();c.CredentialBlobSize=DWORD(n);c.CredentialBlob=reinterpret_cast<LPBYTE>(const_cast<char*>(key));c.Persist=CRED_PERSIST_LOCAL_MACHINE;c.UserName=const_cast<wchar_t*>(L"FrameLab");
   if(!CredWriteW(&c,0))throw std::runtime_error("Windows could not store the server key.");keyState_=-1;error.clear();return true;
  }catch(const std::exception& e){error=e.what();return false;}
 }
 void forgetKey(const SubtitleSettings& s){try{auto target=wide("FrameLab/CustomAI/"+normalize(s.customUrl,s.customProtocol));if(!CredDeleteW(target.c_str(),CRED_TYPE_GENERIC,0)&&GetLastError()!=ERROR_NOT_FOUND)throw std::runtime_error("Windows could not remove the server key.");keyState_=-1;error.clear();}catch(const std::exception& e){error=e.what();}}
 void refresh(const SubtitleSettings& s){if(busy_)return;if(worker.joinable())worker.join();busy_=true;worker=std::thread([this,url=s.customUrl,protocol=s.customProtocol]{fetch(url,protocol);});}
 bool busy()const{return busy_;}
 std::vector<std::string> models(const SubtitleSettings& s){std::lock_guard lock(mutex);try{return fetched_==normalize(s.customUrl,s.customProtocol)+"|"+s.customProtocol?models_:std::vector<std::string>{};}catch(...){return {};}}
 std::string status(){std::lock_guard lock(mutex);return status_;}
};
