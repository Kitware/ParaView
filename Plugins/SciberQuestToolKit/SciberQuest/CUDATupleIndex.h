/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __CUDATupleIndex_h
#define __CUDATupleIndex_h

#include "CartesianExtent.h" // for CartesianExtent
#include "CUDAMemoryManager.hxx" // for CUDAMemoryManager
#include <cuda.h> // standard cuda header
#include <cuda_runtime.h> //

/// CUDATupleIndex - A class to convert a flat index into an i,j,k tuple
/**
CUDATupleIndex - A class to convert a flat index into an i,j,k tuple.
*/
template<int MODE> class CUDATupleIndex;

template<int MODE>
CUDATupleIndex<MODE> *NewCUDATupleIndex(
    CartesianExtent &ext,
    int nGhost,
    int mode)
{
    switch(mode)
      {
      case CartesianExtent::DIM_MODE_2D_XZ:
        return new CUDATupleIndex<CartesianExtent::DIM_MODE_2D_XZ>(ext,nGhost);
        break;

      case CartesianExtent::DIM_MODE_2D_YZ:
        break;

      case CartesianExtent::DIM_MODE_2D_XY:
        break;

      case CartesianExtent::DIM_MODE_3D:
        break;
      }
}



/// specialization for 2d xy
template<>
class CUDATupleIndex<CartesianExtent::DIM_MODE_2D_XY>
{
public:
  __host__
  __device__
  CUDATupleIndex()
        :
      Ni(0),
      NGhost(0)
  {}

  ///
  __host__
  CUDATupleIndex(
      const CartesianExtent &ext,
      long nGhost)
  {
    int size[3];
    ext.Size(size);
    this->Ni=size[0];
    this->NGhost=nGhost;
  }

  ///
  __host__
  CUDATupleIndex(
      unsigned long ni,
      unsigned long nj,
      unsigned long nk,
      long nGhost)
        :
      Ni(ni),
      NGhost(nGhost)
  {}

  ///
  __host__
  ~CUDATupleIndex(){}

  ///
  __device__
  void operator()(
      unsigned long idx,
      unsigned long &i,
      unsigned long &j,
      unsigned long &k)
  {
    j=idx/this->Ni;
    i=idx-j*this->Ni;
    j+=this->NGhost;
    i+=this->NGhost;
  }

  ///
  __device__
  void Print(const char *name)
  {
    printf("[%i %i] %s<2D_XY>=(Ni=%lu NGhost=%lu)\n",
    blockIdx.x,threadIdx.x,
    name,
    this->Ni,this->NGhost);
  }

private:
  unsigned long Ni;
  long NGhost;
};

/// spcialization for xz
template<>
class CUDATupleIndex<CartesianExtent::DIM_MODE_2D_XZ>
{
public:
  __host__
  __device__
  CUDATupleIndex()
        :
      Ni(0),
      NGhost(0)
  {}

  ///
  __host__
  CUDATupleIndex(
      const CartesianExtent &ext,
      long nGhost)
  {
    int size[3];
    ext.Size(size);
    this->Ni=size[0];
    this->NGhost=nGhost;
  }

  ///
  __host__
  CUDATupleIndex(
      unsigned long ni,
      unsigned long nj,
      unsigned long nk,
      long nGhost)
        :
      Ni(ni),
      NGhost(nGhost)
  {}

  ///
  __host__
  ~CUDATupleIndex(){}

  ///
  __device__
  void operator()(
      unsigned long idx,
      unsigned long &i,
      unsigned long &j,
      unsigned long &k)
  {
    k=idx/this->Ni;
    i=idx-j*this->Ni;
    k+=this->NGhost;
    i+=this->NGhost;
  }

  ///
  __device__
  void Print(const char *name)
  {
    printf("[%i %i] %s<2D_XZ>=(Ni=%lu NGhost=%lu)\n",
    blockIdx.x,threadIdx.x,
    name,
    this->Ni,this->NGhost);
  }

private:
  unsigned long Ni;
  long NGhost;

};

/// specialization for yz
template<>
class CUDATupleIndex<CartesianExtent::DIM_MODE_2D_YZ>
{
public:
  __host__
  __device__
  CUDATupleIndex()
        :
      Nj(0),
      NGhost(0)
  {}

  ///
  __host__
  CUDATupleIndex(
      const CartesianExtent &ext,
      long nGhost)
  {
    int size[3];
    ext.Size(size);
    this->Nj=size[0];
    this->NGhost=nGhost;
  }

  ///
  __host__
  CUDATupleIndex(
      unsigned long ni,
      unsigned long nj,
      unsigned long nk,
      long nGhost)
        :
      Nj(ni),
      NGhost(nGhost)
  {}

  ///
  __host__
  ~CUDATupleIndex(){}

  ///
  __device__
  void operator()(
      unsigned long idx,
      unsigned long &i,
      unsigned long &j,
      unsigned long &k)
  {
    k=idx/this->Nj;
    j=idx-j*this->Nj;
    k+=this->NGhost;
    j+=this->NGhost;
  }

  ///
  __device__
  void Print(const char *name)
  {
    printf("[%i %i] %s<2D_YZ>=(Nj=%lu NGhost=%lu)\n",
    blockIdx.x,threadIdx.x,
    name,
    this->Nj,this->NGhost);
  }

private:
  unsigned long Nj;
  long NGhost;
};

/// specialization for 3d
template<>
class CUDATupleIndex<CartesianExtent::DIM_MODE_3D>
{
public:
  __host__
  __device__
  CUDATupleIndex()
        :
      Nij(0),
      Ni(0),
      NGhost(0)
  {}

  ///
  __host__
  CUDATupleIndex(
      const CartesianExtent &ext,
      long nGhost)
  {
    int size[3];
    ext.Size(size);
    this->Nij=size[0]*size[1];
    this->Ni=size[0];
    this->NGhost=nGhost;
  }

  ///
  __host__
  CUDATupleIndex(
      unsigned long ni,
      unsigned long nj,
      unsigned long nk,
      long nGhost)
        :
      Nij(ni*nj),
      Ni(ni),
      NGhost(nGhost)
  {}

  ///
  __host__
  ~CUDATupleIndex(){}

  ///
  __device__
  void operator()(
      unsigned long idx,
      unsigned long &i,
      unsigned long &j,
      unsigned long &k)
  {
    k=idx/this->Nij;
    j=(idx-k*this->Nij)/this->Ni;
    i=idx-k*this->Nij-j*this->Ni;
    k+=this->NGhost;
    j+=this->NGhost;
    i+=this->NGhost;
  }

  ///
  __device__
  void Print(const char *name)
  {
    printf("[%i %i] %s<3D>=(Nij=%lu Ni=%lu NGhost=%lu)\n",
    blockIdx.x,threadIdx.x,
    name,
    this->Nij,this->Ni,this->NGhost);
  }

private:
  unsigned long Nij;
  unsigned long Ni;
  long NGhost;

};

/*
The tuple is computed using the following formula.


  mode  k  j  i  A    B    C  D  E  F  G  H  I  J
  -----------------------------------------------
  3d    Q  R  S  nxy  nx   1  0  1  0  1  1  1  1
  xy    0  Q  R  nx   1    0  1  0  1  0  1  1  0
  xz    Q  0  R  nx   1    1  0  0  1  0  1  0  1
  yz    Q  R  0  ny   1    1  0  1  0  0  0  1  1
  -----------------------------------------------

Q=idx/A
R=(idx-QA)/B
S=idx-QA-RB

k=QC-gH
j=QD+RE-gI
i=RF+SG-gJ

where g is a translation that can be used to
move from one index space to another.
{
public:
  __host__
  __device__
  CUDATupleIndex()
        :
      A(0),
      B(0),
      C(0),
      D(0),
      E(0),
      F(0),
      G(0),
      H(0),
      I(0),
      J(0),
      DeviceObject(0)
  {}

  ///
  __host__
  CUDATupleIndex(
      const CartesianExtent &ext,
      long nGhost,
      long mode)
        :
      DeviceObject(0)
  {
    int size[3];
    ext.Size(size);
    this->Initialize(size[0],size[1],size[2],nGhost,mode);
  }

  ///
  __host__
  CUDATupleIndex(
      unsigned long ni,
      unsigned long nj,
      unsigned long nk,
      long nGhost,
      long mode)
        :
      DeviceObject(0)
  {
    this->Initialize(ni,nj,nk,nGhost,mode);
  }

  ///
  __host__
  void Initialize(
      unsigned long ni,
      unsigned long nj,
      unsigned long nk,
      long g,
      long mode)
  {
    mode  k  j  i  A    B    C  D  E  F  G  H  I  J
    -----------------------------------------------
    3d    Q  R  S  nxy  nx   1  0  1  0  1  1  1  1
    xy    0  Q  R  nx   1    0  1  0  1  0  1  1  0
    xz    Q  0  R  nx   1    1  0  0  1  0  1  0  1
    yz    Q  R  0  ny   1    1  0  1  0  0  0  1  1
    -----------------------------------------------

    switch(mode)
      {
      case CartesianExtent::DIM_MODE_2D_XZ:
        this->A = ni;
        this->B = 1;
        //
        this->C = 1;
        this->D = 0;
        this->E = 0;
        this->F = 1;
        this->G = 0;
        //
        this->H = g;
        this->I = 0;
        this->J = g;
        break;

      case CartesianExtent::DIM_MODE_2D_YZ:
        this->A = nj;
        this->B = 1;
        //
        this->C = 1;
        this->D = 0;
        this->E = 1;
        this->F = 0;
        this->G = 0;
        //
        this->H = 0;
        this->I = g;
        this->J = g;
        break;

      case CartesianExtent::DIM_MODE_2D_XY:
        this->A = ni;
        this->B = 1;
        //
        this->C = 0;
        this->D = 1;
        this->E = 0;
        this->F = 1;
        this->G = 0;
        //
        this->H = g;
        this->I = g;
        this->J = 0;
        break;

      case CartesianExtent::DIM_MODE_3D:
        this->A = ni*nj;
        this->B = ni;
        //
        this->C = 1;
        this->D = 0;
        this->E = 1;
        this->F = 0;
        this->G = 1;
        //
        this->H = g;
        this->I = g;
        this->J = g;
        break;
      }

      delete this->DeviceObject;
      this->DeviceObject
        = CUDAMemoryManager<CUDATupleIndex>::New(this);
      this->DeviceObject->Push();
  }

  ///
  __host__
  ~CUDATupleIndex()
  {
    delete this->DeviceObject;
  }

  ///
  __host__
  CUDATupleIndex *GetDevicePointer()
  {
    return this->DeviceObject->GetDevicePointer();
  }

  ///
  __device__
  void operator()(
      unsigned long idx,
      unsigned long &i,
      unsigned long &j,
      unsigned long &k)
  {
    unsigned long Q=idx/this->A;
    unsigned long R=(idx - Q*this->A)/this->B;
    unsigned long S=idx - Q*this->A - R*this->B;

    k=Q*this->C + this->H;
    j=Q*this->D + R*this->E + this->I;
    i=R*this->F + S*this->G + this->J;
  }

  ///
  __device__
  void Print(const char *name)
  {
    printf("[%i %i] %s=(A=%lu B=%lu, C=%lu D=%lu E=%lu F=%lu G=%lu, H=%lu I=%lu J=%lu)\n",
    blockIdx.x,threadIdx.x,
    name,
    this->A,this->B,
    this->C,this->D,this->E,this->F,this->G,
    this->H,this->I,this->J);
  }

private:
  // tup dims
  unsigned long A;
  unsigned long B;
  // tup coefs
  unsigned long C;
  unsigned long D;
  unsigned long E;
  unsigned long F;
  unsigned long G;
  // shift coefs
  long H;
  long I;
  long J;
  //
  CUDAMemoryManager<CUDATupleIndex> *DeviceObject;
};
*/
#endif

// VTK-HeaderTest-Exclude: CUDATupleIndex.h
