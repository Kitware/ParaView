/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __Numerics_hxx
#define __Numerics_hxx

#include<iostream>

#include <cstdlib>
#include <cmath>
#include <complex>

#include "SQPOSIXOnWindowsWarningSupression.h"
#include "SQPosixOnWindows.h"
#include "SQEigenWarningSupression.h"
#include <Eigen/Eigenvalues>
using namespace Eigen;

#include "Tuple.hxx"
#include "FlatIndex.h"
#include "SQMacros.h"

//*****************************************************************************
template<typename T>
bool IsReal(std::complex<T> &c, T eps=T(1.0e-6))
{
  return (fabs(imag(c)) < eps);
}

//*****************************************************************************
template<typename T>
bool IsComplex(std::complex<T> &c, T eps=T(1.0e-6))
{
  return (fabs(imag(c)) >= eps);
}

//*****************************************************************************
template<typename T>
int fequal(T a, T b, T tol)
{
  T pda=fabs(a);
  T pdb=fabs(b);
  pda=pda<tol?tol:pda;
  pdb=pdb<tol?tol:pdb;
  T smaller=pda<pdb?pda:pdb;
  T norm=fabs(b-a)/smaller;
  if (norm<=tol)
    {
    return 1;
    }
  return 0;
}

//*****************************************************************************
template <typename T>
T LaplacianOfGaussian(T X[3], T a, T B[3], T c)
{
  // X - evaluate at this location
  // a - peak height
  // B - center
  // c - width

  T x,y,z;
  x=X[0]-B[0];
  y=X[1]-B[1];
  z=X[2]-B[2];

  T r2 = x*x+y*y+z*z;
  T c2 = c*c;

  return -a/c2 * (((T)1)-r2/c2) * ((T)exp(-r2/(((T)2)*c2)));
}


//*****************************************************************************
template <typename T>
T Gaussian(T X[3], T a, T B[3], T c)
{
  // X - evaluate at this location
  // a - peak height
  // B - center
  // c - width

  T x,y,z;
  x=X[0]-B[0];
  y=X[1]-B[1];
  z=X[2]-B[2];

  T r2 = x*x+y*y+z*z;

  return a*((T)exp(-r2/(((T)2)*c*c)));
}

//*****************************************************************************
inline
void indexToIJ(int idx, int nx, int &i, int &j)
{
  // convert a flat array index into a i,j,k three space tuple.
  j=idx/nx;
  i=idx-j*nx;
}

//*****************************************************************************
inline
void indexToIJK(int idx, int nx, int nxy, int &i, int &j, int &k)
{
  // convert a flat array index into a i,j,k three tuple.
  k=idx/nxy;
  j=(idx-k*nxy)/nx;
  i=idx-k*nxy-j*nx;
}

//*****************************************************************************
template <typename T>
void linspace(T lo, T hi, int n, T *data)
{
  // generate n equally spaced points on the segment [lo hi] on real line
  // R^1.

  if (n==1)
    {
    data[0]=(hi+lo)/((T)2);
    return;
    }

  T delta=(hi-lo)/((T)(n-1));

  for (int i=0; i<n; ++i)
    {
    data[i]=lo+((T)i)*delta;
    }
}

//*****************************************************************************
template <typename Ti, typename To>
void linspace(Ti X0[3], Ti X1[3], int n, To *X)
{
  // generate n equally spaced points on the line segment [X0 X1] in R^3.

  if (n==1)
    {
    X[0]=(To)((X1[0]+X0[0])/Ti(2));
    X[1]=(To)((X1[1]+X0[1])/Ti(2));
    X[2]=(To)((X1[2]+X0[2])/Ti(2));
    return;
    }

  Ti dX[3]={
    (X1[0]-X0[0])/((Ti)(n-1)),
    (X1[1]-X0[1])/((Ti)(n-1)),
    (X1[2]-X0[2])/((Ti)(n-1))};

  for (int i=0; i<n; ++i)
    {
    X[0]=(To)(X0[0]+((Ti)i)*dX[0]);
    X[1]=(To)(X0[1]+((Ti)i)*dX[1]);
    X[2]=(To)(X0[2]+((Ti)i)*dX[2]);
    X+=3;
    }
}

//*****************************************************************************
template <typename T>
void logspace(T lo, T hi, int n, T p, T *data)
{
  // generate n log spaced points inbetween lo and hi on
  // the real line (R^1). The variation in the spacing is
  // symetric about the mid point of the [lo hi] range.

  int mid=n/2;
  int nlo=mid;
  int nhi=n-mid;
  T s=hi-lo;

  T rhi=(T)pow(((T)10),p);

  linspace<T>(((T)1),T(0.99)*rhi,nlo,data);
  linspace<T>(((T)1),rhi,nhi,data+nlo);

  int i=0;
  for (; i<nlo; ++i)
    {
    data[i]=lo+s*(T(0.5)*((T)log10(data[i]))/p);
    }
  for (; i<n; ++i)
    {
    data[i]=lo+s*(((T)1)-((T)log10(data[i]))/(((T)2)*p));
    }
}

//*****************************************************************************
template <typename T>
T Interpolate(double t, T v0, T v1)
{
  T w=(((T)1)-T(t))*v0 + T(t)*v1;
  return w;
}

//*****************************************************************************
template <typename T>
T Interpolate(double t0, double t1, T v0, T v1, T v2, T v3)
{
  T w0=Interpolate(t0,v0,v1);
  T w1=Interpolate(t0,v2,v3);
  T w2=Interpolate(t1,w0,w1);
  return w2;
}

//*****************************************************************************
template <typename T>
T Interpolate(
      double t0,
      double t1,
      double t2,
      T v0,
      T v1,
      T v2,
      T v3,
      T v4,
      T v5,
      T v6,
      T v7)
{
  T w0=Interpolate(t0,t1,v0,v1,v2,v3);
  T w1=Interpolate(t0,t1,v4,v5,v6,v7);
  T w2=Interpolate(t2,w0,w1);
  return w2;
}

//=============================================================================
template <typename T>
class CentralStencil
{
public:
  CentralStencil(int ni, int nj, int nk, int nComps, T *v)
       :
      Ni(ni),Nj(nj),Nk(nk),NiNj(ni*nj),
      NComps(nComps),
      Vilo(0),Vihi(0),Vjlo(0),Vjhi(0),Vklo(0),Vkhi(0),
      V(v)
       {}

  void SetCenter(int i, int j, int k)
    {
    this->Vilo=this->NComps*(k*this->NiNj+j*this->Ni+(i-1));
    this->Vihi=this->NComps*(k*this->NiNj+j*this->Ni+(i+1));
    this->Vjlo=this->NComps*(k*this->NiNj+(j-1)*this->Ni+i);
    this->Vjhi=this->NComps*(k*this->NiNj+(j+1)*this->Ni+i);
    this->Vklo=this->NComps*((k-1)*this->NiNj+j*this->Ni+i);
    this->Vkhi=this->NComps*((k+1)*this->NiNj+j*this->Ni+i);
    }

  // center
  // T *Operator()()
  //   {
  //   return this->V+this->Vilo+this->NComps;
  //   }

  // i direction
  T ilo(int comp)
    {
    return this->V[this->Vilo+comp];
    }
  T ihi(int comp)
    {
    return this->V[this->Vihi+comp];
    }
  // j direction
  T jlo(int comp)
    {
    return this->V[this->Vjlo+comp];
    }
  T jhi(int comp)
    {
    return this->V[this->Vjhi+comp];
    }
  // k-direction
  T klo(int comp)
    {
    return this->V[this->Vklo+comp];
    }
  T khi(int comp)
    {
    return this->V[this->Vkhi+comp];
    }


private:
  CentralStencil();
private:
  int Ni,Nj,Nk,NiNj;
  int NComps;
  int Vilo,Vihi,Vjlo,Vjhi,Vklo,Vkhi;
  T *V;
};

//*****************************************************************************
template<typename T>
void slowSort(T *a, int l, int r)
{
  for (int i=l; i<r; ++i)
    {
    for (int j=i; j>l; --j)
      {
      if (a[j]>a[j-1])
        {
        T tmp=a[j-1];
        a[j-1]=a[j];
        a[j]=tmp;
        }
      }
    }
}

//*****************************************************************************
template<typename T, int nComp>
bool IsNan(T *V)
{
  bool nan=false;
  for (int i=0; i<nComp; ++i)
    {
    if (isnan(V[i]))
      {
      nan=true;
      break;
      }
    }
  return nan;
}

//*****************************************************************************
template<typename T, int nComp>
void Init(T *V, T *V_0, int n)
{
  for (int i=0; i<n; ++i, V+=nComp)
    {
    for (int j=0; j<nComp; ++j)
      {
      V[j]=V_0[j];
      }
    }
}

//*****************************************************************************
template<typename T, int nComp>
bool Find(int *I, T *V, T *val)
{
  bool has=false;

  int ni=I[0];
  int nj=I[1];
  int ninj=ni*nj;

  for (int k=0; k<I[2]; ++k)
    {
    for (int j=0; j<I[1]; ++j)
      {
      for (int i=0; i<I[0]; ++i)
        {
        int q=k*ninj+j*ni+i;
        int hit=0;

        for (int p=0; p<nComp; ++p)
          {
          if (V[q+p]==val[p])
            {
            ++hit;
            }
          }

         // match only if all comps match.
         if (hit==nComp)
          {
          has=true;
          std::cerr
            << __LINE__ << " FOUND val=" << val
            << " at " << Tuple<int>(i,j,k) << std::endl;
          }
        }
      }
    }
  return has;
}

//*****************************************************************************
template<typename T, int nComp>
bool HasNans(int *I, T *V, T *val)
{
  (void)val;

  bool has=false;

  int ni=I[0];
  int nj=I[1];
  int ninj=ni*nj;

  for (int k=0; k<I[2]; ++k)
    {
    for (int j=0; j<I[1]; ++j)
      {
      for (int i=0; i<I[0]; ++i)
        {
        int q=k*ninj+j*ni+i;
        for (int p=0; p<nComp; ++p)
          {
          if (isnan(V[q+p]))
            {
            has=true;
            std::cerr
              << __LINE__ << " ERROR NAN. "
              << "I+" << q+p <<"=" << Tuple<int>(i,j,k)
              << std::endl;
            }
          }
        }
      }
    }
  return has;
}

// I  -> number of points
// V  -> vector field
// mV -> Magnitude
//*****************************************************************************
template <typename T>
void Magnitude(int *I, T *  V, T *  mV)
{
  for (int k=0; k<I[2]; ++k)
    {
    for (int j=0; j<I[1]; ++j)
      {
      for (int i=0; i<I[0]; ++i)
        {
        const int p  = k*I[0]*I[1]+j*I[0]+i;
        const int vi = 3*p;
        const int vj = vi + 1;
        const int vk = vi + 2;
        mV[p]=sqrt(V[vi]*V[vi]+V[vj]*V[vj]+V[vk]*V[vk]);
        }
      }
    }
}

// Magnitude of a vector
//*****************************************************************************
template <typename T>
void Magnitude(
      size_t n,
      T * __restrict__ V,
      T * __restrict__ mV)
{
  for (size_t q=0; q<n; ++q)
    {
    size_t qq=3*q;
    mV[q] = ((T)sqrt(V[qq]*V[qq]+V[qq+1]+V[qq+1]+V[qq+2]*V[qq+2]));
    }
}

// Magnitude of a vector
//*****************************************************************************
template <typename T>
void Magnitude(
      size_t nt, // number of tuples
      size_t nc, // number of components
      T * __restrict__ V,
      T * __restrict__ mV)
{
  for (size_t q=0; q<nt; ++q)
    {
    size_t qq=nc*q;
    T vv=((T)0);
    for (size_t c=0; c<nc; ++c)
      {
      size_t r=qq+c;
      vv+=V[r]*V[r];
      }
    mV[q] = ((T)sqrt(vv));
    }
}

// Difference of two arrays D=A-B
//*****************************************************************************
template <typename T>
void Difference(
      size_t nt, // number of tuples
      size_t nc, // number of components
      T * __restrict__ A,
      T * __restrict__ B,
      T * __restrict__ D)
{
  for (size_t q=0; q<nt; ++q)
    {
    size_t qq=nc*q;
    for (size_t c=0; c<nc; ++c)
      {
      size_t r=qq+c;
      D[r]=A[r]-B[r];
      }
    }
}

//*****************************************************************************
template<typename T>
void Split(
      int c,
      size_t n,
      int nComp,
      T * __restrict__  V,
      T * __restrict__  Vc)
{
  // take vector array and split a component into a scalar array.
  for (size_t i=0; i<n; ++i)
    {
    size_t ii=nComp*i;
    Vc[i]=V[ii+c];
    }
}

//*****************************************************************************
template<typename T>
void Split(
      size_t n,
      T * __restrict__  V,
      T * __restrict__  Vx,
      T * __restrict__  Vy,
      T * __restrict__  Vz)
{
  // take vector array and split into 3 scalar arrays.
  for (size_t i=0; i<n; ++i)
    {
    size_t ii=3*i;
    Vx[i]=V[ii  ];
    Vy[i]=V[ii+1];
    Vz[i]=V[ii+2];
    }
}

//*****************************************************************************
template<typename T>
void Split(
      int n,
      T * __restrict__  V,
      T * __restrict__  Vxx,
      T * __restrict__  Vxy,
      T * __restrict__  Vxz,
      T * __restrict__  Vyx,
      T * __restrict__  Vyy,
      T * __restrict__  Vyz,
      T * __restrict__  Vzx,
      T * __restrict__  Vzy,
      T * __restrict__  Vzz)
{
  // take scalar components and interleve into a vector array.
  for (int i=0; i<n; ++i)
    {
    int ii=9*i;
    Vxx[i]=V[ii  ];
    Vxy[i]=V[ii+1];
    Vxz[i]=V[ii+2];
    Vyx[i]=V[ii+3];
    Vyy[i]=V[ii+4];
    Vyz[i]=V[ii+5];
    Vzx[i]=V[ii+6];
    Vzy[i]=V[ii+7];
    Vzz[i]=V[ii+8];
    }
}

//*****************************************************************************
template<typename T>
void Interleave(
      size_t n,
      T * __restrict__  Vx,
      T * __restrict__  Vy,
      T * __restrict__  Vz,
      T * __restrict__  V)
{
  // take scalar components and interleve into a vector array.
  for (size_t i=0; i<n; ++i)
    {
    size_t ii=3*i;
    V[ii  ]=Vx[i];
    V[ii+1]=Vy[i];
    V[ii+2]=Vz[i];
    }
}

//*****************************************************************************
template<typename T>
void Interleave(
      int n,
      T * __restrict__  Vxx,
      T * __restrict__  Vxy,
      T * __restrict__  Vxz,
      T * __restrict__  Vyx,
      T * __restrict__  Vyy,
      T * __restrict__  Vyz,
      T * __restrict__  Vzx,
      T * __restrict__  Vzy,
      T * __restrict__  Vzz,
      T * __restrict__  V)
{
  // take scalar components and interleve into a vector array.
  for (int i=0; i<n; ++i)
    {
    int ii=9*i;
    V[ii  ]=Vxx[i];
    V[ii+1]=Vxy[i];
    V[ii+2]=Vxz[i];
    V[ii+3]=Vyx[i];
    V[ii+4]=Vyy[i];
    V[ii+5]=Vyz[i];
    V[ii+6]=Vzx[i];
    V[ii+7]=Vzy[i];
    V[ii+8]=Vzz[i];
    }
}

// input  -> input(src) patch bounds
// output -> output(dest) patch bounds
// V      -> input(src) data
// W      -> output(dest) data
// nComp  -> number of sclar components
//*****************************************************************************
#define USE_INPUT_BOUNDS true
#define USE_OUTPUT_BOUNDS false
template <typename T>
void Copy(
      int *input,
      int *output,
      T*  V,
      T*  W,
      int nComp,
      int mode,
      bool inputBounds=true)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // use the smaller of the input and output for
  // loop bounds.
  int bounds[6];
  if (inputBounds)
    {
    memcpy(bounds,input,6*sizeof(int));
    }
  else
    {
    memcpy(bounds,output,6*sizeof(int));
    }

  // loop over input in patch coordinates (both patches are in the same space)
  for (int r=bounds[4]; r<=bounds[5]; ++r)
    {
    const int _k=r-output[4];
    const int  k=r-input[4];
    for (int q=bounds[2]; q<=bounds[3]; ++q)
      {
      const int _j=q-output[2];
      const int  j=q-input[2];
      for (int p=bounds[0]; p<=bounds[1]; ++p)
        {
        const int _i=p-output[0];
        const int  i=p-input[0];

        size_t _vi=nComp*_idx.Index(_i,_j,_k);
        size_t  vi=nComp*idx.Index(i,j,k);

        // copy components
        for (int c=0; c<nComp; ++c)
          {
          W[_vi+c] = V[vi+c];
          }
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// nComp  -> number of components in V
// V      -> input patch scalar or vector field
// W      -> output patch scalar or vector field
// D      -> output patch scalar or vector field
//*****************************************************************************
template <typename T>
void Difference(
      int *input,
      int *output,
      int nComp,
      int mode,
      T* __restrict__  V,
      T* __restrict__  W,
      T* __restrict__  D)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int _k=r-output[4];
    const int  k=r-input[4];

    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int _j=q-output[2];
      const int  j=q-input[2];

      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int _i=p-output[0];
        const int  i=p-input[0];

        const size_t _pi=nComp*_idx.Index(_i,_j,_k);

        size_t vi = nComp*idx.Index(i,j,k);

        for (int c=0; c<nComp; ++c)
          {
          D[_pi+c] = V[vi+c] - W[_pi+c];
          }
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// K      -> kernel (square matrix whose sum is 1)
// nk     -> number of rows in K
// V      -> scalar or vector field
// nComp  -> number of components in V
// W      -> convolution of V and K
// dim    -> dim, 2d or 3d
//*****************************************************************************
template <typename T>
void Convolution(
      int *input,
      int *output,
      int *kernel,
      int nComp,
      int mode,
      T* __restrict__  V,
      T* __restrict__  W,
      float * __restrict__ K)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // kernel dimensions
  const int kni=kernel[1]-kernel[0]+1;
  const int knj=kernel[3]-kernel[2]+1;
  const int knk=kernel[5]-kernel[4]+1;
  FlatIndex kidx(kni,knj,knk,mode);

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int _k=r-output[4];
    const int  k=r-input[4];

    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int _j=q-output[2];
      const int  j=q-input[2];

      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int _i=p-output[0];
        const int  i=p-input[0];

        const size_t _pi=nComp*_idx.Index(_i,_j,_k);

        // intialize the output
        for (int c=0; c<nComp; ++c)
          {
          W[_pi+c] = ((T)0);
          }

        for (int h=kernel[4]; h<=kernel[5]; ++h)
          {
          const int kk=h-kernel[4];

          for (int g=kernel[2]; g<=kernel[3]; ++g)
            {
            const int kj=g-kernel[2];

            for (int f=kernel[0]; f<=kernel[1]; ++f)
              {
              const int ki=f-kernel[0];
              size_t kii = kidx.Index(ki,kj,kk);

              size_t vi = nComp*idx.Index(i+f,j+g,k+h);

              for (int c=0; c<nComp; ++c)
                {
                W[_pi+c] += V[vi+c]*((T)K[kii]);
                }
              }
            }
          }
        }
      }
    }
}

/**
This implementation is written so that adjacent threads access adjacent
memory locations. This requires that vtk vectors/tensors etc be split.
*/
//*****************************************************************************
template<typename T>
void ScalarConvolution2D(
      //int worldRank,
      size_t vni,
      size_t wni,
      size_t wnij,
      size_t kni,
      size_t knij,
      size_t nGhost,
      T * __restrict__ V,
      T * __restrict__ W,
      float * __restrict__ K)
{
  (void)knij;
  (void)nGhost;

  // get a tuple from the current flat index in the output
  // index space
  for (size_t wi=0; wi<wnij; ++wi)
    {
    size_t i,j;
    j=wi/wni;
    i=wi-j*wni;

    // compute using the aligned buffers
    T w=(0);
    for (size_t g=0; g<kni; ++g)
      {
      size_t b=kni*g;
      size_t q=vni*(j+g)+i;
      for (size_t f=0; f<kni; ++f)
        {
        size_t vi=q+f;
        size_t ki=b+f;
        w+=V[vi]*((T)K[ki]);
        }
      }
    W[wi]=w;
    }
}

//*****************************************************************************
template<typename T>
void ScalarConvolution3D(
      size_t vni,
      size_t vnij,
      size_t wni,
      size_t wnij,
      size_t wnijk,
      size_t kni,
      size_t knij,
      size_t knijk,
      size_t nGhost,
      T * __restrict__ V,
      T * __restrict__ W,
      float * __restrict__ K)
{
  (void)knijk;
  (void)nGhost;

  // visit each output element
  for (size_t wi=0; wi<wnijk; ++wi)
    {
    size_t i,j,k;
    k=wi/wnij;
    j=(wi-k*wnij)/wni;
    i=wi-k*wnij-j*wni;

    // compute convolution
    T w=((T)0);
    for (size_t h=0; h<kni; ++h)
      {
      size_t c=knij*h;
      size_t r=vnij*(k+h);
      for (size_t g=0; g<kni; ++g)
        {
        size_t b=c+kni*g;
        size_t q=r+vni*(j+g)+i;
        for (size_t f=0; f<kni; ++f)
          {
          size_t ki=b+f;
          size_t vi=q+f;

          w+=V[vi]*((T)K[ki]);
          }
        }
      }

    W[wi]=w;
    }
}

/*
this vectorized version is slightly SLOWER than then unoptimized version

// ****************************************************************************
template<typename T>
void ScalarConvolution2D(
      //int worldRank,
      size_t vni,
      size_t wni,
      size_t wnij,
      size_t kni,
      size_t knij,
      size_t nGhost,
      T * __restrict__ V,
      T * __restrict__ W,
      float * __restrict__ K)
{
  // buffers for vectorized inner loop
  size_t knij4=knij+4-knij%4;
  size_t knij4b=knij4*sizeof(float);
  float * __restrict__ aK=0;
  posix_memalign((void**)&aK,16,knij4b);
  memset(aK,0,knij4b);
  for (size_t ki=0; ki<knij; ++ki)
    {
    aK[ki]=K[ki];
    }

  float * __restrict__ aV=0;
  posix_memalign((void**)&aV,16,knij4b);
  memset(aV,0,knij4b);

  // get a tuple from the current flat index in the output
  // index space
  for (size_t wi=0; wi<wnij; ++wi)
    {
    size_t i,j;
    j=wi/wni;
    i=wi-j*wni;

    // move input elements to the aligned buffer
    size_t avi=0;
    for (size_t g=0; g<kni; ++g)
      {
      size_t q=vni*(j+g)+i;
      for (size_t f=0; f<kni; ++f)
        {
        size_t vi=q+f;
        aV[avi]=V[vi];
        ++avi;
        }
      }

    // compute using the aligned buffers
    float w=((T)0);
    for (size_t ki=0; ki<knij4; ++ki)
      {
      w=w+aV[ki]*aK[ki];
      }

    W[wi]=w;
    }

  free(aV);
  free(aK);
}

// ****************************************************************************
template<typename T>
void ScalarConvolution3D(
      size_t vni,
      size_t vnij,
      size_t wni,
      size_t wnij,
      size_t wnijk,
      size_t kni,
      size_t knij,
      size_t knijk,
      size_t nGhost,
      T * __restrict__ V,
      T * __restrict__ W,
      float * __restrict__ K)
{
  // buffers for vectorized inner loop
  size_t knijk4=knijk+4-knijk%4;
  size_t knijk4b=knijk4*sizeof(float);
  float * __restrict__ aK=0;
  posix_memalign((void**)&aK,16,knijk4b);
  memset(aK,0,knijk4b);
  for (size_t ki=0; ki<knijk; ++ki)
    {
    aK[ki]=K[ki];
    }

  float * __restrict__ aV=0;
  posix_memalign((void**)&aV,16,knijk4b);
  memset(aV,0,knijk4b);

  // visit each output element
  for (size_t wi=0; wi<wnijk; ++wi)
    {
    size_t i,j,k;
    k=wi/wnij;
    j=(wi-k*wnij)/wni;
    i=wi-k*wnij-j*wni;

    // move input data into aligned buffer
    size_t avi=0;
    for (size_t h=0; h<kni; ++h)
      {
      size_t r=vnij*(k+h);
      for (size_t g=0; g<kni; ++g)
        {
        size_t q=r+vni*(j+g)+i;
        for (size_t f=0; f<kni; ++f)
          {
          size_t vi=q+f;

          aV[avi]=V[vi];
          ++avi;
          }
        }
      }

    // compute convolution
    float w=((T)0);
    for (size_t ki=0; ki<knijk4; ++ki)
      {
      w=w+aV[ki]*aK[ki];
      }

    W[wi]=w;
    }

  free(aV);
  free(aK);
}
*/


/**
Functor for comapring array values by index
*/
template<typename T>
class IndirectCompare
{
public:
  //
  IndirectCompare() : Data(0) {}
  IndirectCompare(T *data) : Data(data) {}

  // compare data at the given indices
  bool operator()(size_t l, size_t r)
  { return this->Data[l]<this->Data[r]; }

private:
  T *Data;
};

/**
This implementation is written so that adjacent threads access adjacent
memory locations. This requires that vtk vectors/tensors etc be split.
*/
//*****************************************************************************
template<typename T>
void ScalarMedianFilter2D(
      //int worldRank,
      size_t vni,
      size_t wni,
      size_t wnij,
      size_t kni,
      size_t knij,
      size_t nGhost,
      T * __restrict__ V,
      T * __restrict__ W)
{
  (void)nGhost;

  size_t *ids=0;
  posix_memalign((void**)&ids,16,knij*sizeof(size_t));

  IndirectCompare<T> comp(V);

  // get a tuple from the current flat index in the output
  // index space
  for (size_t wi=0; wi<wnij; ++wi)
    {
    size_t i,j;
    j=wi/wni;
    i=wi-j*wni;

    // setup search space
    size_t ki=0;
    for (size_t g=0; g<kni; ++g)
      {
      size_t q=vni*(j+g)+i;
      for (size_t f=0; f<kni; ++f)
        {
        size_t vi=q+f;
        ids[ki]=vi;
        ++ki;
        }
      }

    // sort
    //std::sort(ids,ids+knij,comp);
    std::partial_sort(ids,ids+knij/2+1,ids+knij,comp);

    // std::cerr << wi << " " << V[ids[0]] << " " << V[ids[knij/2]] << " " << V[ids[knij-1]] << std::endl;

    // median
    W[wi]=V[ids[knij/2]];
    }

  free(ids);
}

//*****************************************************************************
template<typename T>
void ScalarMedianFilter3D(
      size_t vni,
      size_t vnij,
      size_t wni,
      size_t wnij,
      size_t wnijk,
      size_t kni,
      size_t knij,
      size_t knijk,
      size_t nGhost,
      T * __restrict__ V,
      T * __restrict__ W)
{
  (void)knij;
  (void)nGhost;

  size_t *ids=0;
  posix_memalign((void**)&ids,16,knijk*sizeof(size_t));

  IndirectCompare<T> comp(V);

  // visit each output element
  for (size_t wi=0; wi<wnijk; ++wi)
    {
    size_t i,j,k;
    k=wi/wnij;
    j=(wi-k*wnij)/wni;
    i=wi-k*wnij-j*wni;

    // set up search space
    size_t ki=0;
    for (size_t h=0; h<kni; ++h)
      {
      size_t r=vnij*(k+h);
      for (size_t g=0; g<kni; ++g)
        {
        size_t q=r+vni*(j+g)+i;
        for (size_t f=0; f<kni; ++f)
          {
          size_t vi=q+f;
          ids[ki]=vi;
          ++ki;
          }
        }
      }

    // sort
    //std::sort(ids,ids+knijk,comp);
    std::partial_sort(ids,ids+knijk/2+1,ids+knijk,comp);

    // median
    W[wi]=V[ids[knijk/2]];
    }

  free(ids);
}


//*****************************************************************************
template <typename T>
void DivergenceFace(int *I, double *dX, T *V, T *mV, T *div)
{
  // *hi variables are number of cells in the out cell centered
  // array. The in array is a point centered array of face data
  // with the last face left off.
  const int pihi=I[0]+1;
  const int pjhi=I[1]+1;
  // const int pkhi=I[2]+1;

  for (int k=0; k<I[2]; ++k)
    {
    for (int j=0; j<I[1]; ++j)
      {
      for (int i=0; i<I[0]; ++i)
        {
        const int c=k*I[0]*I[1]+j*I[0]+i;
        const int p=k*pihi*pjhi+j*pihi+i;

        const int vilo = 3 * (k*pihi*pjhi+j*pihi+ i   );
        const int vihi = 3 * (k*pihi*pjhi+j*pihi+(i+1));
        const int vjlo = 3 * (k*pihi*pjhi+   j *pihi+i) + 1;
        const int vjhi = 3 * (k*pihi*pjhi+(j+1)*pihi+i) + 1;
        const int vklo = 3 * (   k *pihi*pjhi+j*pihi+i) + 2;
        const int vkhi = 3 * ((k+1)*pihi*pjhi+j*pihi+i) + 2;

        //std::cerr << "(" << vilo << ", " << vihi << ", " << vjlo << ", " << vjhi << ", " << vklo << ", " << vkhi << ")" << std::endl;

        // const double modV=mV[cId];
        // (sqrt(V[vilo]*V[vilo] + V[vjlo]*V[vjlo] + V[vklo]*V[vklo])
        // + sqrt(V[vihi]*V[vihi] + V[vjhi]*V[vjhi] + V[vkhi]*V[vkhi]))/((T)2);

        div[c] =(V[vihi]-V[vilo])/dX[0]/mV[p];
        div[c]+=(V[vjhi]-V[vjlo])/dX[1]/mV[p];
        div[c]+=(V[vkhi]-V[vklo])/dX[2]/mV[p];
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// W      -> vector curl
//*****************************************************************************
template <typename T>
void Rotation(
      int *input,
      int *output,
      int mode,
      double *dX,
      T *V,
      T *Wx,
      T *Wy,
      T *Wz)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const T dx[3]={
      ((T)dX[0])*((T)2),
      ((T)dX[1])*((T)2),
      ((T)dX[2])*((T)2)};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int  k=r-input[4];
    const int _k=r-output[4];

    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int  j=q-input[2];
      const int _j=q-output[2];

      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int  i=p-input[0];
        const int _i=p-output[0];

        const size_t _pi=_idx.Index(_i,_j,_k);

        //      __   ->
        //  w = \/ x V
        Wx[_pi]=((T)0);
        Wy[_pi]=((T)0);
        Wz[_pi]=((T)0);
        if (iok)
          {
          size_t vilo_y=3*idx.Index(i-1,j,k)+1;
          size_t vilo_z=vilo_y+1;

          size_t vihi_y=3*idx.Index(i+1,j,k)+1;
          size_t vihi_z=vihi_y+1;

          Wy[_pi] -= (V[vihi_z]-V[vilo_z])/dx[0];
          Wz[_pi] += (V[vihi_y]-V[vilo_y])/dx[0];
          }

        if (jok)
          {
          size_t vjlo_x=3*idx.Index(i,j-1,k);
          size_t vjlo_z=vjlo_x+2;

          size_t vjhi_x=3*idx.Index(i,j+1,k);
          size_t vjhi_z=vjhi_x+2;

          Wx[_pi] += (V[vjhi_z]-V[vjlo_z])/dx[1];
          Wz[_pi] -= (V[vjhi_x]-V[vjlo_x])/dx[1];
          }

        if (kok)
          {
          size_t vklo_x=3*idx.Index(i,j,k-1);
          size_t vklo_y=vklo_x+1;

          size_t vkhi_x=3*idx.Index(i,j,k+1);
          size_t vkhi_y=vkhi_x+1;

          Wx[_pi] -= (V[vkhi_y]-V[vklo_y])/dx[2];
          Wy[_pi] += (V[vkhi_x]-V[vklo_x])/dx[2];
          }
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// W      -> vector curl
//*****************************************************************************
template <typename TP, typename TD>
void Rotation(
      int *input,
      int *output,
      TP *x,
      TP *y,
      TP *z,
      TD *V,
      TD *Wx,
      TD *Wy,
      TD *Wz)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _ninj=_ni*_nj;

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // stencil deltas
        const TP dx[3]
          = {x[p+1]-x[p-1],y[q+1]-y[q-1],z[r+1]-z[r-1]};

        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int pi=_k*_ninj+_j*_ni+_i;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil into the input array
        const int vilo=3*(k*ninj+j*ni+(i-1));
        const int vihi=3*(k*ninj+j*ni+(i+1));
        const int vjlo=3*(k*ninj+(j-1)*ni+i);
        const int vjhi=3*(k*ninj+(j+1)*ni+i);
        const int vklo=3*((k-1)*ninj+j*ni+i);
        const int vkhi=3*((k+1)*ninj+j*ni+i);

        //      __   ->
        //  w = \/ x V
        Wx[pi]=T((V[vjhi+2]-V[vjlo+2])/dx[1]-(V[vkhi+1]-V[vklo+1])/dx[2]);
        Wy[pi]=T((V[vkhi  ]-V[vklo  ])/dx[2]-(V[vihi+2]-V[vilo+2])/dx[0]);
        Wz[pi]=T((V[vihi+1]-V[vilo+1])/dx[0]-(V[vjhi  ]-V[vjlo  ])/dx[1]);
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// H      -> helicity
//*****************************************************************************
template <typename T>
void Helicity(
      int *input,
      int *output,
      int mode,
      double *dX,
      T *V,
      T *H)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const T dx[3]={
      ((T)dX[0])*((T)2),
      ((T)dX[1])*((T)2),
      ((T)dX[2])*((T)2)};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int _k=r-output[4];
    const int  k=r-input[4];
    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int _j=q-output[2];
      const int  j=q-input[2];
      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int _i=p-output[0];
        const int  i=p-input[0];

        //      __   ->
        //  w = \/ x V
        T wx=((T)0);
        T wy=((T)0);
        T wz=((T)0);
        if (iok)
          {
          size_t vilo_y=3*idx.Index(i-1,j,k)+1;
          size_t vilo_z=vilo_y+1;

          size_t vihi_y=3*idx.Index(i+1,j,k)+1;
          size_t vihi_z=vihi_y+1;

          wy -= (V[vihi_z]-V[vilo_z])/dx[0];
          wz += (V[vihi_y]-V[vilo_y])/dx[0];
          }

        if (jok)
          {
          size_t vjlo_x=3*idx.Index(i,j-1,k);
          size_t vjlo_z=vjlo_x+2;

          size_t vjhi_x=3*idx.Index(i,j+1,k);
          size_t vjhi_z=vjhi_x+2;

          wx += (V[vjhi_z]-V[vjlo_z])/dx[1];
          wz -= (V[vjhi_x]-V[vjlo_x])/dx[1];
          }

        if (kok)
          {
          size_t vklo_x=3*idx.Index(i,j,k-1);
          size_t vklo_y=vklo_x+1;

          size_t vkhi_x=3*idx.Index(i,j,k+1);
          size_t vkhi_y=vkhi_x+1;

          wx -= (V[vkhi_y]-V[vklo_y])/dx[2];
          wy += (V[vkhi_x]-V[vklo_x])/dx[2];
          }

        const size_t pi=_idx.Index(_i,_j,_k);

        const size_t vi=3*idx.Index(i,j,k);;
        const size_t vj=vi+1;
        const size_t vk=vj+1;

        //        ->  ->
        // H =  V . w
        H[pi]=(V[vi]*wx+V[vj]*wy+V[vk]*wz);
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// H      -> helicity
//*****************************************************************************
template <typename TP, typename TD>
void Helicity(int *input, int *output, TP *x, TP *y, TP *z, TD *V, TD *H)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _ninj=_ni*_nj;

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // stencil deltas
        const TD dx[3] = {
            (TD)(x[p+1]-x[p-1]),
            (TD)(y[q+1]-y[q-1]),
            (TD)(z[r+1]-z[r-1])};

        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int pi=_k*_ninj+_j*_ni+_i;
        const int vi=3*pi;
        const int vj=vi+1;
        const int vk=vi+2;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil
        const int vilo=3*(k*ninj+j*ni+(i-1));
        const int vihi=3*(k*ninj+j*ni+(i+1));
        const int vjlo=3*(k*ninj+(j-1)*ni+i);
        const int vjhi=3*(k*ninj+(j+1)*ni+i);
        const int vklo=3*((k-1)*ninj+j*ni+i);
        const int vkhi=3*((k+1)*ninj+j*ni+i);

        //      __   ->
        //  w = \/ x V
        const TD w[3]={
              (V[vjhi+2]-V[vjlo+2])/dx[1]-(V[vkhi+1]-V[vklo+1])/dx[2],
              (V[vkhi  ]-V[vklo  ])/dx[2]-(V[vihi+2]-V[vilo+2])/dx[0],
              (V[vihi+1]-V[vilo+1])/dx[0]-(V[vjhi  ]-V[vjlo  ])/dx[1]
              };
        //        ->  ->
        // H =  V . w
        H[pi]=(V[vi]*w[0]+V[vj]*w[1]+V[vk]*w[2]);
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// H      -> normalized helicity(out)
//*****************************************************************************
template <typename T>
void NormalizedHelicity(
    int *input,
    int *output,
    int mode,
    double *dX,
    T *V,
    T *H)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const T dx[3]={
      ((T)dX[0])*((T)2),
      ((T)dX[1])*((T)2),
      ((T)dX[2])*((T)2)};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int _k=r-output[4];
    const int  k=r-input[4];
    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int _j=q-output[2];
      const int  j=q-input[2];
      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int _i=p-output[0];
        const int  i=p-input[0];

        //      __   ->
        //  w = \/ x V
        T wx=((T)0);
        T wy=((T)0);
        T wz=((T)0);
        if (iok)
          {
          size_t vilo_y=3*idx.Index(i-1,j,k)+1;
          size_t vilo_z=vilo_y+1;

          size_t vihi_y=3*idx.Index(i+1,j,k)+1;
          size_t vihi_z=vihi_y+1;

          wy -= (V[vihi_z]-V[vilo_z])/dx[0];
          wz += (V[vihi_y]-V[vilo_y])/dx[0];
          }

        if (jok)
          {
          size_t vjlo_x=3*idx.Index(i,j-1,k);
          size_t vjlo_z=vjlo_x+2;

          size_t vjhi_x=3*idx.Index(i,j+1,k);
          size_t vjhi_z=vjhi_x+2;

          wx += (V[vjhi_z]-V[vjlo_z])/dx[1];
          wz -= (V[vjhi_x]-V[vjlo_x])/dx[1];
          }

        if (kok)
          {
          size_t vklo_x=3*idx.Index(i,j,k-1);
          size_t vklo_y=vklo_x+1;

          size_t vkhi_x=3*idx.Index(i,j,k+1);
          size_t vkhi_y=vkhi_x+1;

          wx -= (V[vkhi_y]-V[vklo_y])/dx[2];
          wy += (V[vkhi_x]-V[vklo_x])/dx[2];
          }

        //  ->
        // |w|
        const T modW=((T)sqrt(wx*wx+wy*wy+wz*wz));

        const size_t vi=3*idx.Index(i,j,k);
        const size_t vj=vi+1;
        const size_t vk=vj+1;

        //  ->
        // |V|
        const T modV
          = ((T)sqrt(V[vi]*V[vi]+V[vj]*V[vj]+V[vk]*V[vk]));

        const size_t pi=_idx.Index(_i,_j,_k);

        //         ->  ->     -> ->
        // H_n = ( V . w ) / |V||w|
        H[pi]=(V[vi]*wx+V[vj]*wy+V[vk]*wz)/(modV*modW);
        // Cosine of the angle between v and w. Angle between v and w is small
        // near vortex, H_n = +-1.
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// H      -> normalized helicity(out)
//*****************************************************************************
template <typename TP, typename TD>
void NormalizedHelicity(
      int *input,
      int *output,
      TP *x,
      TP *y,
      TP *z,
      TD *V,
      TD *H)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _ninj=_ni*_nj;

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // stencil deltas
        const TD dx[3] = {
            ((TD)(x[p+1]-x[p-1]))
            ((TD)(y[q+1]-y[q-1]))
            ((TD)(z[r+1]-z[r-1]))};

        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int pi=_k*_ninj+_j*_ni+_i;

        // TODO vi is input pi is output
        const int vi=3*pi;
        const int vj=vi+1;
        const int vk=vi+2;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil
        const int vilo=3*(k*ninj+j*ni+(i-1));
        const int vihi=3*(k*ninj+j*ni+(i+1));
        const int vjlo=3*(k*ninj+(j-1)*ni+i);
        const int vjhi=3*(k*ninj+(j+1)*ni+i);
        const int vklo=3*((k-1)*ninj+j*ni+i);
        const int vkhi=3*((k+1)*ninj+j*ni+i);

        //  ->
        // |V|
        const TD modV
          = sqrt(V[vi]*V[vi]+V[vj]*V[vj]+V[vk]*V[vk]);

        //      __   ->
        //  w = \/ x V
        const TD w[3]={
              (V[vjhi+2]-V[vjlo+2])/dx[1]-(V[vkhi+1]-V[vklo+1])/dx[2],
              (V[vkhi  ]-V[vklo  ])/dx[2]-(V[vihi+2]-V[vilo+2])/dx[0],
              (V[vihi+1]-V[vilo+1])/dx[0]-(V[vjhi  ]-V[vjlo  ])/dx[1]};

        const TD modW=sqrt(w[0]*w[0]+w[1]*w[1]+w[2]*w[2]);

        //         ->  ->     -> ->
        // H_n = ( V . w ) / |V||w|
        H[pi]=(V[vi]*w[0]+V[vj]*w[1]+V[vk]*w[2])/(modV*modW);
        // Cosine of the angle between v and w. Angle between v and w is small
        // near vortex, H_n = +-1.

        // std::cerr
        //   << "H=" << H[pi] << " "
        //   << "modV= " << modV << " "
        //   << "modW=" << modW << " "
        //   << "w=" << Tuple<double>((double *)w,3) << " "
        //   << "V=" << Tuple<T>(&V[vi],3)
        //   << std::endl;
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// L      -> eigenvalues (lambda) of the corrected pressure hessian
//*****************************************************************************
template <typename T>
void Lambda(
      int *input,
      int *output,
      int mode,
      double *dX,
      T *V,
      T *L)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const T dx[3]={
      ((T)dX[0])*((T)2),
      ((T)dX[1])*((T)2),
      ((T)dX[2])*((T)2)};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int _k=r-output[4];
    const int  k=r-input[4];
    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int _j=q-output[2];
      const int  j=q-input[2];
      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int _i=p-output[0];
        const int  i=p-input[0];

        // J: gradient velocity tensor, (jacobian)
        T j11=((T)0), j12=((T)0), j13=((T)0);
        if (iok)
          {
          size_t vilo_x=3*idx.Index(i-1,j,k);
          size_t vilo_y=vilo_x+1;
          size_t vilo_z=vilo_y+1;

          size_t vihi_x=3*idx.Index(i+1,j,k);
          size_t vihi_y=vihi_x+1;
          size_t vihi_z=vihi_y+1;

          j11=(V[vihi_x]-V[vilo_x])/dx[0];
          j12=(V[vihi_y]-V[vilo_y])/dx[0];
          j13=(V[vihi_z]-V[vilo_z])/dx[0];
          }

        T j21=((T)0), j22=((T)0), j23=((T)0);
        if (jok)
          {
          size_t vjlo_x=3*idx.Index(i,j-1,k);
          size_t vjlo_y=vjlo_x+1;
          size_t vjlo_z=vjlo_y+1;

          size_t vjhi_x=3*idx.Index(i,j+1,k);
          size_t vjhi_y=vjhi_x+1;
          size_t vjhi_z=vjhi_y+1;

          j21=(V[vjhi_x]-V[vjlo_x])/dx[1];
          j22=(V[vjhi_y]-V[vjlo_y])/dx[1];
          j23=(V[vjhi_z]-V[vjlo_z])/dx[1];
          }

        T j31=((T)0), j32=((T)0), j33=((T)0);
        if (kok)
          {
          size_t vklo_x=3*idx.Index(i,j,k-1);
          size_t vklo_y=vklo_x+1;
          size_t vklo_z=vklo_y+1;

          size_t vkhi_x=3*idx.Index(i,j,k+1);
          size_t vkhi_y=vkhi_x+1;
          size_t vkhi_z=vkhi_y+1;

          j31=(V[vkhi_x]-V[vklo_x])/dx[2];
          j32=(V[vkhi_y]-V[vklo_y])/dx[2];
          j33=(V[vkhi_z]-V[vklo_z])/dx[2];
          }

        Matrix<T,3,3> J;
        J <<
          j11, j12, j13,
          j21, j22, j23,
          j31, j32, j33;

        // construct pressure corrected hessian
        Matrix<T,3,3> S=0.5*(J+J.transpose());
        Matrix<T,3,3> W=0.5*(J-J.transpose());
        Matrix<T,3,3> HP=S*S+W*W;

        // compute eigen values, lambda
        Matrix<T,3,1> e;
        SelfAdjointEigenSolver<Matrix<T,3,3> >solver(HP,false);
        e=solver.eigenvalues();

        const size_t pi=_idx.Index(_i,_j,_k);
        const size_t vi=3*pi;
        const size_t vj=vi+1;
        const size_t vk=vj+1;

        L[vi]=e(0,0);
        L[vj]=e(1,0);
        L[vk]=e(2,0);

        slowSort(&L[vi],0,3);
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// L      -> eigenvalues (lambda) of the corrected pressure hessian
//*****************************************************************************
template <typename TP, typename TD>
void Lambda(int *input, int *output, TP *x, TP *y, TP *z, TD *V, TD *L)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _ninj=_ni*_nj;

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // stencil deltas
        const TD dx[3] = {
            ((TD)(x[p+1]-x[p-1])),
            ((TD)(y[q+1]-y[q-1])),
            ((TD)(z[r+1]-z[r-1]))};

        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int pi=_k*_ninj+_j*_ni+_i;
        const int vi=3*pi;
        const int vj=vi+1;
        const int vk=vi+2;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil
        const int vilo=3*(k*ninj+j*ni+(i-1));
        const int vihi=3*(k*ninj+j*ni+(i+1));
        const int vjlo=3*(k*ninj+(j-1)*ni+i);
        const int vjhi=3*(k*ninj+(j+1)*ni+i);
        const int vklo=3*((k-1)*ninj+j*ni+i);
        const int vkhi=3*((k+1)*ninj+j*ni+i);

        // J: gradient velocity tensor, (jacobian)
        Matrix<TD,3,3> J;
        J <<
          (V[vihi]-V[vilo])/dx[0], (V[vihi+1]-V[vilo+1])/dx[0], V[vihi+2]-V[vilo+2]/dx[0],
          (V[vjhi]-V[vjlo])/dx[1], (V[vjhi+1]-V[vjlo+1])/dx[1], V[vjhi+2]-V[vjlo+2]/dx[1],
          (V[vkhi]-V[vklo])/dx[2], (V[vkhi+1]-V[vklo+1])/dx[2], V[vkhi+2]-V[vklo+2]/dx[2];

        // construct pressure corrected hessian
        Matrix<TD,3,3> S=((TD)0.5)*(J+J.transpose());
        Matrix<TD,3,3> W=((TD)0.5)*(J-J.transpose());
        Matrix<TD,3,3> HP=S*S+W*W;

        // compute eigen values, lambda
        Matrix<TD,3,1> e;
        SelfAdjointEigenSolver<Matrix<TD,3,3> >solver(HP,false);
        e=solver.eigenvalues();

        L[vi]=e(0,0);
        L[vj]=e(1,0);
        L[vk]=e(2,0);

        L[vi]=(((L[vi]>=((TD)-1E-5))&&(L[vi]<=((TD)1E-5)))?TD(0):L[vi]);
        L[vj]=(((L[vj]>=((TD)-1E-5))&&(L[vj]<=((TD)1E-5)))?TD(0):L[vj]);
        L[vk]=(((L[vk]>=((TD)-1E-5))&&(L[vk]<=((TD)1E-5)))?TD(0):L[vk]);

        slowSort(&L[vi],0,3);
        // std::cerr << L[vi] << ", "  << L[vj] << ", " << L[vk] << std::endl;
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// L      -> second eigenvalues (lambda-2) of the corrected pressure hessian
//*****************************************************************************
template <typename T>
void Lambda2(
      int *input,
      int *output,
      int mode,
      double *dX,
      T *V,
      T *L2)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const T dx[3]={
      ((T)dX[0])*((T)2),
      ((T)dX[1])*((T)2),
      ((T)dX[2])*((T)2)};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int _k=r-output[4];
    const int  k=r-input[4];
    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int _j=q-output[2];
      const int  j=q-input[2];
      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int _i=p-output[0];
        const int  i=p-input[0];

        // J: gradient velocity tensor, (jacobian)
        T j11=((T)0), j12=((T)0), j13=((T)0);
        if (iok)
          {
          size_t vilo_x=3*idx.Index(i-1,j,k);
          size_t vilo_y=vilo_x+1;
          size_t vilo_z=vilo_y+1;

          size_t vihi_x=3*idx.Index(i+1,j,k);
          size_t vihi_y=vihi_x+1;
          size_t vihi_z=vihi_y+1;

          j11=(V[vihi_x]-V[vilo_x])/dx[0];
          j12=(V[vihi_y]-V[vilo_y])/dx[0];
          j13=(V[vihi_z]-V[vilo_z])/dx[0];
          }

        T j21=((T)0), j22=((T)0), j23=((T)0);
        if (jok)
          {
          size_t vjlo_x=3*idx.Index(i,j-1,k);
          size_t vjlo_y=vjlo_x+1;
          size_t vjlo_z=vjlo_y+1;

          size_t vjhi_x=3*idx.Index(i,j+1,k);
          size_t vjhi_y=vjhi_x+1;
          size_t vjhi_z=vjhi_y+1;

          j21=(V[vjhi_x]-V[vjlo_x])/dx[1];
          j22=(V[vjhi_y]-V[vjlo_y])/dx[1];
          j23=(V[vjhi_z]-V[vjlo_z])/dx[1];
          }

        T j31=((T)0), j32=((T)0), j33=((T)0);
        if (kok)
          {
          size_t vklo_x=3*idx.Index(i,j,k-1);
          size_t vklo_y=vklo_x+1;
          size_t vklo_z=vklo_y+1;

          size_t vkhi_x=3*idx.Index(i,j,k+1);
          size_t vkhi_y=vkhi_x+1;
          size_t vkhi_z=vkhi_y+1;

          j31=(V[vkhi_x]-V[vklo_x])/dx[2];
          j32=(V[vkhi_y]-V[vklo_y])/dx[2];
          j33=(V[vkhi_z]-V[vklo_z])/dx[2];
          }

        Matrix<T,3,3> J;
        J <<
          j11, j12, j13,
          j21, j22, j23,
          j31, j32, j33;

        // construct pressure corrected hessian
        Matrix<T,3,3> S=0.5*(J+J.transpose());
        Matrix<T,3,3> W=0.5*(J-J.transpose());
        Matrix<T,3,3> HP=S*S+W*W;

        // compute eigen values, lambda
        Matrix<T,3,1> e;
        SelfAdjointEigenSolver<Matrix<T,3,3> >solver(HP,false);
        e=solver.eigenvalues();  // input array bounds.

        const size_t pi=_idx.Index(_i,_j,_k);
        /*
        const size_t vi=3*pi;
        const size_t vj=vi+1;
        const size_t vk=vi+2;
        */

        // extract lambda-2
        slowSort(e.data(),0,3);
        L2[pi]=e(1,0);
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// L      -> second eigenvalues (lambda-2) of the corrected pressure hessian
//*****************************************************************************
template <typename TP, typename TD>
void Lambda2(int *input, int *output, TP *x, TP *y, TP *z, TD *V, TD *L2)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _ninj=_ni*_nj;

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        const TD dx[3] = {
            ((TD)(x[p+1]-x[p-1])),
            ((TD)(y[q+1]-y[q-1])),
            ((TD)(z[r+1]-z[r-1]))};

        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int pi=_k*_ninj+_j*_ni+_i;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil
        const int vilo=3*(k*ninj+j*ni+(i-1));
        const int vihi=3*(k*ninj+j*ni+(i+1));
        const int vjlo=3*(k*ninj+(j-1)*ni+i);
        const int vjhi=3*(k*ninj+(j+1)*ni+i);
        const int vklo=3*((k-1)*ninj+j*ni+i);
        const int vkhi=3*((k+1)*ninj+j*ni+i);

        // J: gradient velocity tensor, (jacobian)
        Matrix<TD,3,3> J;
        J <<
          (V[vihi]-V[vilo])/dx[0], (V[vihi+1]-V[vilo+1])/dx[0], V[vihi+2]-V[vilo+2]/dx[0],
          (V[vjhi]-V[vjlo])/dx[1], (V[vjhi+1]-V[vjlo+1])/dx[1], V[vjhi+2]-V[vjlo+2]/dx[1],
          (V[vkhi]-V[vklo])/dx[2], (V[vkhi+1]-V[vklo+1])/dx[2], V[vkhi+2]-V[vklo+2]/dx[2];

        // construct pressure corrected hessian
        Matrix<TD,3,3> S=((TD)0.5)*(J+J.transpose());
        Matrix<TD,3,3> W=((TD)0.5)*(J-J.transpose());
        Matrix<TD,3,3> HP=S*S+W*W;

        // compute eigen values, lambda
        Matrix<TD,3,1> e;
        SelfAdjointEigenSolver<Matrix<TD,3,3> >solver(HP,false);
        e=solver.eigenvalues();

        // extract lambda-2
        slowSort(e.data(),0,3);
        L2[pi]=e(1,0);
        L2[pi]=(((L2[pi]>=((TD)-1E-5))&&(L2[pi]<=((TD)1E-5)))?TD(0):L2[pi]);
        // TODO -- this is probably needed because of discrete particle
        // noise, as such it should not be used unless it's needed.
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// D      -> divergence
//*****************************************************************************
template <typename T>
void Divergence(
      int *input,
      int *output,
      int mode,
      double *dX,
      T *V,
      T *D)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const T dx[3]={
      ((T)dX[0])*((T)2),
      ((T)dX[1])*((T)2),
      ((T)dX[2])*((T)2)};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int  k=r-input[4];
    const int _k=r-output[4];

    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int  j=q-input[2];
      const int _j=q-output[2];

      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int  i=p-input[0];
        const int _i=p-output[0];
        const size_t _pi=_idx.Index(_i,_j,_k);

        //      __   ->
        //  D = \/ . V
        D[_pi]=((T)0);
        if (iok)
          {
          size_t vilo_x=3*idx.Index(i-1,j,k);
          size_t vihi_x=3*idx.Index(i+1,j,k);
          D[_pi] += (V[vihi_x]-V[vilo_x])/dx[0];
          }

        if (jok)
          {
          size_t vjlo_y=3*idx.Index(i,j-1,k)+1;
          size_t vjhi_y=3*idx.Index(i,j+1,k)+1;
          D[_pi] += (V[vjhi_y]-V[vjlo_y])/dx[1];
          }

        if (kok)
          {
          size_t vklo_z=3*idx.Index(i,j,k-1)+2;
          size_t vkhi_z=3*idx.Index(i,j,k+1)+2;
          D[_pi] += (V[vkhi_z]-V[vklo_z])/dx[2];
          }
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// D      -> divergence
//*****************************************************************************
template <typename TP, typename TD>
void Divergence(
      int *input,
      int *output,
      TP *x,
      TP *y,
      TP *z,
      TD *V,
      TD *D)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _ninj=_ni*_nj;

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // stencil deltas
        const TD dx[3] = {
            ((TD)(x[p+1]-x[p-1])),
            ((TD)(y[q+1]-y[q-1])),
            ((TD)(z[r+1]-z[r-1]))};

        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int _pi=_k*_ninj+_j*_ni+_i;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil into the input array
        const int vilo=3*(k*ninj+j*ni+(i-1));
        const int vihi=3*(k*ninj+j*ni+(i+1));
        const int vjlo=3*(k*ninj+(j-1)*ni+i);
        const int vjhi=3*(k*ninj+(j+1)*ni+i);
        const int vklo=3*((k-1)*ninj+j*ni+i);
        const int vkhi=3*((k+1)*ninj+j*ni+i);

        //      __   ->
        //  D = \/ . V
        D[_pi]
           = (V[vihi  ] - V[vilo  ])/dx[0]
           + (V[vjhi+1] - V[vjlo+1])/dx[1]
           + (V[vkhi+2] - V[vklo+2])/dx[2];
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// S      -> scalar field
// L      -> laplacian
//*****************************************************************************
template <typename T>
void Laplacian(
      int *input,
      int *output,
      int mode,
      double *dX,
      T *S,
      T *L)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const T dx2[3]={
      ((T)dX[0])*((T)dX[0]),
      ((T)dX[1])*((T)dX[1]),
      ((T)dX[2])*((T)dX[2])};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int  k=r-input[4];
    const int _k=r-output[4];

    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int  j=q-input[2];
      const int _j=q-output[2];

      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int _i=p-output[0];
        const size_t _pi=_idx.Index(_i,_j,_k);

        const int  i=p-input[0];
        const size_t  pi=idx.Index(i,j,k);

        //      __2
        //  L = \/ S
        L[_pi]=((T)0);
        if (iok)
          {
          const size_t ilo=idx.Index(i-1,j,k);
          const size_t ihi=idx.Index(i+1,j,k);
          L[_pi] += (S[ihi] + S[ilo] - ((T)2)*S[pi])/dx2[0];
          }

        if (jok)
          {
          const size_t jlo=idx.Index(i,j-1,k);
          const size_t jhi=idx.Index(i,j+1,k);
          L[_pi] += (S[jhi] + S[jlo] - ((T)2)*S[pi])/dx2[1];
          }

        if (kok)
          {
          const size_t klo=idx.Index(i,j,k-1);
          const size_t khi=idx.Index(i,j,k+1);
          L[_pi] += (S[khi] + S[klo] - ((T)2)*S[pi])/dx2[2];
          }
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// W      -> vector curl
//*****************************************************************************
template <typename TP, typename TD>
void Laplacian(
      int *input,
      int *output,
      TP *x,
      TP *y,
      TP *z,
      TD *S,
      TD *L)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _ninj=_ni*_nj;

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // stencil deltas
        TD dx2[3] = {
            ((TD)(x[p+1]-x[p-1])),
            ((TD)(y[q+1]-y[q-1])),
            ((TD)(z[r+1]-z[r-1]))};

        dx2[0]*=dx2[0];
        dx2[1]*=dx2[1];
        dx2[2]*=dx2[2];

        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int _pi=_k*_ninj+_j*_ni+_i;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        //
        const int pi=k*ninj+j*ni+i;

        // stencil into the input array
        const int ilo=k*ninj+j*ni+(i-1);
        const int ihi=k*ninj+j*ni+(i+1);
        const int jlo=k*ninj+(j-1)*ni+i;
        const int jhi=k*ninj+(j+1)*ni+i;
        const int klo=(k-1)*ninj+j*ni+i;
        const int khi=(k+1)*ninj+j*ni+i;

        //      __2
        //  L = \/ S
        L[_pi]
           = (S[ihi] + S[ilo] - TD(2)*S[pi])/dx2[0]
           + (S[jhi] + S[jlo] - TD(2)*S[pi])/dx2[1]
           + (S[khi] + S[klo] - TD(2)*S[pi])/dx2[2];
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// S      -> scalar field
// G      -> gradient
//*****************************************************************************
template <typename T>
void Gradient(
      int *input,
      int *output,
      int mode,
      double *dX,
      T *S,
      T *Gx,
      T *Gy,
      T *Gz)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const T dx[3]={
      ((T)dX[0])*((T)2),
      ((T)dX[1])*((T)2),
      ((T)dX[2])*((T)2)};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int  k=r-input[4];
    const int _k=r-output[4];

    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int  j=q-input[2];
      const int _j=q-output[2];

      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int  i=p-input[0];
        const int _i=p-output[0];
        const size_t _pi=_idx.Index(_i,_j,_k);

        //      __
        //  G = \/ S
        Gx[_pi]=((T)0);
        Gy[_pi]=((T)0);
        Gz[_pi]=((T)0);
        if (iok)
          {
          size_t ilo=idx.Index(i-1,j,k);
          size_t ihi=idx.Index(i+1,j,k);
          Gx[_pi] = T((S[ihi]-S[ilo])/dx[0]);
          }

        if (jok)
          {
          size_t jlo=idx.Index(i,j-1,k);
          size_t jhi=idx.Index(i,j+1,k);
          Gy[_pi] = T((S[jhi]-S[jlo])/dx[1]);
          }

        if (kok)
          {
          size_t klo=idx.Index(i,j,k-1);
          size_t khi=idx.Index(i,j,k+1);
          Gz[_pi] = T((S[khi]-S[klo])/dx[2]);
          }
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// S      -> scalar field
// G      -> gardient
//*****************************************************************************
template <typename TP, typename TD>
void Gradient(
      int *input,
      int *output,
      TP *x,
      TP *y,
      TP *z,
      TD *S,
      TD *Gx,
      TD *Gy,
      TD *Gz)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int ninj=ni*nj;

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _ninj=_ni*_nj;

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    for (int q=output[2]; q<=output[3]; ++q)
      {
      for (int p=output[0]; p<=output[1]; ++p)
        {
        // stencil deltas
        const TP dx[3]
          = {x[p+1]-x[p-1],y[q+1]-y[q-1],z[r+1]-z[r-1]};

        // output array indices
        const int _i=p-output[0];
        const int _j=q-output[2];
        const int _k=r-output[4];
        // index into output array;
        const int _pi=_k*_ninj+_j*_ni+_i;

        // input array indices
        const int i=p-input[0];
        const int j=q-input[2];
        const int k=r-input[4];
        // stencil into the input array
        const int ilo=k*ninj+j*ni+(i-1);
        const int ihi=k*ninj+j*ni+(i+1);
        const int jlo=k*ninj+(j-1)*ni+i;
        const int jhi=k*ninj+(j+1)*ni+i;
        const int klo=(k-1)*ninj+j*ni+i;
        const int khi=(k+1)*ninj+j*ni+i;

        //      __
        //  G = \/ S
        Gx[_pi] = (S[ihi]-S[ilo])/dx[0];
        Gy[_pi] = (S[jhi]-S[jlo])/dx[1];
        Gz[_pi] = (S[khi]-S[klo])/dx[2];
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// J      -> vector gradient (Jaccobian)
//*****************************************************************************
template <typename T>
void Gradient(
      int *input,
      int *output,
      int mode,
      double *dX,
      T *V,
      T *Jxx,
      T *Jxy,
      T *Jxz,
      T *Jyx,
      T *Jyy,
      T *Jyz,
      T *Jzx,
      T *Jzy,
      T *Jzz)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const T dx[3]={
      ((T)dX[0])*((T)2),
      ((T)dX[1])*((T)2),
      ((T)dX[2])*((T)2)};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int  k=r-input[4];
    const int _k=r-output[4];

    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int  j=q-input[2];
      const int _j=q-output[2];

      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int  i=p-input[0];
        const int _i=p-output[0];

        const size_t _pi=_idx.Index(_i,_j,_k);

        // J: gradient tensor, (jacobian)
        Jxx[_pi]=((T)0);
        Jxy[_pi]=((T)0);
        Jxz[_pi]=((T)0);
        if (iok)
          {
          size_t vilo_x=3*idx.Index(i-1,j,k);
          size_t vilo_y=vilo_x+1;
          size_t vilo_z=vilo_y+1;

          size_t vihi_x=3*idx.Index(i+1,j,k);
          size_t vihi_y=vihi_x+1;
          size_t vihi_z=vihi_y+1;

          Jxx[_pi] = (V[vihi_x]-V[vilo_x])/dx[0];;
          Jxy[_pi] = (V[vihi_y]-V[vilo_y])/dx[0];;
          Jxz[_pi] = (V[vihi_z]-V[vilo_z])/dx[0];;
          }

        Jyx[_pi]=((T)0);
        Jyy[_pi]=((T)0);
        Jyz[_pi]=((T)0);
        if (jok)
          {
          size_t vjlo_x=3*idx.Index(i,j-1,k);
          size_t vjlo_y=vjlo_x+1;
          size_t vjlo_z=vjlo_y+1;

          size_t vjhi_x=3*idx.Index(i,j+1,k);
          size_t vjhi_y=vjhi_x+1;
          size_t vjhi_z=vjhi_y+1;

          Jyx[_pi] = (V[vjhi_x]-V[vjlo_x])/dx[1];;
          Jyy[_pi] = (V[vjhi_y]-V[vjlo_y])/dx[1];;
          Jyz[_pi] = (V[vjhi_z]-V[vjlo_z])/dx[1];;
          }

        Jzx[_pi]=((T)0);
        Jzy[_pi]=((T)0);
        Jzz[_pi]=((T)0);
        if (kok)
          {
          size_t vklo_x=3*idx.Index(i,j,k-1);
          size_t vklo_y=vklo_x+1;
          size_t vklo_z=vklo_y+1;

          size_t vkhi_x=3*idx.Index(i,j,k+1);
          size_t vkhi_y=vkhi_x+1;
          size_t vkhi_z=vkhi_y+1;

          Jzx[_pi] = (V[vkhi_x]-V[vklo_x])/dx[2];;
          Jzy[_pi] = (V[vkhi_y]-V[vklo_y])/dx[2];;
          Jzz[_pi] = (V[vkhi_z]-V[vklo_z])/dx[2];;
          }
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// Q      ->
//*****************************************************************************
template <typename T>
void QCriteria(
      int *input,
      int *output,
      int mode,
      double *dX,
      T *V,
      T *Q)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const T dx[3]={
      ((T)dX[0])*((T)2),
      ((T)dX[1])*((T)2),
      ((T)dX[2])*((T)2)};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int  k=r-input[4];
    const int _k=r-output[4];

    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int  j=q-input[2];
      const int _j=q-output[2];

      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int  i=p-input[0];
        const int _i=p-output[0];

        const size_t _pi=_idx.Index(_i,_j,_k);

        // J: gradient tensor, (jacobian)
        T Jxx=((T)0);
        T Jxy=((T)0);
        T Jxz=((T)0);
        if (iok)
          {
          size_t vilo_x=3*idx.Index(i-1,j,k);
          size_t vilo_y=vilo_x+1;
          size_t vilo_z=vilo_y+1;

          size_t vihi_x=3*idx.Index(i+1,j,k);
          size_t vihi_y=vihi_x+1;
          size_t vihi_z=vihi_y+1;

          Jxx = (V[vihi_x]-V[vilo_x])/dx[0];;
          Jxy = (V[vihi_y]-V[vilo_y])/dx[0];;
          Jxz = (V[vihi_z]-V[vilo_z])/dx[0];;
          }

        T Jyx=((T)0);
        T Jyy=((T)0);
        T Jyz=((T)0);
        if (jok)
          {
          size_t vjlo_x=3*idx.Index(i,j-1,k);
          size_t vjlo_y=vjlo_x+1;
          size_t vjlo_z=vjlo_y+1;

          size_t vjhi_x=3*idx.Index(i,j+1,k);
          size_t vjhi_y=vjhi_x+1;
          size_t vjhi_z=vjhi_y+1;

          Jyx = (V[vjhi_x]-V[vjlo_x])/dx[1];;
          Jyy = (V[vjhi_y]-V[vjlo_y])/dx[1];;
          Jyz = (V[vjhi_z]-V[vjlo_z])/dx[1];;
          }

        T Jzx=((T)0);
        T Jzy=((T)0);
        T Jzz=((T)0);
        if (kok)
          {
          size_t vklo_x=3*idx.Index(i,j,k-1);
          size_t vklo_y=vklo_x+1;
          size_t vklo_z=vklo_y+1;

          size_t vkhi_x=3*idx.Index(i,j,k+1);
          size_t vkhi_y=vkhi_x+1;
          size_t vkhi_z=vkhi_y+1;

          Jzx = (V[vkhi_x]-V[vklo_x])/dx[2];;
          Jzy = (V[vkhi_y]-V[vklo_y])/dx[2];;
          Jzz = (V[vkhi_z]-V[vklo_z])/dx[2];;
          }

        T divV=Jxx+Jyy+Jzz;
        Q[_pi]
          = (divV*divV - (Jxx*Jxx + Jxy*Jyx + Jxz*Jzx
                           + Jyx*Jxy + Jyy*Jyy + Jyz*Jzy
                             + Jzx*Jxz + Jzy*Jyz + Jzz*Jzz))/((T)2);
        }
      }
    }
}


// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// dX     -> grid spacing triple
// V      -> vector field
// M      -> matrix arrays
// W      -> result
//*****************************************************************************
template <typename T>
void VectorMatrixMul(
      int *input,
      int *output,
      int mode,
      T *V,
      T *Mxx,
      T *Mxy,
      T *Mxz,
      T *Myx,
      T *Myy,
      T *Myz,
      T *Mzx,
      T *Mzy,
      T *Mzz,
      T *W)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int  k=r-input[4];
    const int _k=r-output[4];

    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int  j=q-input[2];
      const int _j=q-output[2];

      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int  i=p-input[0];
        const int _i=p-output[0];

        const size_t _pi=_idx.Index(_i,_j,_k);
        const size_t  pi= 3*idx.Index( i, j, k);

        W[_pi  ] = V[pi  ]*Mxx[_pi] + V[pi+1]*Myx[_pi] + V[pi+2]*Mzx[_pi];
        W[_pi+1] = V[pi+1]*Mxy[_pi] + V[pi+1]*Myy[_pi] + V[pi+2]*Mzy[_pi];
        W[_pi+2] = V[pi+2]*Mxz[_pi] + V[pi+1]*Myz[_pi] + V[pi+2]*Mzz[_pi];
        }
      }
    }
}

// input  -> patch input array is defined on
// output -> patch outpu array is defined on
// V      -> vector field
// W      -> result
//*****************************************************************************
template <typename T>
void Normalize(
      int *input,
      int *output,
      int mode,
      T *V,
      T *W)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int  k=r-input[4];
    const int _k=r-output[4];

    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int  j=q-input[2];
      const int _j=q-output[2];

      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int  i=p-input[0];
        const int _i=p-output[0];

        const size_t _pi=_idx.Index(_i,_j,_k);
        const size_t  pi= 3*idx.Index( i, j, k);

        T mv = ((T)sqrt(V[pi]*V[pi]+V[pi+1]*V[pi+1]+V[pi+2]*V[pi+2]));

        W[_pi  ] /= mv;
        W[_pi+1] /= mv;
        W[_pi+2] /= mv;
        }
      }
    }
}

//*****************************************************************************
template <typename T>
void EigenvalueDiagnostic(
      int *input,
      int *output,
      int mode,
      double *dX,
      T *V,
      T *L)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const T dx[3]={
      ((T)dX[0])*((T)2),
      ((T)dX[1])*((T)2),
      ((T)dX[2])*((T)2)};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int _k=r-output[4];
    const int  k=r-input[4];
    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int _j=q-output[2];
      const int  j=q-input[2];
      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int _i=p-output[0];
        const int  i=p-input[0];

        // J: gradient velocity tensor, (jacobian)
        T j11=((T)0), j12=((T)0), j13=((T)0);
        if (iok)
          {
          size_t vilo_x=3*idx.Index(i-1,j,k);
          size_t vilo_y=vilo_x+1;
          size_t vilo_z=vilo_y+1;

          size_t vihi_x=3*idx.Index(i+1,j,k);
          size_t vihi_y=vihi_x+1;
          size_t vihi_z=vihi_y+1;

          j11=(V[vihi_x]-V[vilo_x])/dx[0];
          j12=(V[vihi_y]-V[vilo_y])/dx[0];
          j13=(V[vihi_z]-V[vilo_z])/dx[0];
          }

        T j21=((T)0), j22=((T)0), j23=((T)0);
        if (jok)
          {
          size_t vjlo_x=3*idx.Index(i,j-1,k);
          size_t vjlo_y=vjlo_x+1;
          size_t vjlo_z=vjlo_y+1;

          size_t vjhi_x=3*idx.Index(i,j+1,k);
          size_t vjhi_y=vjhi_x+1;
          size_t vjhi_z=vjhi_y+1;

          j21=(V[vjhi_x]-V[vjlo_x])/dx[1];
          j22=(V[vjhi_y]-V[vjlo_y])/dx[1];
          j23=(V[vjhi_z]-V[vjlo_z])/dx[1];
          }

        T j31=((T)0), j32=((T)0), j33=((T)0);
        if (kok)
          {
          size_t vklo_x=3*idx.Index(i,j,k-1);
          size_t vklo_y=vklo_x+1;
          size_t vklo_z=vklo_y+1;

          size_t vkhi_x=3*idx.Index(i,j,k+1);
          size_t vkhi_y=vkhi_x+1;
          size_t vkhi_z=vkhi_y+1;

          j31=(V[vkhi_x]-V[vklo_x])/dx[2];
          j32=(V[vkhi_y]-V[vklo_y])/dx[2];
          j33=(V[vkhi_z]-V[vklo_z])/dx[2];
          }

        Matrix<T,3,3> J;
        J <<
          j11, j12, j13,
          j21, j22, j23,
          j31, j32, j33;

        // compute eigen values, lambda
        Matrix<std::complex<T>,3,1> e;
        EigenSolver<Matrix<T,3,3> >solver(J,false);
        e=solver.eigenvalues();

        std::complex<T> &e1 = e(0);
        std::complex<T> &e2 = e(1);
        std::complex<T> &e3 = e(2);

        // see Haimes, and Kenwright VGT and Feature Extraction fig 2
        // 0 - repelling node
        // 1 - type 1 saddle
        // 2 - type 2 saddle
        // 3 - attracting node
        // 4 - repelling spiral
        // 5 - type 1 saddle spiral
        // 6 - type 2 saddle spiral
        // 7 - attracting spiral
        const size_t pi=_idx.Index(_i,_j,_k);
        if (IsComplex(e1)||IsComplex(e2)||IsComplex(e3))
          {
          // spiral flow
          // one real , one conjugate pair

          int realIdx;
          int imagIdx1;
          //int imagIdx2;

          if (IsReal(e1))
            {
            realIdx=0;
            imagIdx1=1;
            //imagIdx2=2;
            }
          else
          if (IsReal(e2))
            {
            realIdx=1;
            imagIdx1=0;
            //imagIdx2=2;
            }
          else
          if (IsReal(e3))
            {
            realIdx=2;
            imagIdx1=0;
            //imagIdx2=1;
            }
          else
            {
            std::cerr << "No real eigne value." << std::endl;
            return;
            }

          bool attracting=(real(e(realIdx))<((T)0));
          bool type1=(imag(e(imagIdx1))<((T)0));

          if (type1 && attracting)
            {
            L[pi]=7;
            }
          else
          if (!type1 && attracting)
            {
            L[pi]=5;
            }
          else
          if (type1 && !attracting)
            {
            L[pi]=6;
            }
          else
          if (!type1 && !attracting)
            {
            L[pi]=4;
            }
          }
        else
          {
          // three real
          int nAttracting=0;
          for (int i=0; i<3; ++i)
            {
            if (real(e(i))<((T)0)) ++nAttracting;
            }
          L[pi]=((T)nAttracting);
          }
        }
      }
    }
}

#endif
