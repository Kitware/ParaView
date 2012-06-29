/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __CUDAMacros_h
#define __CUDAMacros_h

#if defined SQTK_CUDA
#include <cuda.h>

#include <iomanip>
using std::setprecision;
using std::setw;
using std::scientific;
#include<iostream>
using std::endl;
using std::cerr;

#define CUDAErrorMacro(os,eno,estr)                 \
    os                                              \
      << "Error in:" << endl                        \
      << __FILE__ << ", line " << __LINE__ << endl  \
      << cudaGetErrorString(eno) << endl            \
      << "" estr << endl;
#else
#define CUDAErrorMacro(os,eno,estr)
#endif

#endif
