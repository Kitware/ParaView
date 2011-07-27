/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __SystemType_h
#define __SystemType_h

enum SystemType
{
  SYSTEM_TYPE_UNDEFINED=-1,
  SYSTEM_TYPE_APPLE=0,
  SYSTEM_TYPE_WIN=1,
  SYSTEM_TYPE_LINUX=2
};

#if defined(__APPLE__)
  #define SYSTEM_TYPE SYSTEM_TYPE_APPLE
  class OSXSystemInterface;
  typedef OSXSystemInterface CurrentSystemInterface;

#elif defined(_WIN32)
  #define SYSTEM_TYPE SYSTEM_TYPE_WIN
  class WindowsSystemInterface;
  typedef WindowsSystemInterface CurrentSystemInterface;

#else
  #define SYSTEM_TYPE SYSTEM_TYPE_LINUX
  class LinuxSystemInterface;
  typedef LinuxSystemInterface CurrentSystemInterface;

#endif

#endif
