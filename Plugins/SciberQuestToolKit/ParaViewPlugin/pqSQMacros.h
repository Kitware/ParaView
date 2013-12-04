/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __pqSQMacros_h
#define __pqSQMacros_h

#define pqSQErrorMacro(os,estr)                    \
    os                                             \
      << "Error in:" << endl                       \
      << __FILE__ << ", line " << __LINE__ << endl \
      << "" estr;

#define pqSQWarningMacro(os,estr)                   \
    os                                              \
      << "Warning in:" << endl                      \
      << __FILE__ << ", line " << __LINE__ << endl  \
      << "" estr;

#endif
