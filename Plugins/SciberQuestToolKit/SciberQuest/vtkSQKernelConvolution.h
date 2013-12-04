/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __vtkSQKernelConvolution_h
#define __vtkSQKernelConvolution_h

#include <set> // for set
#include <string> // for string

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"
#include "CartesianExtent.h" // for CartesianExtent

class vtkPVXMLElement;
class vtkInformation;
class vtkInformationVector;
class CPUConvolutionDriver;
class CUDAConvolutionDriver;

// .DESCRIPTION
//
class VTKSCIBERQUEST_EXPORT vtkSQKernelConvolution : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSQKernelConvolution,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQKernelConvolution *New();

  // Description:
  // Initialize the filter from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Array selection.
  void AddInputArray(const char *name);
  void ClearInputArrays();

  // Description:
  // Deep copy input arrays to the output. A shallow copy is not possible
  // due to the presence of ghost layers.
  void AddArrayToCopy(const char *name);
  void ClearArraysToCopy();

  //BTX
  enum {
    MODE_3D=0,
    MODE_2D_XY,
    MODE_2D_XZ,
    MODE_2D_YZ
    };
  //ETX
  // Description:
  // Set the mode to 2 or 3D.
  void SetMode(int mode);
  vtkGetMacro(Mode,int);

  //BTX
  enum {
    KERNEL_TYPE_GAUSSIAN=0,
    KERNEL_TYPE_LOG=1,
    KERNEL_TYPE_CONSTANT=2
    };
  //ETX
  // Description:
  // Select a kernel for the convolution.
  void SetKernelType(int type);
  vtkGetMacro(KernelType,int);

  // Description:
  // Set the stencil width, must be an odd integer, bound bellow by 3
  // and above by the size of the smallest block
  void SetKernelWidth(int width);
  vtkGetMacro(KernelWidth,int);

  // Description:
  vtkSetMacro(ComputeResidual,int);
  vtkGetMacro(ComputeResidual,int);

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
  vtkSQKernelConvolution();
  virtual ~vtkSQKernelConvolution();

  // Description:
  // Called before execution to generate the selected kernel.
  int UpdateKernel();

private:
  int WorldSize;
  int WorldRank;
  int HostSize;
  int HostRank;
  //
  std::set<std::string> InputArrays;
  std::set<std::string> ArraysToCopy;
  int ComputeResidual;
  //
  int KernelWidth;
  int KernelType;
  CartesianExtent KernelExt;
  float *Kernel;
  int KernelModified;
  //
  int Mode;
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

  int LogLevel;

private:
  vtkSQKernelConvolution(const vtkSQKernelConvolution &); // Not implemented
  void operator=(const vtkSQKernelConvolution &); // Not implemented
};

#endif
