/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __SQPosixOnWindows_h
#define __SQPosixOnWindows_h

#if defined(WIN32)

// imitate posix_memalign on windows.
#include <malloc.h>
static
inline void posix_memalign(void **pData, size_t alignAt, size_t nBytes)
{
  *pData=_aligned_malloc(nBytes,alignAt);
}

#define __restrict__ __restrict

#endif

#endif

// VTK-HeaderTest-Exclude: SQPosixOnWindows.h
