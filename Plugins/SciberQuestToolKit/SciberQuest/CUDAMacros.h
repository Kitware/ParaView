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
#include <cuda.h> // standard cuda header

#include <iostream> // for endl

#define CUDAErrorMacro(os,eno,estr)                      \
    os                                                   \
      << "Error in:" << std::endl                        \
      << __FILE__ << ", line " << __LINE__ << std::endl  \
      << cudaGetErrorString(eno) << std::endl            \
      << "" estr << std::endl;
#else
#define CUDAErrorMacro(os,eno,estr)
#endif

#endif

// VTK-HeaderTest-Exclude: CUDAMacros.h
