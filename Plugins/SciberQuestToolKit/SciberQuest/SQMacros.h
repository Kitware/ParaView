/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __SQMacros_h
#define __SQMacros_h

#include <iostream> // for endl

#define safeio(s) (s?s:"NULL")

#define sqErrorMacro(os,estr)                            \
    os                                                   \
      << "Error in:" << std::endl                        \
      << __FILE__ << ", line " << __LINE__ << std::endl  \
      << "" estr << std::endl;

#define sqWarningMacro(os,estr)                          \
    os                                                   \
      << "Warning in:" << std::endl                      \
      << __FILE__ << ", line " << __LINE__ << std::endl  \
      << "" estr << std::endl;

#define SafeDelete(a)\
  if (a)\
    {\
    a->Delete();\
    }

// The vtkFloatTemplateMacro is like vtkTemplateMacro but
// expands only to floating point types.

#define vtkFloatTemplateMacroCase(typeN, type, call) \
  case typeN: { typedef type VTK_TT; call; }; break

#define vtkFloatTemplateMacro(call) \
  vtkFloatTemplateMacroCase(VTK_DOUBLE, double, call); \
  vtkFloatTemplateMacroCase(VTK_FLOAT, float, call);

#if defined(WIN32)
// TODO - move this to the file where it ocurs
#pragma warning(disable : 4800) // int to bool performance
#endif
#endif

// VTK-HeaderTest-Exclude: SQMacros.h
