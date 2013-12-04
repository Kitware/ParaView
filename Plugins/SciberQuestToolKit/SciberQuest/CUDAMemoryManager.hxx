/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __CUDAMemoryManager_h
#define __CUDAMemoryManager_h

#include "SQMacros.h"
class vtkDataArray;

#include <iostream>

// this enables call trace printed to std::cerr
//#define CUDAMemoryManagerDEBUG

/// CUDAMemoryManager - interface to classes for managing device memory
/**
*/
template<typename T>
class CUDAMemoryManager
{
public:
  virtual ~CUDAMemoryManager(){}

  /**
  Copy from n elements of type T from host memory pointer
  into the device memory. If n is not specified then the
  size of the block created is used.
  */
  virtual int Push(T *hostBlock, unsigned long nT)
  { return this->WarnNoImplem("Push(T*,ulong)"); }

  virtual int Push(T *hostBlock)
  { return this->WarnNoImplem("Push(T*)"); }

  virtual int Push(vtkDataArray *da)
  { return this->WarnNoImplem("Push(vtkDataArray*)"); }

  virtual int Push(vtkDataArray *da, unsigned long nT)
  { return this->WarnNoImplem("Push(vtkDataArray*,ulong)"); }

  /**
  Copy from n elements of type T from device into the host
  memory pointer provided. If n is not specified then the
  size of the block created is used.
  */
  virtual int Pull(T *hostBlock, unsigned long nT)
  { return this->WarnNoImplem("Pull(T*,ulong)"); }

  virtual int Pull(T *hostBlock)
  { return this->WarnNoImplem("Pull(T*)"); }

  virtual int Pull(vtkDataArray *da)
  { return this->WarnNoImplem("Pull(vtkDataArray*)"); }

  virtual int Pull(vtkDataArray *da, unsigned long nT)
  { return this->WarnNoImplem("Pull(vtkDataArray*,ulong)"); }

  ///
  virtual T *GetDevicePointer()=0;

  ///
  virtual unsigned long GetSize()=0;

private:
  int WarnNoImplem(const char *id);
};

//-----------------------------------------------------------------------------
template<typename T>
int CUDAMemoryManager<T>::WarnNoImplem(const char *id)
{
  sqErrorMacro(std::cerr,<< id);
  return -1;
}

#endif
