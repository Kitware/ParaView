/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef SQEigenWarningSupression_h
#define SQEigenWarningSupression_h

#if !defined(__INTEL_COMPILER) && defined(__GNUG__)
#define GCC_VER  (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if GCC_VER > 40200
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#if GCC_VER > 40400
#pragma GCC diagnostic ignored "-Wenum-compare"
#endif
#endif

#if defined(WIN32)
#pragma warning(disable : 4512) // assignment operator could not be generated
#pragma warning(disable : 4181) // qualifier applied to reference type ignored
#endif

#endif

// VTK-HeaderTest-Exclude: SQEigenWarningSupression.h
