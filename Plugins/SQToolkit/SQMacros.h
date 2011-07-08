/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __SQMacros_h
#define __SQMacros_h

#include <iomanip>
using std::setprecision;
using std::setw;
using std::scientific;
#include<iostream>
using std::endl;
using std::cerr;

#define safeio(s) (s?s:"NULL")

#define sqErrorMacro(os,estr)                       \
    os                                              \
      << "Error in:" << endl                        \
      << __FILE__ << ", line " << __LINE__ << endl  \
      << "" estr << endl;

#define SafeDelete(a)\
  if (a)\
    {\
    a->Delete();\
    }


#define DO_PRAGMA(x) _Pragma(#x)
#define sqTODOMacro(x)\
  DO_PRAGMA(message("TODO - "#x))
#endif
