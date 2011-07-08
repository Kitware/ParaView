/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __UnixSystemInterface_h
#define __UnixSystemInterface_h

#include "SystemInterface.h"

#include <string>
using std::string;

/// Linux interface.
class UnixSystemInterface : public SystemInterface
{
public:
  UnixSystemInterface();
  virtual ~UnixSystemInterface(){}

  /**
  Return the processs identifier of this process.
  */
  virtual int GetProcessId(){ return this->Pid; }

  /**
  Return the hostname.
  */
  virtual string GetHostName();

  /**
  Execute the given command in a new process.
  */
  virtual int Exec(string &cmd);

  /**
  If set will print a stack trace for segfault, gp fault, floating point
  exception etc...
  */
  virtual void StackTraceOnError(int enable);

  /**
  Turn on/off floating point exceptions.
  */
  virtual void CatchAllFloatingPointExceptions(int enable);
  virtual void CatchDIVBYZERO(int enable);
  virtual void CatchINEXACT(int enable);
  virtual void CatchINVALID(int enable);
  virtual void CatchOVERFLOW(int enable);
  virtual void CatchUNDERFLOW(int enable);

private:
  int Pid;
};

#endif
