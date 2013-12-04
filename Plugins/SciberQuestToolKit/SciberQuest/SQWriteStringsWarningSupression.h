
/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef SQWriteStringWarningSupression_h
#define SQWriteStringWarningSupression_h

// disbale warning about passing string literals.
#if !defined(__INTEL_COMPILER) && defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#endif

// VTK-HeaderTest-Exclude: SQWriteStringsWarningSupression.h
