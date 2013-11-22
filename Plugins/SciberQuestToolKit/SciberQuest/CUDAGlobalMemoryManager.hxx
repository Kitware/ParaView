/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __CUDAGlobalMemoryManager_h
#define __CUDAGlobalMemoryManager_h

//#define CUDAMemoryManagerDEBUG

#include "CUDAMemoryManager.hxx"
#include "CUDAMacros.h"

#include "vtkDataArray.h"

#include <iostream>

#include <cuda.h>
#include <cuda_runtime.h>

/// CUDAGlobalMemoryManager - Memory manager for global device memory
template<typename T>
class CUDAGlobalMemoryManager : public CUDAMemoryManager<T>
{
public:
  /**
  Constructors and copy constructors. The copy constructors copy
  the data from host memory poiniter into the new device memory
  block. The vtkDataArray overload conditionally copies data
  if push is set to true.
  */
  static CUDAGlobalMemoryManager<T> *New();
  static CUDAGlobalMemoryManager<T> *New(unsigned long n);
  static CUDAGlobalMemoryManager<T> *New(unsigned long nx, unsigned long ny);
  static CUDAGlobalMemoryManager<T> *New(T *hostBlock, unsigned long n=1);
  static CUDAGlobalMemoryManager<T> *New(T *hostBlock, unsigned long nx, unsigned long ny);
  static CUDAGlobalMemoryManager<T> *New(vtkDataArray *da, int push=0);

  ///
  virtual ~CUDAGlobalMemoryManager();

  /**
  Copy from host memory to device memory.
  */
  virtual int Push(T *hostBlock);
  virtual int Push(T *hostBlock, unsigned long nT);
  virtual int Push(T *hostBlock, unsigned long ni, unsigned long nj);
  virtual int Push(vtkDataArray *da);
  virtual int Push(vtkDataArray *da, unsigned long nT);

  /**
  Copy from device memory into host memory.
  */
  virtual int Pull(T *hostBlock);
  virtual int Pull(T *hostBlock, unsigned long nT);
  virtual int Pull(T *hostBlock, unsigned long ni, unsigned long nj);
  virtual int Pull(vtkDataArray *da);
  virtual int Pull(vtkDataArray *da, unsigned long nT);

  /**
  Set device memory to zero.
  */
  int Zero();
  int Zero(unsigned long n);
  int Zero(unsigned long ni, unsigned long nj);

  /**
  Get a pointer to the device memory, usually to pass to
  a kernel.
  */
  virtual T *GetDevicePointer(){ return this->DeviceBlock; }
  virtual unsigned long GetSize(){ return this->Ni; }

  /**
  Get the pitch from 2d allocation.
  */
  unsigned long GetPitch(){ return this->Pitch; }

  /**
  Assign a texture reference to this device memory.
  */
  virtual int BindToTexture(
        textureReference *tex);

  virtual int BindToTexture(
        textureReference *tex,
        int ni);

  virtual int BindToTexture(
        textureReference *tex,
        int ni,
        int nj);

  virtual int BindToTexture(
        textureReference *tex,
        int ni,
        int nj,
        int nk);

  /**
  Low level allocation and deallocation.
  */
  virtual int NewDeviceBlock(unsigned long n);
  virtual int NewDeviceBlock(unsigned long nx, unsigned long ny);
  virtual int DeleteDeviceBlock();

protected:
  ///
  CUDAGlobalMemoryManager();
  CUDAGlobalMemoryManager(unsigned long n);
  CUDAGlobalMemoryManager(unsigned long ni, unsigned long nj);
  CUDAGlobalMemoryManager(T *hostBlock, unsigned long n=1);
  CUDAGlobalMemoryManager(T *hostBlock, unsigned long ni, unsigned long nj);

  /**
  Get a reference to the texture by name.
  */
  const textureReference *GetTextureReference(const char *name);


  CUDAGlobalMemoryManager(const CUDAGlobalMemoryManager<T>&); // not implemented
  void operator=(const CUDAGlobalMemoryManager<T>&); // not implemented

private:
  T *DeviceBlock;
  unsigned long Ni;
  unsigned long Nj;
  unsigned long Nk;
  unsigned long Pitch;
};

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T>::CUDAGlobalMemoryManager()
      :
  DeviceBlock(0),
  Ni(1),
  Nj(0),
  Nk(0),
  Pitch(0)
{
  unsigned long size=sizeof(T);
  this->NewDeviceBlock(size);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T>::CUDAGlobalMemoryManager(
    unsigned long ni,
    unsigned long nj)
        :
    DeviceBlock(0),
    Ni(ni),
    Nj(nj),
    Nk(0),
    Pitch(0)
{
  this->NewDeviceBlock(ni,nj);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T>::CUDAGlobalMemoryManager(unsigned long n)
        :
    DeviceBlock(0),
    Ni(n),
    Nj(0),
    Nk(0),
    Pitch(0)
{
  unsigned long size=n*sizeof(T);
  this->NewDeviceBlock(size);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T>::CUDAGlobalMemoryManager(
    T *hostBlock,
    unsigned long ni,
    unsigned long nj)
        :
    DeviceBlock(0),
    Ni(ni),
    Nj(ni),
    Nk(0),
    Pitch(0)
{
  this->NewDeviceBlock(ni,nj);
  this->Push(hostBlock);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T>::CUDAGlobalMemoryManager(
    T *hostBlock,
    unsigned long n)
        :
    DeviceBlock(0),
    Ni(n),
    Nj(0),
    Nk(0),
    Pitch(0)
{
  unsigned long size=n*sizeof(T);
  this->NewDeviceBlock(size);
  this->Push(hostBlock);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T>::~CUDAGlobalMemoryManager()
{
  this->DeleteDeviceBlock();
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T> *
CUDAGlobalMemoryManager<T>::New()
{
  return new CUDAGlobalMemoryManager<T>;
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T> *
CUDAGlobalMemoryManager<T>::New(unsigned long n)
{
  return new CUDAGlobalMemoryManager<T>(n);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T> *
CUDAGlobalMemoryManager<T>::New(unsigned long ni, unsigned long nj)
{
  return new CUDAGlobalMemoryManager<T>(ni,nj);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T> *
CUDAGlobalMemoryManager<T>::New(T *hostBlock, unsigned long n)
{
   return new CUDAGlobalMemoryManager<T>(hostBlock,n);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T> *
CUDAGlobalMemoryManager<T>::New(T *hostBlock, unsigned long ni, unsigned long nj)
{
   return new CUDAGlobalMemoryManager<T>(hostBlock,ni,nj);
}

//-----------------------------------------------------------------------------
template<typename T>
CUDAGlobalMemoryManager<T> *
CUDAGlobalMemoryManager<T>::New(vtkDataArray *da, int push)
{
  int nComp=da->GetNumberOfComponents();
  unsigned long n=da->GetNumberOfTuples()*nComp;

  if (push)
    {
    return
      new CUDAGlobalMemoryManager<T>((T*)da->GetVoidPointer(0),n);
    }
  return
    new CUDAGlobalMemoryManager<T>(n);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::NewDeviceBlock(
      unsigned long ni,
      unsigned long nj)
{
  this->DeleteDeviceBlock();
  cudaError_t ierr;
  size_t pitch;
  ierr=cudaMallocPitch(&this->DeviceBlock,&pitch,ni*sizeof(T),nj);
  if (ierr)
    {
    CUDAErrorMacro(std::cerr,ierr,"NewDeviceBlock failed");
    return -1;
    }
  this->Pitch=pitch;

  #ifdef CUDAMemoryManagerDEBUG
  std::cerr
    << "Allocated " << this->DeviceBlock << ", " << ni << "-x-" << nj
    << " bytes on the device. Pitch=" << pitch
    << std::endl;
  #endif
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::NewDeviceBlock(unsigned long n)
{
  this->DeleteDeviceBlock();
  if (n==0) return 0;
  cudaError_t ierr;
  ierr=cudaMalloc(&this->DeviceBlock,n);
  if (ierr)
    {
    CUDAErrorMacro(std::cerr,ierr,"NewDeviceBlock failed");
    return -1;
    }
  #ifdef CUDAMemoryManagerDEBUG
  std::cerr
    << "Allocated " << this->DeviceBlock << ", " << n
    << " bytes on the device"
    << std::endl;
  #endif
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::DeleteDeviceBlock()
{
  if (this->DeviceBlock)
    {
    #ifdef CUDAMemoryManagerDEBUG
    std::cerr << "Deleted " << this->DeviceBlock << std::endl;
    #endif
    cudaFree(this->DeviceBlock);
    this->DeviceBlock=0;
    }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Push(vtkDataArray *da)
{
  T *pDa=(T*)da->GetVoidPointer(0);
  return this->Push(pDa);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Push(vtkDataArray *da, unsigned long nT)
{
  T *pDa=(T*)da->GetVoidPointer(0);
  return this->Push(pDa,nT);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Push(T *hostBlock)
{
  if (this->Nj)
    {
    return this->Push(hostBlock,this->Ni,this->Nj);
    }

  return this->Push(hostBlock,this->Ni);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Push(T *hostBlock, unsigned long nT)
{
  unsigned long n=nT*sizeof(T);
  #ifdef CUDAMemoryManagerDEBUG
  std::cerr
    << "Pushing " << this->DeviceBlock << ", " << n
    << " bytes to the device"
    << std::endl;
  #endif
  cudaError_t ierr;
  ierr=cudaMemcpy(
      this->DeviceBlock,
      hostBlock,
      n,
      cudaMemcpyHostToDevice);
  if (ierr)
    {
    CUDAErrorMacro(std::cerr,ierr,"Push failed");
    return -1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Push(
      T *hostBlock,
      unsigned long ni,
      unsigned long nj)
{
  #ifdef CUDAMemoryManagerDEBUG
  std::cerr
    << "Pushing " << this->DeviceBlock << ", " << ni*nj*sizeof(T)
    << " bytes to the device"
    << std::endl;
  #endif
  cudaError_t ierr;
  unsigned long hostPitch=ni*sizeof(T);
  ierr=cudaMemcpy2D(
      this->DeviceBlock,
      this->Pitch,
      hostBlock,
      hostPitch,
      hostPitch,
      nj,
      cudaMemcpyHostToDevice);
  if (ierr)
    {
    CUDAErrorMacro(std::cerr,ierr,"Push 2d failed");
    return -1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Pull(vtkDataArray *da)
{
  unsigned long n=this->Ni;
  T *pDa=(T*)da->GetVoidPointer(0);
  return this->Pull(pDa,n);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Pull(vtkDataArray *da, unsigned long nT)
{
  T *pDa=(T*)da->GetVoidPointer(0);
  return this->Pull(pDa,nT);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Pull(T *hostBlock)
{
  if (this->Nj)
    {
    return this->Pull(hostBlock,this->Ni,this->Nj);
    }

  return this->Pull(hostBlock,this->Ni);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Pull(T *hostBlock, unsigned long nT)
{
  unsigned long n=nT*sizeof(T);
  #ifdef CUDAMemoryManagerDEBUG
  std::cerr
    << "Pulling " << this->DeviceBlock << ", " << n
    << " bytes from the device"
    << std::endl;
  #endif
  cudaError_t ierr;
  ierr=cudaMemcpy(
      hostBlock,
      this->DeviceBlock,
      n,
      cudaMemcpyDeviceToHost);
  if (ierr)
    {
    CUDAErrorMacro(std::cerr,ierr,"Pull failed");
    return -1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Pull(
      T *hostBlock,
      unsigned long ni,
      unsigned long nj)
{
  #ifdef CUDAMemoryManagerDEBUG
  std::cerr
    << "Pulling " << this->DeviceBlock << ", " << ni*nj*sizeof(T)
    << " bytes from the device"
    << std::endl;
  #endif
  cudaError_t ierr;
  unsigned long hostPitch=ni*sizeof(T);
  ierr=cudaMemcpy2D(
      hostBlock,
      hostPitch,
      this->DeviceBlock,
      this->Pitch,
      hostPitch,
      nj,
      cudaMemcpyDeviceToHost);
  if (ierr)
    {
    CUDAErrorMacro(std::cerr,ierr,"Pull failed");
    return -1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Zero()
{
  if (this->Nj)
    {
    return this->Zero(this->Ni,this->Nj);
    }

  return this->Zero(this->Ni);
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Zero(unsigned long ni, unsigned long nj)
{
  #ifdef CUDAMemoryManagerDEBUG
  std::cerr
    << "Zero'd " << this->DeviceBlock << ", " << ni*nj*sizeof(T)
    << " bytes"
    << std::endl;
  #endif
  cudaError_t ierr;
  ierr=cudaMemset2D(this->DeviceBlock,this->Pitch,0,ni*sizeof(T),nj);
  if (ierr)
    {
    CUDAErrorMacro(std::cerr,ierr,"Memset 0 failed");
    return -1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int CUDAGlobalMemoryManager<T>::Zero(unsigned long nT)
{
  unsigned long n=nT*sizeof(T);
  #ifdef CUDAMemoryManagerDEBUG
  std::cerr
    << "Zero'd " << this->DeviceBlock << ", " << n
    << " bytes"
    << std::endl;
  #endif
  cudaError_t ierr;
  ierr=cudaMemset(this->DeviceBlock,0,n);
  if (ierr)
    {
    CUDAErrorMacro(std::cerr,ierr,"Memset 0 failed");
    return -1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int
CUDAGlobalMemoryManager<T>::BindToTexture(textureReference *tex)
{
  if (this->Nj)
    {
    return this->BindToTexture(tex,this->Ni,this->Nj);
    }

  return this->BindToTexture(tex,this->Ni);
}

//-----------------------------------------------------------------------------
template<typename T>
int
CUDAGlobalMemoryManager<T>::BindToTexture(
        textureReference *tex,
        int ni)
{
  cudaChannelFormatDesc desc=cudaCreateChannelDesc<T>();
  cudaError_t ierr=cudaBindTexture(
      NULL,
      tex,
      this->DeviceBlock,
      &desc,
      ni*sizeof(T));
  if (ierr)
    {
    CUDAErrorMacro(
        std::cerr,
        ierr,
        << "Failed to bind " << this->DeviceBlock);
    return -1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int
CUDAGlobalMemoryManager<T>::BindToTexture(
        textureReference *tex,
        int ni,
        int nj)
{
  cudaChannelFormatDesc desc=cudaCreateChannelDesc<T>();
  cudaError_t ierr=cudaBindTexture2D(
      NULL,
      tex,
      this->DeviceBlock,
      &desc,
      ni,
      nj,
      this->Pitch);
  if (ierr)
    {
    CUDAErrorMacro(
        std::cerr,
        ierr,
        << "Failed to bind 2d " << this->DeviceBlock);
    return -1;
    }

  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int
CUDAGlobalMemoryManager<T>::BindToTexture(
        textureReference *tex,
        int ni,
        int nj,
        int nk)
{
  // TODO
  sqErrorMacro(std::cerr,"Not implemented.");
  return -1;
}

//-----------------------------------------------------------------------------
template<typename T>
const textureReference *
CUDAGlobalMemoryManager<T>::GetTextureReference(
        const char *name)
{
  const textureReference *ref;
  cudaError_t ierr;
  ierr=cudaGetTextureReference(&ref,name);
  if (ierr)
    {
    CUDAErrorMacro(std::cerr,ierr,"Failed to find a texture reference for " << name);
    return 0;
    }
  return ref;
}

#endif
