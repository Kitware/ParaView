/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/

//#define CUDAConvolutionDEBUG

#include "CUDAThreadedIterator.h"
#include "CUDA3DDecomp.h"

#include <cuda.h>
#include <cuda_runtime.h>

#ifdef CUDAConvolutionDEBUG
  #include "stdio.h"
#endif

__global__
void hello(float f)
{
  #ifdef CUDAConvolutionDEBUG
  printf("Hello %i %i\n",gridDim.x,threadIdx.x);
  #endif
}

/**
constant memory buffer for use  when passing the kernel
into CUDAConvolution fundtions.
*/
__constant__ float gK[16384];

/**
texture memory reference for input data arrays
*/
texture<float,2> gV2;
texture<float,3> gV3;

/**
This implementation is written so that adjacent threads access adjacent
memory locations. This requires that vtk vectors/tensors etc be split.
This implementation uses texture memory for the input arrays.
*/
//*****************************************************************************
template<typename T>
__global__
void CUDAScalarConvolution2D(
      //int worldRank,
      unsigned long vni,
      unsigned long wni,
      unsigned long wnij,
      unsigned long kni,
      unsigned long nGhost,
      T * __restrict__ W,
      float * __restrict__ K)
{
  #ifdef CUDAConvolutionDEBUG
  int worldRank=0;
  //printf("CUDAScalarConvolution2D - striped\n");
  printf("[%i] vni=%lu wni=%lu wnij=%lu kni=%lu nGhost=%lu W=%p K=%p\n",
    worldRank,vni,wni,wnij,kni,nGhost,(void*)W,(void*)K);
  #endif

  // get a tuple from the current flat index in the output
  // index space
  unsigned long wi=ThreadIdToArrayIndex();
  if (!IndexIsValid(wi,wnij)) return;

  W[wi]=0.0;

  unsigned long i,j;
  j=wi/wni;
  i=wi-j*wni;
  /*j+=nGhost;
  i+=nGhost;*/

  #ifdef CUDAConvolutionDEBUG
  printf(
    "[%i][%i %i %i %i] i,j=(%li,%li) wi=%li W[wi]=%0.4f\n",
    worldRank,blockIdx.x,blockIdx.y,blockIdx.z,threadIdx.x,i,j,wi,W[wi]);
  #endif

  // visit each kernel element
  for (long g=0; g<kni; ++g)
    {
    unsigned long b=g*kni;
    for (long f=0; f<kni; ++f)
      {
      unsigned long ki=b+f;

      int p=i+f;
      int q=j+g;
      T V_vi=tex2D(gV2,p,q);

      W[wi]+=V_vi*K[ki];
      #ifdef CUDAConvolutionDEBUG
      unsigned long vi=q*vni+p;
      printf(
        "[%i][%i %i %i %i] i=%lu j=%lu f=%li g=%li vi=%li V[vi]=%0.4f "
        "b=%lu ki=%lu K[ki]=%0.4f wi=%lu W[wi]=%0.4f\n",
        worldRank,blockIdx.x,blockIdx.y,blockIdx.z,threadIdx.x,i,j,f,g,vi,V_vi,b,ki,K[ki],wi,W[wi]);
      #endif
      }
    }
}

/**
This implementation is written so that adjacent threads access adjacent
memory locations. This requires that vtk vectors/tensors etc be split.
*/
//*****************************************************************************
template<typename T>
__global__
void CUDAScalarConvolution2D(
      //int worldRank,
      unsigned long vni,
      unsigned long wni,
      unsigned long wnij,
      unsigned long kni,
      unsigned long nGhost,
      T * __restrict__ V,
      T * __restrict__ W,
      float * __restrict__ K)
{
  #ifdef CUDAConvolutionDEBUG
  int worldRank=0;
  //printf("CUDAScalarConvolution2D - striped\n");
  printf("[%i] vni=%lu wni=%lu wnij=%lu kni=%lu nGhost=%lu V=%p W=%p K=%p\n",
    worldRank,vni,wni,wnij,kni,nGhost,(void*)V,(void*)W,(void*)K);
  #endif

  // get a tuple from the current flat index in the output
  // index space
  unsigned long wi=ThreadIdToArrayIndex();
  if (!IndexIsValid(wi,wnij)) return;

  W[wi]=0.0;

  unsigned long i,j;
  j=wi/wni;
  i=wi-j*wni;
  /*j+=nGhost;
  i+=nGhost;*/

  #ifdef CUDAConvolutionDEBUG
  printf(
    "[%i][%i %i %i %i] i,j=(%li,%li) wi=%li W[wi]=%0.4f\n",
    worldRank,blockIdx.x,blockIdx.y,blockIdx.z,threadIdx.x,i,j,wi,W[wi]);
  #endif

  // visit each kernel element
  for (long g=0; g<kni; ++g)
    {
    //unsigned long q=vni*(j+g-nGhost);
    unsigned long q=vni*(j+g);
    unsigned long b=g*kni;
    for (long f=0; f<kni; ++f)
      {
      //unsigned long p=i+f-nGhost;
      unsigned long vi=q+i+f;

      unsigned long ki=b+f;
      W[wi]+=V[vi]*K[ki];
      #ifdef CUDAConvolutionDEBUG
      printf(
        "[%i][%i %i %i %i] q=%lu vi=%lu V[vi]=%0.4f "
        "b=%lu ki=%lu K[ki]=%0.4f wi=%lu W[wi]=%0.4f\n",
        worldRank,blockIdx.x,blockIdx.y,blockIdx.z,threadIdx.x,q,vi,V[vi],b,ki,K[ki],wi,W[wi]);
      #endif
      }
    }
}

//*****************************************************************************
template<typename T>
__global__
void CUDAScalarConvolution3D(
      unsigned long vni,
      unsigned long vnij,
      unsigned long wni,
      unsigned long wnij,
      unsigned long wnijk,
      unsigned long kni,
      unsigned long knij,
      unsigned long nGhost,
      T * __restrict__ V,
      T * __restrict__ W,
      float * __restrict__ K)
{
  #ifdef CUDAConvolutionDEBUG
  //printf("CUDAScalarConvolution3D\n");
  printf("vni=%lu wni=%lu wnij=%lu kni=%lu nComp=%lu nGhost=%lu V=%p W=%p K=%p\n",
    vni,wni,wnij,kni,nComp,nGhost,(void*)V,(void*)W,(void*)K);
  #endif

  // get a tuple from the current flat index in the output
  // index space
  unsigned long wi=ThreadIdToArrayIndex();
  if (!IndexIsValid(wi,wnij)) return;

  unsigned long i,j,k;
  k=wi/wnij;
  j=(wi-k*wnij)/wni;
  i=wi-k*wnij-j*wni;
  /*
  k+=nGhost;
  j+=nGhost;
  i+=nGhost;
  */

  // initialize the output
  W[wi]=0.0;

  #ifdef CUDAConvolutionDEBUG
  printf(
    "[%i %i] i,j,k=(%li,%li,%li) wi=%li W[wi]=%0.4f\n",
    blockIdx.x,threadIdx.x,i,j,k,wi,W[wi]);
  #endif

  // visit each kernel element
  for (long h=0; h<kni; ++h)
    {
    //unsigned long r=nij*(k+h-nGhost);
    unsigned long r=vnij*(k+h);
    unsigned long c=h*knij;
    for (long g=0; g<kni; ++g)
      {
      //unsigned long q=ni*(j+g-nGhost);
      unsigned long q=r+vni*(j+g);
      unsigned long b=c+g*kni;
      for (long f=0; f<kni; ++f)
        {
        //unsigned long p=i+f-nGhost;
        unsigned long vi=q+i+f;
        unsigned long ki=b+f;

        W[wi]+=V[vi]*K[ki];

        #ifdef CUDAConvolutionDEBUG
        printf(
          "[%i %i] q,r=(%lu,%lu) vi=%lu V[vi]=%0.4f "
          "b,c=(%lu,%lu) ki=%lu K[ki]=%0.4f wi=%lu W[vi]=%0.4f\n",
          blockIdx.x,threadIdx.x,q,r,vi,V[vi],b,c,ki,K[ki],wi,W[wi]);
        #endif
        }
      }
    }
}


//*****************************************************************************
template<typename T>
__global__
void CUDAConvolution2D(
      unsigned long vni,
      unsigned long wni,
      unsigned long wnij,
      unsigned long kni,
      unsigned long nComp,
      unsigned long nGhost,
      T * __restrict__ V,
      T * __restrict__ W,
      float * __restrict__ K)
{
  #ifdef CUDAConvolutionDEBUG
  //printf("CUDAConvolution2D - striped\n");
  printf("vni=%lu wni=%lu wnij=%lu kni=%lu nComp=%lu nGhost=%lu V=%p W=%p K=%p\n",
    vni,wni,wnij,kni,nComp,nGhost,(void*)V,(void*)W,(void*)K);
  #endif

  CUDAThreadedIterator it(wnij);
  for (; it.Ok(); it.Next())
    {
    // get a tuple from the current flat index in the output
    // index space
    unsigned long wi=it();

    unsigned long i,j;
    j=wi/wni;
    i=wi-j*wni;
    /*
    j+=nGhost;
    i+=nGhost;
    */

    wi*=nComp;

    // initialize the output vector
    for (long c=0; c<nComp; ++c)
      {
      W[wi+c]=0.0;
      }

    #ifdef CUDAConvolutionDEBUG
    printf(
      "[%i %i] i,j=(%li,%li) wi=%li W[wi]=%0.4f\n",
      blockIdx.x,threadIdx.x,i,j,wi,W[wi]);
    #endif

    // visit each kernel element
    for (long g=0; g<kni; ++g)
      {
      //unsigned long q=vni*(j+g-nGhost);
      unsigned long q=vni*(j+g);
      unsigned long b=g*kni;
      for (long f=0; f<kni; ++f)
        {
        //unsigned long p=i+f-nGhost;
        unsigned long vi=q+i+f;
        vi*=nComp;

        unsigned long ki=b+f;

        for (long m=0; m<nComp; ++m)
          {
          W[wi+m]+=V[vi+m]*K[ki];
          #ifdef CUDAConvolutionDEBUG
          printf(
            "[%i %i] q=%lu vi=%lu V[vi]=%0.4f "
            "b=%lu ki=%lu K[ki]=%0.4f wi=%lu W[vi]=%0.4f\n",
            blockIdx.x,threadIdx.x,q,vi+m,V[vi+m],b,ki,K[ki],wi+m,W[wi+m]);
          #endif
          }

        }
      }
    }
}

//*****************************************************************************
template<typename T>
__global__
void CUDAConvolution3D(
      unsigned long vni,
      unsigned long vnij,
      unsigned long wni,
      unsigned long wnij,
      unsigned long wnijk,
      unsigned long kni,
      unsigned long nComp,
      unsigned long nGhost,
      T * __restrict__ V,
      T * __restrict__ W,
      float * __restrict__ K)
{
  #ifdef CUDAConvolutionDEBUG
  //printf("CUDAConvolution2DXY - striped\n");
  printf("vni=%lu wni=%lu wnij=%lu kni=%lu nComp=%lu nGhost=%lu V=%p W=%p K=%p\n",
    vni,wni,wnij,kni,nComp,nGhost,(void*)V,(void*)W,(void*)K);
  #endif
  unsigned long knij=kni*kni;


  CUDAThreadedIterator it(wnijk);
  for (it.Initialize(); it.Ok(); it.Next())
    {
    // get a tuple from the current flat index in the output
    // index space
    unsigned long wi=it();

    unsigned long i,j,k;
    k=wi/wnij;
    j=(wi-k*wnij)/wni;
    i=wi-k*wnij-j*wni;
    /*
    k+=nGhost;
    j+=nGhost;
    i+=nGhost;
    */

    wi*=nComp;

    // initialize the output vector
    for (long c=0; c<nComp; ++c)
      {
      W[wi+c]=0.0;
      }

    #ifdef CUDAConvolutionDEBUG
    printf(
      "[%i %i] i,j,k=(%li,%li,%li) wi=%li W[wi]=%0.4f\n",
      blockIdx.x,threadIdx.x,i,j,k,wi,W[wi]);
    #endif

    // visit each kernel element
    for (long h=0; h<kni; ++h)
      {
      //unsigned long r=nij*(k+h-nGhost);
      unsigned long r=vnij*(k+h);
      unsigned long c=h*knij;
      for (long g=0; g<kni; ++g)
        {
        //unsigned long q=ni*(j+g-nGhost);
        unsigned long q=r+vni*(j+g);
        unsigned long b=c+g*kni;
        for (long f=0; f<kni; ++f)
          {
          //unsigned long p=i+f-nGhost;
          unsigned long vi=q+i+f;
          vi*=nComp;

          unsigned long ki=b+f;

          for (long m=0; m<nComp; ++m)
            {
            W[wi+m]+=V[vi+m]*K[ki];
            }

          #ifdef CUDAConvolutionDEBUG
          printf(
            "[%i %i] q,r=(%lu,%lu) vi=%lu V[vi]=%0.4f "
            "b,c=(%lu,%lu) ki=%lu K[ki]=%0.4f wi=%lu W[vi]=%0.4f\n",
            blockIdx.x,threadIdx.x,q,r,vi,V[vi],b,c,ki,K[ki],wi,W[wi]);
          #endif
          }
        }
      }
    }
}


/**
CUDAConvolution
Applies a convoultion to a 2d(xy,xz,yz) or 3d recilinear
dataset. This implementation is to be used with a striped
domain decomposition where the dataset is decomposed into
a set of stripes identified by the cuda blockIdx.x. Each
stripe is further decomposed by cuda threadId.x.

tupW - convert a flat index in output to a tuple in input space
sizeW - output size
compW - number of components
idxV - convert a tuple to a flat index in input space
extK - kernel extent [-ng/2 ng/2]
idxK - convert a tuple to flat index in kernel space
V - input data
W - output data
K - kernel data

//-----------------------------------------------------------------------------
template<typename T>
__global__
void CUDAConvolution(
      CUDAFlatIndex *idxV,
      T * __restrict__ V,
      CUDATupleIndex *tupW,
      unsigned long sizeW,
      int compW,
      T * __restrict__ W,
      int *extK,
      CUDAFlatIndex *idxK,
      T * __restrict__ K)
{
  #ifdef CUDAConvolutionDEBUG
  //printf("CUDAConvolution - striped\n");
  idxV->Print("idxV");
  printf("[%i %i] V=%p\n",blockIdx.x,threadIdx.x,(void *)V);
  tupW->Print("tupW");
  printf("[%i %i] sizeW=%lu\n",blockIdx.x,threadIdx.x,sizeW);
  printf("[%i %i] compW=%i\n",blockIdx.x,threadIdx.x,compW);
  printf("[%i %i] W=%p\n",blockIdx.x,threadIdx.x,(void *)W);
  printf("[%i %i] extK=(%i, %i, %i, %i, %i, %i)\n",blockIdx.x,threadIdx.x,extK[0],extK[1],extK[2],extK[3],extK[4],extK[5]);
  idxK->Print("idxK");
  printf("[%i %i] K=%p\n",blockIdx.x,threadIdx.x,(void *)K);
  #endif

  CUDAThreadedIterator it(sizeW);
  for (it.Initialize(); it.Ok(); it.Next())
    {
    // get a tuple from the current flat index in the output
    // index space
    unsigned long wi=it();
    unsigned long i,j,k;
    (*tupW)(wi,i,j,k);
    wi*=compW;

    // initialize the output vector
    for (long c=0; c<compW; ++c)
      {
      W[wi+c]=0.0;
      }

    #ifdef CUDAConvolutionDEBUG
    printf(
      "[%i %i] i,j,k=(%li,%li,%li) wi=%li W[wi]=%0.4f\n",
      blockIdx.x,threadIdx.x,i,j,k,wi,W[wi]);
    #endif

    // visit each element of the extK
    for (long h=extK[4]; h<=extK[5]; ++h)
      {
      unsigned long kk=h-extK[4];

      for (long g=extK[2]; g<=extK[3]; ++g)
        {
        unsigned long kj=g-extK[2];

        for (long f=extK[0]; f<=extK[1]; ++f)
          {
          unsigned long ki=f-extK[0];
          unsigned long kii=(*idxK)(ki,kj,kk);

          // get the coresponding flat index in the input index
          // space
          unsigned long p=i+f;
          unsigned long q=j+g;
          unsigned long r=k+h;
          unsigned long vi=(*idxV)(p,q,r);
          vi*=compW;

          for (long c=0; c<compW; ++c)
            {
            W[wi+c]+=V[vi+c]*K[kii];
            }

          #ifdef CUDAConvolutionDEBUG
          printf(
            "[%i %i] p,q,r=(%lu,%lu,%lu) vi=%lu V[vi]=%0.4f "
            "ki,kj,kk=(%lu,%lu,%lu) kii=%lu K[kii]=%0.4f wi=%lu W[vi]=%0.4f\n",
            blockIdx.x,threadIdx.x,p,q,r,vi,V[vi],ki,kj,kk,kii,K[kii],wi,W[wi]);
          #endif
          }
        }
      }
    }
}

*/
/**
CUDAConvolution
Applies a convoultion to a 2d(xy,xz,yz) or 3d recilinear
dataset. This implementation is to be used with a domain
decomposition where each cuda block represnets a single
voxel in the dataset and each thread represents a single
vector component. NOTE: This only works with newrer cards
that support 3d block decompositions.

idxW - convert a tuple in output space to a flat index
idxV - convert a tuple in input space to a flat index
nCompV - number of components
nGhostV - number of ghosts in each direction
extK - kernel extent [-ng/2 ng/2]
idxK - convert a tuple to flat index in kernel space
V - input data
W - output data
K - kernel data
//-----------------------------------------------------------------------------
template<typename T>
__global__
void CUDAConvolution(
      CUDAFlatIndex *idxV,
      int nCompV,
      int *nGhostV,
      T * __restrict__ V,
      CUDAFlatIndex *idxW,
      T * __restrict__ W,
      int *extK,
      CUDAFlatIndex *idxK,
      T * __restrict__ K)
{
  #ifdef CUDAConvolutionDEBUG
  printf("CUDAConvolution - 3D Block\n");
  printf("nComp=%i",nCompV);
  printf("nGhostV=%i %i %i\n",nGhostV[0],nGhostV[1],nGhostV[2]);
  printf("extK=%i %i %i %i %i %i\n",extK[0],extK[1],extK[2],extK[3],extK[4],extK[5]);
  #endif

  // get output tuple
  unsigned long i=blockIdx.x;
  unsigned long j=blockIdx.y;
  unsigned long k=blockIdx.z;
  unsigned long c=threadIdx.x;
  #ifdef CUDAConvolutionDEBUG
  #endif
  // get output index
  unsigned long wi=(*idxW)(i,j,k);
  wi*=nCompV;
  wi+=c;
  W[wi]=0.0;
  #ifdef CUDAConvolutionDEBUG
  printf("i,j,k,c=(%li,%li,%li,%li) wi=%li W[wi]=%0.4f\n",i,j,k,c,wi,W[wi]);
  #endif

  // get input tuple
  i+=nGhostV[0];
  j+=nGhostV[1];
  k+=nGhostV[2];

  // visit each element of the extK
  for (long h=extK[4]; h<=extK[5]; ++h)
    {
    unsigned long kk=h-extK[4];

    for (long g=extK[2]; g<=extK[3]; ++g)
      {
      unsigned long kj=g-extK[2];

      for (long f=extK[0]; f<=extK[1]; ++f)
        {
        // kernel index
        unsigned long ki=f-extK[0];
        unsigned long kii=(*idxK)(ki,kj,kk);

        // input index
        unsigned long p=i+f;
        unsigned long q=j+g;
        unsigned long r=k+h;
        unsigned long vi=(*idxV)(p,q,r);
        vi*=nCompV;
        vi+=c;
        #ifdef CUDAConvolutionDEBUG
        printf("p,q,r,c=(%li,%li,%li,%li) kii=%li K=%0.4f vi=%li V[vi]=%0.4f\n",p,q,r,c,kii,K[kii],vi,V[vi]);
        #endif

        // update
        W[wi]+=V[vi]*K[kii];
        }
      }
    }
  #ifdef CUDAConvolutionDEBUG
  printf("W[wi]=%0.4f\n",W[wi]);
  #endif
}
*/
