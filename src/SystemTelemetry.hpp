#pragma once
#include <Windows.h>
#include <psapi.h>
#include <string>
#include <intrin.h>
struct SystemTelemetry {
 double cpu=-1,appCpu=-1,ramUsedMB=-1,ramTotalMB=-1,appRamMB=-1,appPrivateMB=-1;
 unsigned threads=0;std::string cpuName;
 ULONGLONG previousSystem=0,previousIdle=0,previousApp=0;
 static ULONGLONG ticks(FILETIME t){ULARGE_INTEGER n{};n.LowPart=t.dwLowDateTime;n.HighPart=t.dwHighDateTime;return n.QuadPart;}
 SystemTelemetry(){SYSTEM_INFO s{};GetSystemInfo(&s);threads=s.dwNumberOfProcessors;int regs[4];char name[49]{};__cpuid(regs,0x80000000);if(unsigned(regs[0])>=0x80000004)for(int i=0;i<3;++i){__cpuid(regs,0x80000002+i);memcpy(name+i*16,regs,16);}cpuName=name;poll();}
 void poll(){FILETIME idle{},kernel{},user{},created{},exit{},pk{},pu{};
  if(GetSystemTimes(&idle,&kernel,&user)&&GetProcessTimes(GetCurrentProcess(),&created,&exit,&pk,&pu)){
   auto total=ticks(kernel)+ticks(user),idleNow=ticks(idle),app=ticks(pk)+ticks(pu);
   if(previousSystem&&total>previousSystem){double delta=double(total-previousSystem);cpu=100.*(1.-double(idleNow-previousIdle)/delta);appCpu=100.*double(app-previousApp)/delta;}
   previousSystem=total;previousIdle=idleNow;previousApp=app;
  }
  MEMORYSTATUSEX memory{sizeof(memory)};if(GlobalMemoryStatusEx(&memory)){ramTotalMB=memory.ullTotalPhys/1048576.;ramUsedMB=(memory.ullTotalPhys-memory.ullAvailPhys)/1048576.;}
  PROCESS_MEMORY_COUNTERS_EX p{};p.cb=sizeof(p);if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&p),sizeof(p))){appRamMB=p.WorkingSetSize/1048576.;appPrivateMB=p.PrivateUsage/1048576.;}
 }
};
