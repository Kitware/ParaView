/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "MemoryMonitor.h"

#include "FsUtils.h"
#include "SQMacros.h"

#include <vector>
using std::vector;
#include <string>
using std::string;


//-----------------------------------------------------------------------------
MemoryMonitor::MemoryMonitor()
      :
    SystemTotal(0)
{
  // get the total system memory
  int ok;
  vector<string> meminfo;
  ok=LoadLines("/proc/meminfo",meminfo);
  if (!ok)
    {
    sqErrorMacro(cerr,"Failed to open /proc/meminfo.");
    return;
    }
  ok=NameValue(meminfo,"MemTotal:",this->SystemTotal);
  if (!ok)
    {
    sqErrorMacro(cerr,"Failed to get the total system memory.");
    return;
    }
}

//-----------------------------------------------------------------------------
float MemoryMonitor::GetVmRSSPercent()
{
   return (float)this->GetVmRSS()/(float)this->GetSystemTotal();
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetVmRSS()
{
  return this->GetStatusField("VmRSS:");
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetVmPeak()
{
  return this->GetStatusField("VmPeak:");
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetVmSize()
{
  return this->GetStatusField("VmSize:");
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetVmLock()
{
  return this->GetStatusField("VmLck:");
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetVmHWM()
{
  return this->GetStatusField("VmHWM:");
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetVmData()
{
  return this->GetStatusField("VmData:");
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetVmStack()
{
  return this->GetStatusField("VmStk:");
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetVmExec()
{
  return this->GetStatusField("VmExe:");
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetVmLib()
{
  return this->GetStatusField("VmLib:");
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetVmPTE()
{
  return this->GetStatusField("VmPTE:");
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetVmSwap()
{
  return this->GetStatusField("VmSwap:");
}

//-----------------------------------------------------------------------------
unsigned long long MemoryMonitor::GetStatusField(const char *name)
{
  // load a fresh copy of /proc/self/status get the value from name
  // value pairs there in
  int ok;
  vector<string> status;
  ok=LoadLines("/proc/self/status",status);
  if (!ok)
    {
    sqErrorMacro(cerr,"Failed to open /proc/self/status.");
    return -1;
    }
  unsigned long long value;
  ok=NameValue(status,name,value);
  if (!ok)
    {
    sqErrorMacro(cerr,"Failed to find " << name << ".");
    return -1;
    }

  return value;
}