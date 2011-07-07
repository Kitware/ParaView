/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#ifndef __MemoryMonitor_h
#define __MemoryMonitor_h

class MemoryMonitor
{
public:
  MemoryMonitor();

  unsigned long long GetSystemTotal(){ return this->SystemTotal; }

  float GetVmRSSPercent();

  unsigned long long GetVmRSS();
  unsigned long long GetVmPeak();
  unsigned long long GetVmSize();
  unsigned long long GetVmLock();
  unsigned long long GetVmHWM();
  unsigned long long GetVmData();
  unsigned long long GetVmStack();
  unsigned long long GetVmExec();
  unsigned long long GetVmLib();
  unsigned long long GetVmPTE();
  unsigned long long GetVmSwap();

private:
  unsigned long long GetStatusField(const char *name);

private:
  unsigned long long SystemTotal;
};

#endif
