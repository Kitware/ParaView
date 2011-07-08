/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __LinuxSystemInterface_h
#define __LinuxSystemInterface_h

#include "UnixSystemInterface.h"

/// Linux interface.
class LinuxSystemInterface : public UnixSystemInterface
{
public:
  LinuxSystemInterface();
  virtual ~LinuxSystemInterface(){}

  /**
  Return the total amount of physical RAM avaiable on the system.
  */
  virtual unsigned long GetMemoryTotal(){ return this->MemoryTotal; }

  /**
  Return the amount of physical RAM used by this process.
  */
  virtual unsigned long GetMemoryUsed(){ return this->GetVmRSS(); }

  /**
  More detailed information specific to linux.
  */
  unsigned long GetVmRSS();
  unsigned long GetVmPeak();
  unsigned long GetVmSize();
  unsigned long GetVmLock();
  unsigned long GetVmHWM();
  unsigned long GetVmData();
  unsigned long GetVmStack();
  unsigned long GetVmExec();
  unsigned long GetVmLib();
  unsigned long GetVmPTE();
  unsigned long GetVmSwap();

private:
  unsigned long GetStatusField(const char *name);

private:
  unsigned long MemoryTotal;
};

#endif
