#pragma once
#include <Windows.h>
#include <wincred.h>
#include <winhttp.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Data.Json.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"winhttp.lib")

// One current-user credential shared by FrameLab installations. Never in ZIPs,
// preferences, request logs, process arguments or the public model catalog.
inline constexpr wchar_t openAiCredential[]=L"FrameLab/OpenAI/APIKey";
struct OpenAiProfile {
 std::string id,label,defaultEffort;std::vector<std::string> efforts,temperatureEfforts,snapshots;
 double inputPrice=-1,outputPrice=-1;
 bool matches(const std::string& model)const{return id==model||std::find(snapshots.begin(),snapshots.end(),model)!=snapshots.end();}
 bool temperature(const std::string& effort)const{return std::find(temperatureEfforts.begin(),temperatureEfforts.end(),effort)!=temperatureEfforts.end();}
};
class OpenAiSettings {
 struct Internet {HINTERNET h=nullptr;~Internet(){if(h)WinHttpCloseHandle(h);}operator HINTERNET()const{return h;}};
 std::mutex mutex;std::thread worker;std::atomic<bool> busy_=false;std::vector<std::string> available_;std::string status_;bool refreshed_=false;mutable int keyState_=-1;
 static std::vector<std::string> strings(const winrt::Windows::Data::Json::JsonObject& object,const wchar_t* key){
  std::vector<std::string> values;if(object.HasKey(key))for(auto value:object.GetNamedArray(key))values.push_back(winrt::to_string(value.GetString()));return values;
 }
 void fetch(){
  try{
   winrt::init_apartment(winrt::apartment_type::multi_threaded);
   struct Apartment{~Apartment(){winrt::uninit_apartment();}} apartment;
   PCREDENTIALW credential=nullptr;
   if(!CredReadW(openAiCredential,CRED_TYPE_GENERIC,0,&credential))throw std::runtime_error("Save an API key first.");
   struct CredentialGuard{PCREDENTIALW p;~CredentialGuard(){if(p){SecureZeroMemory(p->CredentialBlob,p->CredentialBlobSize);CredFree(p);}}} guard{credential};
   std::string secret(reinterpret_cast<char*>(credential->CredentialBlob),credential->CredentialBlobSize);
   if(secret.empty()||secret.size()>512||secret.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=std::string::npos){SecureZeroMemory(secret.data(),secret.size());throw std::runtime_error("Invalid stored key. Save it again.");}
   std::wstring header=L"Authorization: Bearer "+std::wstring(secret.begin(),secret.end());SecureZeroMemory(secret.data(),secret.size());
   struct HeaderGuard{std::wstring& h;~HeaderGuard(){SecureZeroMemory(h.data(),h.size()*sizeof(wchar_t));}} headerGuard{header};
   Internet session{WinHttpOpen(L"FrameLab-subtitles/1",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
   if(!session.h)throw std::runtime_error("Cannot initialize API connection.");
   WinHttpSetTimeouts(session,8000,8000,8000,8000);
   Internet connection{WinHttpConnect(session,L"api.openai.com",INTERNET_DEFAULT_HTTPS_PORT,0)};
   Internet request{connection.h?WinHttpOpenRequest(connection,L"GET",L"/v1/models",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr};
   DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
   if(!request.h||!WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects))||!WinHttpSendRequest(request,header.c_str(),DWORD(header.size()),nullptr,0,0,0)||!WinHttpReceiveResponse(request,nullptr))throw std::runtime_error("API network/TLS error. Try Refresh models again.");
   DWORD status=0,size=sizeof(status);WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX);
   if(status!=200)throw std::runtime_error("API HTTP "+std::to_string(status)+". Check key, project access and billing.");
   std::string body;char chunk[8192];DWORD got=0;
   for(;;){if(!WinHttpReadData(request,chunk,sizeof(chunk),&got))throw std::runtime_error("Incomplete model list.");if(!got)break;body.append(chunk,got);if(body.size()>2*1024*1024)throw std::runtime_error("Model list too large.");}
   auto json=winrt::Windows::Data::Json::JsonObject::Parse(winrt::to_hstring(body));std::vector<std::string> values;
   SYSTEMTIME utc{};GetSystemTime(&utc);char today[16];snprintf(today,sizeof(today),"%04u-%02u-%02u",utc.wYear,utc.wMonth,utc.wDay);
   for(auto item:json.GetNamedArray(L"data")){auto value=item.GetObject();auto id=winrt::to_string(value.GetNamedString(L"id",L""));
    if(value.HasKey(L"shutdown_date")&&value.GetNamedValue(L"shutdown_date").ValueType()==winrt::Windows::Data::Json::JsonValueType::String&&winrt::to_string(value.GetNamedString(L"shutdown_date"))<=today)continue;
    if(id.size()<=160&&!id.empty()&&id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.:-")==std::string::npos)values.push_back(id);
   }
   std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end()),values.end());
   {std::lock_guard lock(mutex);available_=std::move(values);refreshed_=true;status_="Model list refreshed from your API project.";}
  }catch(const winrt::hresult_error&){std::lock_guard lock(mutex);status_="Unable to read API model list.";}
   catch(const std::exception& e){std::lock_guard lock(mutex);status_=e.what();}
  busy_=false;
 }
public:
 std::vector<OpenAiProfile> profiles;std::string reviewed,error;
 explicit OpenAiSettings(const std::filesystem::path& app){
  try{std::ifstream f(app/L"translation"/L"openai-model-profiles.json");std::string data((std::istreambuf_iterator<char>(f)),{});auto json=winrt::Windows::Data::Json::JsonObject::Parse(winrt::to_hstring(data));reviewed=winrt::to_string(json.GetNamedString(L"reviewed"));
   for(auto row:json.GetNamedArray(L"profiles")){auto j=row.GetObject();OpenAiProfile p;p.id=winrt::to_string(j.GetNamedString(L"id"));p.label=winrt::to_string(j.GetNamedString(L"label"));p.defaultEffort=winrt::to_string(j.GetNamedString(L"default_effort"));p.efforts=strings(j,L"efforts");p.temperatureEfforts=strings(j,L"temperature_efforts");p.snapshots=strings(j,L"snapshots");p.inputPrice=j.GetNamedNumber(L"input_usd",-1);p.outputPrice=j.GetNamedNumber(L"output_usd",-1);profiles.push_back(std::move(p));}
  }catch(...){error="OpenAI capability profiles missing or invalid. Extract the complete package.";}
 }
 ~OpenAiSettings(){if(worker.joinable())worker.join();}
 const OpenAiProfile* profile(const std::string& id)const{for(auto& p:profiles)if(p.matches(id))return &p;return nullptr;}
 bool hasKey()const{if(keyState_>=0)return keyState_!=0;PCREDENTIALW p=nullptr;if(!CredReadW(openAiCredential,CRED_TYPE_GENERIC,0,&p)){keyState_=0;return false;}const bool valid=p->CredentialBlobSize>0;SecureZeroMemory(p->CredentialBlob,p->CredentialBlobSize);CredFree(p);keyState_=valid?1:0;return valid;}
 bool saveKey(const char* key){
  const size_t n=strlen(key);if(n<20||n>512||strspn(key,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=n){error="Invalid key format.";return false;}
  CREDENTIALW c{};c.Type=CRED_TYPE_GENERIC;c.TargetName=const_cast<wchar_t*>(openAiCredential);c.CredentialBlobSize=DWORD(n);c.CredentialBlob=reinterpret_cast<LPBYTE>(const_cast<char*>(key));c.Persist=CRED_PERSIST_LOCAL_MACHINE;c.UserName=const_cast<wchar_t*>(L"FrameLab");
  if(!CredWriteW(&c,0)){error="Windows could not save the key.";return false;}error.clear();keyState_=1;return true;
 }
 void forgetKey(){if(!CredDeleteW(openAiCredential,CRED_TYPE_GENERIC,0)&&GetLastError()!=ERROR_NOT_FOUND)error="Windows could not remove the key.";else{error.clear();keyState_=0;std::lock_guard lock(mutex);available_.clear();refreshed_=false;}}
 void refresh(){if(busy_)return;if(worker.joinable())worker.join();busy_=true;worker=std::thread([this]{fetch();});}
 bool busy()const{return busy_;}
 std::vector<std::string> models(){std::lock_guard lock(mutex);return available_;}
 std::string status(){std::lock_guard lock(mutex);return status_;}
 bool available(const std::string& id){std::lock_guard lock(mutex);return refreshed_&&std::binary_search(available_.begin(),available_.end(),id);}
};
