/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CPUConvolutionDriver_h
#define CPUConvolutionDriver_h

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
