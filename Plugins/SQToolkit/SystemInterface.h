/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __SystemInterface_h
#define __SystemInterface_h

#include "vtkType.h"
#include "SystemType.h"
#include <string>
using std::string;

/// Encapsulates system/os specific operations.
class SystemInterface
{
public:
  SystemInterface(){}
  virtual ~SystemInterface(){}

  /**
  Return the total amount of physical RAM avaiable on the system.
  */
  virtual unsigned long GetMemoryTotal(){ return 0; }

  /**
  Return the amount of physical RAM used by this process.
  */
  virtual unsigned long GetMemoryUsed(){ return 0; }

  /**
  Return the processs identifier of this process.
  */
  virtual int GetProcessId(){ return -1; }

  /**
  Return the hostname.
  */
  virtual string GetHostName(){ return string("localhost"); }

  /**
  Return a code representing the system type.
  */
  virtual int GetSystemType(){ return SYSTEM_TYPE; }

  /**
  Execute the given command in a new process.
  */
  virtual int Exec(string &cmd){ return -1; }

  /**
  If set will print a stack trace for segfault, gp fault, floating point
  exception etc...
  */
  virtual void StackTraceOnError(int ){}

  /**
  Turn on/off floating point exceptions.
  */
  virtual void CatchAllFloatingPointExceptions(int ){}
  virtual void CatchDIVBYZERO(int ){}
  virtual void CatchINEXACT(int ){}
  virtual void CatchINVALID(int ){}
  virtual void CatchOVERFLOW(int ){}
  virtual void CatchUNDERFLOW(int ){}
};

#endif
