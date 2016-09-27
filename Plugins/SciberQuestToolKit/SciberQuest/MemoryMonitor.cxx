/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "MemoryMonitor.h"

#include "FsUtils.h"
#include "SQMacros.h"

#include <vector>
#include <string>


//-----------------------------------------------------------------------------
MemoryMonitor::MemoryMonitor()
      :
    SystemTotal(0)
{
  // get the total system memory
  int ok;
  std::vector<std::string> meminfo;
  ok=(int)LoadLines("/proc/meminfo",meminfo);
  if (!ok)
    {
    sqErrorMacro(std::cerr,"Failed to open /proc/meminfo.");
    return;
    }
  ok=NameValue(meminfo,"MemTotal:",this->SystemTotal);
  if (!ok)
    {
    sqErrorMacro(std::cerr,"Failed to get the total system memory.");
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
  std::vector<std::string> status;
  ok=(int)LoadLines("/proc/self/status",status);
  if (!ok)
    {
    sqErrorMacro(std::cerr,"Failed to open /proc/self/status.");
    return static_cast<unsigned long long>(-1);
    }
  unsigned long long value;
  ok=NameValue(status,name,value);
  if (!ok)
    {
    sqErrorMacro(std::cerr,"Failed to find " << name << ".");
    return static_cast<unsigned long long>(-1);
    }
  return value;
}
