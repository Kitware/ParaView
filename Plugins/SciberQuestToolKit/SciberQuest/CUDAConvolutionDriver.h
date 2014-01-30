/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/

#ifndef __CUDAConvolutionDriver_h
#define __CUDAConvolutionDriver_h

#include "vtkSciberQuestModule.h" // for export
#include <algorithm> // for min

class vtkDataArray;
class CartesianExtent;

/// CUDAConvolutionDriver - Interface to the CUDA kernel
class VTKSCIBERQUEST_EXPORT CUDAConvolutionDriver
{
public:
  ///
  CUDAConvolutionDriver();

  /**
  Select a GPU by CUDA device id.
  */
  int SetDeviceId(int deviceId);
  unsigned int GetDeviceId(){ return this->DeviceId; }

  /**
  Query the number of CUDA device installed on this
  system.
  */
  unsigned int GetNumberOfDevices(){ return this->NDevices; }

  /**
  Query device limits for the selected device.
  */
  unsigned int GetMaxNumberOfThreads(){ return this->MaxThreads; }
  unsigned int GetMaxNumberOfBlocks(){ return this->MaxBlocks; }

  ///
  unsigned int GetNumberOfThreads(){ return this->NThreads; }
  void SetNumberOfThreads(unsigned int nThreads)
  {
    this->NThreads=std::min(nThreads,this->MaxThreads);
  }
  ///
  unsigned int GetNumberOfBlocks(){ return this->NBlocks; }
  void SetNumberOfBlocks(unsigned int nBlocks)
  {
    this->NBlocks=std::min(nBlocks,this->MaxBlocks);
  }

  /**
  Set the work per block that determines the data partitioning on
  the GPU. More warps per block leads to larger blocks.
  */
  unsigned int GetNumberOfWarpsPerBlock(){ return this->WarpsPerBlock; }
  void SetNumberOfWarpsPerBlock(unsigned int warpsPerBlock)
  {
    this->WarpsPerBlock=std::min(warpsPerBlock,this->MaxWarpsPerBlock);
  }

  /**
  Set the memory type to be used to store the kernel on the GPU.
  Can be one of the followinig: CUDA_MEM_TYPE_GLOBAL, CUDA_MEM_TYPE_CONST,
  CUDA_MEM_TYPE_TEX. Note that constant memory is limited to 64kB and
  thus can only be used up to some kernel fixed kernel width. If constant
  memory is selected and the kernel is too large to fit in constant memory
  global memory will be used.
  */
  enum
  {
    CUDA_MEM_TYPE_GLOBAL=0,
    CUDA_MEM_TYPE_CONST=1,
    CUDA_MEM_TYPE_TEX=2
  };
  void SetKernelMemoryType(int memType){ this->KernelMemoryType=memType; }
  void SetKernelMemoryTypeToGlobal(){ this->KernelMemoryType=CUDA_MEM_TYPE_GLOBAL; }
  void SetKernelMemoryTypeToConstant(){ this->KernelMemoryType=CUDA_MEM_TYPE_CONST; }
  void SetKernelMemoryTypeToTexture(){ this->KernelMemoryType=CUDA_MEM_TYPE_TEX; }
  int GetKernelMemoryType(){ return this->KernelMemoryType; }

  /**
  Set the memory type to use for input arrays. Can be one of CUDA_MEM_TYPE_GLOBAL,
  or CUDA_MEM_TYPE_TEX.
  */
  void SetInputMemoryType(int memType){ this->InputMemoryType=memType; }
  void SetInputMemoryTypeToGlobal(){ this->InputMemoryType=CUDA_MEM_TYPE_GLOBAL; }
  void SetInputMemoryTypeToTexture(){ this->InputMemoryType=CUDA_MEM_TYPE_TEX; }
  int GetInputMemoryType(){ return this->InputMemoryType; }

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
  /*
  void Convolution3D(
      CartesianExtent &extV,
      CartesianExtent &extW,
      CartesianExtent &extK,
      int ghostV,
      int mode,
      vtkDataArray *V,
      vtkDataArray *W,
      float *K);
  */
private:
  unsigned int NDevices;
  unsigned int DeviceId;
  unsigned int MaxThreads;
  unsigned int NThreads;
  unsigned int MaxBlocks;
  unsigned int NBlocks;
  int KernelMemoryType;
  int InputMemoryType;
  unsigned int BlockGridMax[3];
  unsigned int WarpSize;
  unsigned int WarpsPerBlock;
  unsigned int MaxWarpsPerBlock;
};

#endif

// VTK-HeaderTest-Exclude: CUDAConvolutionDriver.h
