/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/

#ifndef __CPUConvolutionDriver_h
#define __CPUConvolutionDriver_h

class vtkDataArray;
class CartesianExtent;

/// CPUConvolutionDriver - Interface to the  kernel
class CPUConvolutionDriver
{
public:
  ///
  CPUConvolutionDriver();

  /**
  Select the optimizations used.
  */
  enum{
    OPT_NONE=0,
    OPT_FLATTEN_VTK=1,
    OPT_Z_ORDER=2
  };
  void SetOptimization(int opt){ this->Optimization=opt; }
  int GetOptimization(){ return this->Optimization; }

  /// Invoke the kernel
  int Convolution(
      CartesianExtent &extV,
      CartesianExtent &extW,
      CartesianExtent &extK,
      int ghostV,
      int mode,
      vtkDataArray *V,
      vtkDataArray *W,
      float *K);

private:
  int Optimization;
};

#endif

// VTK-HeaderTest-Exclude: CPUConvolutionDriver.h
