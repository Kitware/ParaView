/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "WindowsSystemInterface.h"

#include "windows.h"
#include "psapi.h"

#include <iostream>
using std::cerr;
using std::endl;

class WindowsSystemInterface::Implementation
{
public:
  Implementation()
        :
    Pid(-1),
    HProc(0),
    HostName("localhost"),
    MemoryTotal(-1)
      {}

  int Pid;
  HANDLE HProc;
  string HostName;
  unsigned long MemoryTotal;
};

//-----------------------------------------------------------------------------
WindowsSystemInterface::WindowsSystemInterface()
{
  this->impl=new Implementation;

  this->impl->Pid=GetCurrentProcessId();

  MEMORYSTATUSEX statex;
  statex.dwLength=sizeof(statex);
  GlobalMemoryStatusEx(&statex);
  this->impl->MemoryTotal=statex.ullTotalPhys/1024;

  this->impl->HProc=OpenProcess(
      PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,
      false,
      this->impl->Pid);
  if (this->impl->HProc==0)
    {
    cerr
      << "Error: failed to obtain process handle for "
      << this->impl->Pid << "."
      << endl;
    }

  char hostName[1024];
  int iErr=gethostname(hostName,1024);
  if (iErr)
    {
    cerr << "Error: failed to obtain the hostname." << endl;
    this->impl->HostName="localhost";
    }
  else
    {
    this->impl->HostName=hostName;
    }
}

//-----------------------------------------------------------------------------
WindowsSystemInterface::~WindowsSystemInterface()
{
  CloseHandle(this->impl->HProc);
  delete this->impl;
}

//-----------------------------------------------------------------------------
unsigned long WindowsSystemInterface::GetMemoryTotal()
{ 
  return this->impl->MemoryTotal;
}

//-----------------------------------------------------------------------------
unsigned long WindowsSystemInterface::GetMemoryUsed()
{
  PROCESS_MEMORY_COUNTERS pmc;
  int ok=GetProcessMemoryInfo(this->impl->HProc,&pmc,sizeof(pmc));
  if (!ok)
    {
    cerr << "Failed to obtain memory information." << endl;
    return -1;
    }
  return pmc.WorkingSetSize/1024;
}

//-----------------------------------------------------------------------------
int WindowsSystemInterface::GetProcessId()
{ 
  return this->impl->Pid;
}

//-----------------------------------------------------------------------------
string WindowsSystemInterface::GetHostName()
{
  return this->impl->HostName;
}
