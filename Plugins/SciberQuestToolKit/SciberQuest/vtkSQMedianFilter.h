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
#ifndef vtkSQMedianFilter_h
#define vtkSQMedianFilter_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"
#include "CartesianExtent.h" // for Cartesian extent

class vtkPVXMLElement;
class vtkInformation;
class vtkInformationVector;
//class CPUConvolutionDriver;
//class CUDAConvolutionDriver;

// .DESCRIPTION
//
class VTKSCIBERQUEST_EXPORT vtkSQMedianFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSQMedianFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQMedianFilter *New();


  // Description:
  // Initialize the filter from an xml document.
  int Initialize(vtkPVXMLElement *root);

  enum {
    MODE_3D=0,
    MODE_2D_XY,
    MODE_2D_XZ,
    MODE_2D_YZ
    };

  // Description:
  // Set the mode to 2 or 3D.
  void SetMode(int mode);
  vtkGetMacro(Mode,int);

  enum {
    KERNEL_TYPE_MIN=0,
    KERNEL_TYPE_MEDIAN,
    KERNEL_TYPE_MAX,
    KERNEL_TYPE_MAX_DIFF
    };

  // Description:
  // Select a kernel for the convolution.
  void SetKernelType(int type);
  vtkGetMacro(KernelType,int);

  // Description:
  // Set the stencil width, must be an odd integer, bound bellow by 3
  // and above by the size of the smallest block
  void SetKernelWidth(int width);
  vtkGetMacro(KernelWidth,int);

  /*
  // Description:
  // Query properties of the current device and available devices.
  vtkGetMacro(NumberOfCUDADevices,int);
  vtkGetVector2Macro(CUDADeviceRange,int);

  // Description:
  // Set the number of MPI ranks to use GPUs per host. This is where
  // EnableCUDA and CUDADeviceID are set on a rank by rank basis.
  void SetAllMPIRanksToUseCUDA(int allUse);
  void SetAllMPIRanksToUseCPU(){ this->SetAllMPIRanksToUseCUDA(0); }
  void SetNumberOfMPIRanksToUseCUDA(int nRanks);
  int GetNumberOfMPIRanksToUseCUDA(){ return this->NumberOfMPIRanksToUseCUDA; }

  // Description:
  // Set the number of GPUs to use per host.
  void SetNumberOfActiveCUDADevices(int nActive);
  int GetNumberOfActiveCUDADevices(){ return this->NumberOfActiveCUDADevices; }

  // Description:
  // Set to use CUDA. This is done automatically when
  // SetNumberOfActiveDevices is called.
  vtkSetMacro(EnableCUDA,int);
  vtkGetMacro(EnableCUDA,int);

  // Description:
  // Set the CUDA device id. This is done automatically when
  // SetNumberOfActiveDevices is called.
  int SetCUDADeviceId(int deviceId);
  vtkGetMacro(CUDADeviceId,int);

  // Description:
  // Set the number of warps per CUDA block. This paramter is used
  // to set the block size so that warp size is an even divisor.
  void SetNumberOfWarpsPerCUDABlock(int nWarpsPerBlock);
  int GetNumberOfWarpsPerCUDABlock();

  // Description:
  // Set a flag indicating it's OK to use constant memory
  // for the convolution kernel.
  void SetKernelCUDAMemoryType(int memType);
  int GetKernelCUDAMemoryType();

  // Description:
  // Set a flag indicating it's OK to use texture memory
  // for input arrays.
  void SetInputCUDAMemoryType(int memType);
  int GetInputCUDAMemoryType();

  // Description:
  // Select a set of optimization for code running on the
  // CPU.
  void SetCPUDriverOptimization(int opt);
  int GetCPUDriverOptimization();
  */

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  //int FillInputPortInformation(int port, vtkInformation *info);
  //int FillOutputPortInformation(int port, vtkInformation *info);
  int RequestDataObject(vtkInformation*,vtkInformationVector** inInfoVec,vtkInformationVector* outInfoVec);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestUpdateExtent(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkSQMedianFilter();
  virtual ~vtkSQMedianFilter();

  // Description:
  // Called before execution to generate the selected kernel.
  int UpdateKernel();

private:
  /*
  int WorldSize;
  int WorldRank;
  int HostSize;
  int HostRank;
  */

  //
  int KernelWidth;
  int KernelType;
  CartesianExtent KernelExt;
  int KernelModified;
  //
  int Mode;

  /*
  //
  int NumberOfIterations;
  //
  int NumberOfCUDADevices;
  int CUDADeviceRange[2];
  int NumberOfActiveCUDADevices;
  int CUDADeviceId;
  int NumberOfMPIRanksToUseCUDA;
  int EnableCUDA;
  //
  CPUConvolutionDriver *CPUDriver;
  CUDAConvolutionDriver *CUDADriver;
  */

  int LogLevel;

private:
  vtkSQMedianFilter(const vtkSQMedianFilter &) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQMedianFilter &) VTK_DELETE_FUNCTION;
};

#endif
