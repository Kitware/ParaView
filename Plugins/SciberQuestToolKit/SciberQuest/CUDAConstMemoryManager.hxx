/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __CUDAConstMemoryManager_h
#define __CUDAConstMemoryManager_h

//#define CUDAMemoryManagerDEBUG

#include "CUDAMemoryManager.hxx"
#include "CUDAMacros.h"

#include "vtkDataArray.h"

#include <iostream>

#include <cuda.h>
#include <cuda_runtime.h>

/// CUDAConstMemoryManager - Memory manager for constant device memory
template<typename T>
class CUDAConstMemoryManager : public CUDAMemoryManager<T>
{
public:
  /**
  Constructors and copy constructors. The copy constructors copy
  the data from host memory poiniter into the new device memory
  block. The vtkDataArray overload conditionally copies data
  if push is set to true.
  */
  static CUDAConstMemoryManager<T> *New(const char *name);
  static CUDAConstMemoryManager<T> *New(const char *name, T *hostBlock);
  static CUDAConstMemoryManager<T> *New(const char *name, T *hostBlock, unsigned long nT);
  static CUDAConstMemoryManager<T> *New(const char *name, vtkDataArray *da, int push=0);

  ///
  virtual ~CUDAConstMemoryManager();

  /**
  Copy from host memory to device memory.
  */
  virtual int Push(T *hostBlock);
  virtual int Push(T *hostBlock, unsigned long nT);
  virtual int Push(vtkDataArray *da);
  virtual int Push(vtkDataArray *da, unsigned long nT);


  /**
  Get a pointer to the device memory, usually to pass to
  a kernel.
  */
  virtual T *GetDevicePointer(){ return this->DeviceBlock; }
  virtual unsigned long GetSize(){ return this->N; }

protected:
  ///
  CUDAConstMemoryManager(const char *name, unsigned long nT=1);
  CUDAConstMemoryManager(const char *name, T *hostBlock, unsigned long nT=1);
  int Initialize(const char *name);

  CUDAConstMemoryManager(const CUDAConstMemoryManager<T>&); // not implemented
  void operator=(const CUDAConstMemoryManager<T>&); // not implemented

private:
  T *DeviceBlock;
  unsigned long N;
  const char *Name;
};

//-----------------------------------------------------------------------------
template<typename T>
CUDAConstMemoryManager<T>::CUDAConstMemoryManager(
      const char *name,
      unsigned long nT)
          :
      DeviceBlock(0),
      N(nT),
      Name(name)
{
  int ierr=this->Initialize(name);
  if (ierr)
    {
    this->DeviceBlock=0;
    this->N=0;
    }
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAConstMemoryManager<T>::CUDAConstMemoryManager(
    const char *name,
    T *hostBlock,
    unsigned long nT)
          :
      DeviceBlock(0),
      N(nT),
      Name(name)
{
  int ierr=this->Initialize(name);
  if (ierr)
    {
    this->DeviceBlock=0;
    this->N=0;
    return;
    }
  this->Push(hostBlock);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAConstMemoryManager<T>::~CUDAConstMemoryManager()
{}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAConstMemoryManager<T>::Initialize(const char *name)
{
  cudaError_t ierr=cudaGetSymbolAddress((void**)&this->DeviceBlock,name);
  if (ierr)
    {
    CUDAErrorMacro(std::cerr,ierr,"Failed to get address for symbol " << name);
    return -1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAConstMemoryManager<T> *
CUDAConstMemoryManager<T>::New(const char *name)
{
  return new CUDAConstMemoryManager<T>(name);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAConstMemoryManager<T> *
CUDAConstMemoryManager<T>::New(const char *name, T *hostBlock)
{
   return new CUDAConstMemoryManager<T>(name,hostBlock,1);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAConstMemoryManager<T> *
CUDAConstMemoryManager<T>::New(
      const char *name,
      T *hostBlock,
      unsigned long nT)
{
   return new CUDAConstMemoryManager<T>(name,hostBlock,nT);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAConstMemoryManager<T> *
CUDAConstMemoryManager<T>::New(
      const char *name,
      vtkDataArray *da,
      int push)
{
  unsigned long n=
    da->GetNumberOfTuples()*da->GetNumberOfComponents();

  if (push)
    {
    return
      new CUDAConstMemoryManager<T>(name,(T*)da->GetVoidPointer(0),n);
    }
  return
    new CUDAConstMemoryManager<T>(name,n);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAConstMemoryManager<T>::Push(vtkDataArray *da)
{
  T *pDa=(T*)da->GetVoidPointer(0);
  return this->Push(pDa,this->N);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAConstMemoryManager<T>::Push(vtkDataArray *da, unsigned long nT)
{
  T *pDa=(T*)da->GetVoidPointer(0);
  return this->Push(pDa,nT);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAConstMemoryManager<T>::Push(T *hostBlock)
{
  return this->Push(hostBlock,this->N);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAConstMemoryManager<T>::Push(T *hostBlock, unsigned long nT)
{
  unsigned long n=nT*sizeof(T);
  #ifdef CUDAMemoryManagerDEBUG
  std::cerr
    << "Pushing " << this->DeviceBlock << ", " << n
    << " bytes to the device"
    << std::endl;
  #endif
  cudaError_t ierr;
  ierr=cudaMemcpyToSymbol(
      this->Name,
      hostBlock,
      n);
  if (ierr)
    {
    CUDAErrorMacro(std::cerr,ierr,"Push failed");
    return -1;
    }
  return 0;
}

#endif
