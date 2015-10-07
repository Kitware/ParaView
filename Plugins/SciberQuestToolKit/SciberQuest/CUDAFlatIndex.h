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
#ifndef CUDAFlatIndex_h
#define CUDAFlatIndex_h

#include "CartesianExtent.h"
#include "CUDAMemoryManager.hxx" // for CUDAMemoryManager

#include <cuda.h> // standard cuda header
#include <cuda_runtime.h> //

/// CUDAFlatIndex - A class to convert i,j,k tuples into flat indices
/**
CUDAFlatIndex - A class to convert i,j,k tuples into flat indices
The following formula is applied:
<pre>
mode -> k*(A)    + j*(B)    +i*(C)
--------------------------------------
3d   -> k*(ninj) + j*(ni)   + i
xy   ->            j*(ni)   + i
xz   -> k*(ni)              + i
yz   -> k*(nj)   + j
--------------------------------------
</pre>
*/
class CUDAFlatIndex
{
public:
  ///
  __host__
  CUDAFlatIndex()
        :
      A(0),
      B(0),
      C(0),
      DeviceObject(0)
  {}

  ///
  __host__
  CUDAFlatIndex(const CartesianExtent &patch, int mode)
        :
      DeviceObject(0)
  {
    int size[3];
    patch.Size(size);
    this->Initialize(size[0],size[1],size[2],mode);
  }

  ///
  __host__
  CUDAFlatIndex(
      unsigned long ni,
      unsigned long nj,
      unsigned long nk,
      int mode)
        :
      DeviceObject(0)
  {
    this->Initialize(ni,nj,nk,mode);
  }

  ///
  __host__
  void Initialize(
      unsigned long ni,
      unsigned long nj,
      unsigned long nk,
      int mode)
  {
    switch(mode)
      {
      case CartesianExtent::DIM_MODE_2D_XZ:
        this->A = ni;
        this->B = 0;
        this->C = 1;
        break;

      case CartesianExtent::DIM_MODE_2D_YZ:
        this->A = nj;
        this->B = 1;
        this->C = 0;
        break;

      case CartesianExtent::DIM_MODE_2D_XY:
        this->A = 0;
        this->B = ni;
        this->C = 1;
        break;

      case CartesianExtent::DIM_MODE_3D:
        this->A = ni*nj;
        this->B = ni;
        this->C = 1;
        break;
      }

      delete this->DeviceObject;
      this->DeviceObject
        = CUDAMemoryManager<CUDAFlatIndex>::New(this);
      this->DeviceObject->Push();
  }

  ///
  __host__
  ~CUDAFlatIndex()
  {
    delete this->DeviceObject;
  }

  ///
  __host__
  CUDAFlatIndex *GetDevicePointer()
  {
    return this->DeviceObject->GetDevicePointer();
  }

  ///
  __device__
  unsigned long operator()(
      unsigned long i,
      unsigned long j,
      unsigned long k)
  {
    return k*this->A + j*this->B + i*this->C;
  }

  ///
  __device__
  void Print(const char *name)
  {
    printf("[%i %i] %s=(A=%lu B=%lu C=%lu)\n",
      blockIdx.x,threadIdx.x,name,this->A,this->B,this->C);
  }

private:
  unsigned long A;
  unsigned long B;
  unsigned long C;
  //
  CUDAMemoryManager<CUDAFlatIndex> *DeviceObject;
};

#endif

// VTK-HeaderTest-Exclude: CUDAFlatIndex.h
