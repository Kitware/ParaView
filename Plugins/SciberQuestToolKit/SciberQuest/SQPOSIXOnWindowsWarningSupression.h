/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef SQPOSIXOnWindowsWarningSupression_h
#define SQPOSIXOnWindowsWarningSupression_h

#if defined(WIN32)
// The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name
#pragma warning(disable : 4996)
#endif

#endif

// VTK-HeaderTest-Exclude: SQPOSIXOnWindowsWarningSupression.h
