/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "LinuxSystemInterface.h"

#include "FsUtils.h"
#include "SQMacros.h"

#include <vector>
using std::vector;
#include <string>
using std::string;
#include <sstream>
using std::istringstream;
#include <iostream>
using std::cerr;
using std::endl;

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
LinuxSystemInterface::LinuxSystemInterface()
      :
    MemoryTotal(0)
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
  ok=NameValue(meminfo,"MemTotal:",this->MemoryTotal);
  if (!ok)
    {
    sqErrorMacro(cerr,"Failed to get the total system memory.");
    return;
    }
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetVmRSS()
{
  return this->GetStatusField("VmRSS:");
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetVmPeak()
{
  return this->GetStatusField("VmPeak:");
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetVmSize()
{
  return this->GetStatusField("VmSize:");
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetVmLock()
{
  return this->GetStatusField("VmLck:");
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetVmHWM()
{
  return this->GetStatusField("VmHWM:");
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetVmData()
{
  return this->GetStatusField("VmData:");
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetVmStack()
{
  return this->GetStatusField("VmStk:");
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetVmExec()
{
  return this->GetStatusField("VmExe:");
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetVmLib()
{
  return this->GetStatusField("VmLib:");
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetVmPTE()
{
  return this->GetStatusField("VmPTE:");
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetVmSwap()
{
  return this->GetStatusField("VmSwap:");
}

//-----------------------------------------------------------------------------
unsigned long LinuxSystemInterface::GetStatusField(const char *name)
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
  unsigned long value;
  ok=NameValue(status,name,value);
  if (!ok)
    {
    sqErrorMacro(cerr,"Failed to find " << name << ".");
    return -1;
    }

  return value;
}
