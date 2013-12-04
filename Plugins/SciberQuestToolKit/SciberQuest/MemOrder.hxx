/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "SQMacros.h"

#include <vector>


//*****************************************************************************
enum {
  REORDER_SPLIT=1,
  REORDER_INTERLEAVE=0
  };
template <typename T>
void SubsetReorder(
      CartesianExtent &extV,
      CartesianExtent &extW,
      T*  V,
      std::vector<T*> &W,
      int dimMode,
      int split)
{
  // copy a subextent of V described by extW  into an array W.
  unsigned long nComp=W.size();

  unsigned long nW[3];
  extW.Size(nW);

  unsigned long nV[3];
  extV.Size(nV);

  unsigned long vi0=extW[0];
  unsigned long vj0=extW[1];
  unsigned long vk0=extW[2];

  unsigned long wni;
  unsigned long wnij;

  unsigned long vni;
  unsigned long vnij;

  unsigned long ni;
  unsigned long nj;
  unsigned long nk;

  switch (dimMode)
    {
    case CartesianExtent::DIM_MODE_2D_XY:
      wni=nW[0];
      wnij=0;

      vni=nV[0];
      vnij=0;

      ni=nW[0];
      nj=nW[1];
      nk=1;
      break;

    case CartesianExtent::DIM_MODE_2D_XZ:
      wni=nW[0];
      wnij=0;

      vni=nV[0];
      vnij=0;

      ni=nW[0];
      nj=nW[2];
      nk=1;
      break;

    case CartesianExtent::DIM_MODE_2D_YZ:
      wni=nW[0];
      wnij=0;

      vni=nV[0];
      vnij=0;

      ni=nW[1];
      nj=nW[2];
      nk=1;
      break;

    case CartesianExtent::DIM_MODE_3D:
      wni=nW[0];
      wnij=nW[0]*nW[1];

      vni=nV[0];
      vnij=nV[0]*nV[1];

      ni=nW[0];
      nj=nW[1];
      nk=nW[2];
      break;
    }

  for (unsigned long k=0; k<nk; ++k)
    {
    unsigned long vk=(vk0+k)*vnij;
    unsigned long wk=k*wnij;
    for (unsigned long j=0; j<nj; ++j)
      {
      unsigned long vj=(vj0+j)*vni+vk;
      unsigned long wj=j*wni+wk;
      for (unsigned long i=0; i<ni; ++i)
        {
        unsigned long vi=nComp*(vj+vi0+i);
        unsigned long wi=wj+i;
        for (unsigned long c=0; c<nComp; ++c)
          {
          if (split)
            {
            W[c][wi]=V[vi+c];
            }
          else
            {
            V[vi+c]=W[c][wi];
            }
          }
        }
      }
    }
}

//*****************************************************************************
template<typename T>
void Split(
      size_t n,
      int nComp,
      T * __restrict__  V,
      T * __restrict__  W)
{
  // reorder a vtk vector array into three contiguous
  // scalar components
  for (size_t i=0; i<n; ++i)
    {
    size_t ii=nComp*i;
    for (int q=0; q<nComp; ++q)
      {
      size_t qq=n*q;
      W[qq+i]=V[ii+q];
    }
  }
}

//*****************************************************************************
template<typename T>
void Interleave(
      size_t n,
      int nComp,
      T * __restrict__  W,
      T * __restrict__  V)
{
  // take an irray that has been ordered contiguously by component
  // and interleave
  for (size_t i=0; i<n; ++i)
    {
    size_t ii=nComp*i;
    for (int q=0; q<nComp; ++q)
      {
      size_t qq=n*q;
      V[ii+q]=W[qq+i];
    }
  }
}

//*****************************************************************************
template<typename T>
void Split(
      size_t n,
      T * __restrict__ V,
      std::vector<T*> &W)
{
  // reorder a vtk vector array into contiguous
  // scalar components
  int nComp=(int)W.size();
  for (size_t i=0; i<n; ++i)
    {
    size_t ii=nComp*i;
    for (int q=0; q<nComp; ++q)
      {
      W[q][i]=V[ii+q];
      }
    }
}

//*****************************************************************************
template<typename T>
void Interleave(
      size_t n,
      std::vector<T *> &W,
      T * __restrict__  V)
{
  // interleave array in contiguous component order
  int nComp=(int)W.size();
  for (size_t i=0; i<n; ++i)
    {
    size_t ii=nComp*i;
    for (int q=0; q<nComp; ++q)
      {
      V[ii+q]=W[q][i];
    }
  }
}
