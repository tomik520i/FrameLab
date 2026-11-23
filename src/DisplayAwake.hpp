#pragma once
#include <Windows.h>
// Thread-scoped request, never a power-plan change or simulated input.
class DisplayAwake {
 bool active_=false;EXECUTION_STATE previous_=ES_CONTINUOUS;DWORD error_=0;
public:
 DisplayAwake()=default;DisplayAwake(const DisplayAwake&)=delete;
 ~DisplayAwake(){update(false);}
 bool active()const{return active_;}DWORD error()const{return error_;}
 void update(bool needed){
  if(needed==active_)return;
  auto prior=SetThreadExecutionState(needed?ES_CONTINUOUS|ES_DISPLAY_REQUIRED|ES_SYSTEM_REQUIRED:previous_);
  if(!prior){error_=GetLastError();if(!error_)error_=ERROR_GEN_FAILURE;return;}
  if(needed)previous_=prior;active_=needed;error_=0;
 }
};
