#pragma once
#include <dxgi.h>
#include <deque>
#include <optional>
#include <string>
#include <cstdint>

// A DXGI PresentCount is a present ID, not the number of unique displayed images.
// Resolve observed OS confirmations to our content IDs; never infer skipped IDs.
struct DisplayVerification {
 struct Submission {UINT present=0;bool generated=false;uint64_t image=0;};
 struct Confirmation {Submission submission;UINT refresh=0,syncRefresh=0;int64_t syncQpc=0;};
 std::deque<Submission> ledger;
 UINT lastConfirmed=0;bool haveConfirmed=false,available=false,haveImage=false;
 bool lastGenerated=false;uint64_t lastImage=0,content=0,generated=0,unresolved=0;
 HRESULT status=DXGI_ERROR_FRAME_STATISTICS_DISJOINT;
 unsigned displayHz=0,displayWidth=0,displayHeight=0;
 HMONITOR monitor=nullptr;
 void clearHistory(){ledger.clear();haveConfirmed=haveImage=available=false;}
 void submitted(IDXGISwapChain* swap,bool isGenerated,uint64_t image){
  UINT id=0;if(FAILED(swap->GetLastPresentCount(&id)))return;
  record({id,isGenerated,image});
 }
 void record(Submission s){ledger.push_back(s);if(ledger.size()>4096)ledger.pop_front();}
 std::optional<Confirmation> observe(IDXGISwapChain* swap){
  DXGI_FRAME_STATISTICS f{};status=swap->GetFrameStatistics(&f);
  if(FAILED(status)){available=false;if(status==DXGI_ERROR_FRAME_STATISTICS_DISJOINT)clearHistory();return {};}
  return accept(f);
 }
 std::optional<Confirmation> accept(const DXGI_FRAME_STATISTICS& f){
  available=true;if(!f.PresentCount||(haveConfirmed&&f.PresentCount==lastConfirmed))return {};
  lastConfirmed=f.PresentCount;haveConfirmed=true;
  for(auto i=ledger.rbegin();i!=ledger.rend();++i)if(i->present==f.PresentCount){
   if(haveImage&&lastImage==i->image&&lastGenerated==i->generated)return {};
   haveImage=true;lastImage=i->image;lastGenerated=i->generated;++content;if(i->generated)++generated;
   return Confirmation{*i,f.PresentRefreshCount,f.SyncRefreshCount,f.SyncQPCTime.QuadPart};
  }
  ++unresolved;return {};
 }
 void updateMonitor(IDXGISwapChain* swap){
  IDXGIOutput* output=nullptr;displayHz=displayWidth=displayHeight=0;
  if(FAILED(swap->GetContainingOutput(&output)))return;
  DXGI_OUTPUT_DESC desc{};HRESULT hr=output->GetDesc(&desc);output->Release();if(FAILED(hr))return;
  if(monitor!=desc.Monitor){monitor=desc.Monitor;clearHistory();}
  DEVMODEW mode{};mode.dmSize=sizeof(mode);
  if(EnumDisplaySettingsW(desc.DeviceName,ENUM_CURRENT_SETTINGS,&mode)){displayHz=mode.dmDisplayFrequency>1?mode.dmDisplayFrequency:0;displayWidth=mode.dmPelsWidth;displayHeight=mode.dmPelsHeight;}
 }
};
