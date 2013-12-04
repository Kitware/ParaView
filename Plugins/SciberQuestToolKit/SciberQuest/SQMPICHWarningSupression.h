/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef SQMPICHWarningSupression_h
#define SQMPICHWarningSupression_h

// quite some warnings that are introduced by MPICH's mpi.h

#if !defined(__INTEL_COMPILER) && defined(__GNUG__)
#define GCC_VER  (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if GCC_VER > 40200
#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#endif

#endif

// VTK-HeaderTest-Exclude: SQMPICHWarningSupression.h
