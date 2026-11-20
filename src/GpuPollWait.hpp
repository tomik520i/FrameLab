#pragma once
#include <Windows.h>

// Sleep(1) may become a ~16 ms wait for an occluded/invisible Windows 11
// window, even with timeBeginPeriod. Poll completion without busy-spinning.
// https://learn.microsoft.com/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw
class GpuPollWait {
 HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_MODIFY_STATE|SYNCHRONIZE);
public:
 GpuPollWait()=default;GpuPollWait(const GpuPollWait&)=delete;GpuPollWait& operator=(const GpuPollWait&)=delete;
 ~GpuPollWait(){if(timer)CloseHandle(timer);}
 void wait(){LARGE_INTEGER due{};due.QuadPart=-10000; // relative 1 ms, in 100 ns units
  if(timer&&SetWaitableTimerEx(timer,&due,0,nullptr,nullptr,nullptr,0)&&WaitForSingleObject(timer,50)==WAIT_OBJECT_0)return;
  Sleep(1);
 }
};
