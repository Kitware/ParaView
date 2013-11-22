/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "CPUConvolutionDriver.h"

#include "SQVTKTemplateMacroWarningSupression.h"
#include "SQPOSIXOnWindowsWarningSupression.h"

#include "SQPosixOnWindows.h"
#include "CartesianExtent.h"
#include "MemOrder.hxx"
#include "Numerics.hxx"
#include "SQMacros.h"
#include "postream.h"

#include "vtkDataArray.h"

#include <iostream>
#include <vector>
#include <cstdlib>

//#define CPUConvolutionDriverDEBUG

//-----------------------------------------------------------------------------
CPUConvolutionDriver::CPUConvolutionDriver()
        :
    Optimization(OPT_NONE)
{}

//-----------------------------------------------------------------------------
int CPUConvolutionDriver::Convolution(
    CartesianExtent &extV,
    CartesianExtent &extW,
    CartesianExtent &extK,
    int nGhost,
    int mode,
    vtkDataArray *V,
    vtkDataArray *W,
    float *K)
{
  // TODO - make sure nothing is leaked if an error occurs!

  #ifdef CPUConvolutionDriverDEBUG
  pCerr()
    << "===============CPUConvolutionDriver::Convolution" << std::endl;
  #endif

  int nV[3];
  extV.Size(nV);
  size_t vnijk=extV.Size();

  int nW[3];
  extW.Size(nW);
  size_t wnijk=extW.Size();
  int nComp=W->GetNumberOfComponents();

  int nK[3];
  extK.Size(nK);
  size_t knijk=extK.Size();

  int fastDim=0;
  int slowDim=1;
  switch (mode)
    {
    case CartesianExtent::DIM_MODE_2D_XY:
      fastDim=0;
      slowDim=1;
      break;
    case CartesianExtent::DIM_MODE_2D_XZ:
      fastDim=0;
      slowDim=2;
      break;
    case CartesianExtent::DIM_MODE_2D_YZ:
      fastDim=1;
      slowDim=2;
      break;
    case CartesianExtent::DIM_MODE_3D:
      fastDim=0;
      slowDim=1;
      break;
    default:
      sqErrorMacro(std::cerr,"Bad dim mode.");
      return -1;
    }

  #ifdef CPUConvolutionDriverDEBUG
  pCerr() << "wnijk=" << wnijk << std::endl;
  pCerr() << "fastDim=" << fastDim << std::endl;
  pCerr() << "slowDim=" << slowDim << std::endl;
  pCerr() << "extV=" << extV << std::endl;
  pCerr() << "nV=(" << nV[fastDim] <<  ", " << nV[slowDim] << ")" << std::endl;
  pCerr() << "extW=" << extW << std::endl;
  pCerr() << "nW=(" << nW[fastDim] <<  ", " << nW[slowDim] << ")" << std::endl;
  #endif

  switch (this->Optimization)
    {
    ///
    case OPT_NONE:
      switch (V->GetDataType())
        {
        vtkFloatTemplateMacro(
          ::Convolution<VTK_TT>(
              extV.GetData(),
              extW.GetData(),
              extK.GetData(),
              nComp,
              mode,
              (VTK_TT*)V->GetVoidPointer(0),
              (VTK_TT*)W->GetVoidPointer(0),
              K));
        }
      break;

    ///
    case OPT_FLATTEN_VTK:
    case OPT_Z_ORDER:
      switch (V->GetDataType())
        {
        // TODO -- replace with vtkTemplateMacro
        case VTK_FLOAT:
          {
          std::vector<float*> sV((size_t)nComp,NULL);
          std::vector<float*> sW((size_t)nComp,NULL);
          for (int q=0; q<nComp; ++q)
            {
            posix_memalign((void**)&sV[q],16,vnijk*sizeof(float));
            posix_memalign((void**)&sW[q],16,wnijk*sizeof(float));
            }

          // convert vtk vectors/tensors into scalar component arrays
          float *hV=(float*)V->GetVoidPointer(0);
          Split<float>(vnijk,hV,sV);

          // apply convolution
          for (int q=0; q<nComp; ++q)
            {
            if ((mode==CartesianExtent::DIM_MODE_2D_XY)
              ||(mode==CartesianExtent::DIM_MODE_2D_XZ)
              ||(mode==CartesianExtent::DIM_MODE_2D_YZ))
              {
              ::ScalarConvolution2D(
                    nV[fastDim],
                    nW[fastDim],
                    wnijk,
                    nK[fastDim],
                    knijk,
                    nGhost,
                    sV[q],
                    sW[q],
                    K);
              }
            else
              {
              ::ScalarConvolution3D(
                    nV[fastDim],
                    nV[fastDim]*nV[slowDim],
                    nW[fastDim],
                    nW[fastDim]*nW[slowDim],
                    wnijk,
                    nK[fastDim],
                    nK[fastDim]*nK[slowDim],
                    knijk,
                    nGhost,
                    sV[q],
                    sW[q],
                    K);
              }
            }

          // put results in vtk order
          float *hW=(float*)W->GetVoidPointer(0);
          Interleave(wnijk,sW,hW);

          // clean up
          for (int q=0; q<nComp; ++q)
            {
            free(sW[q]);
            free(sV[q]);
            }
          }
          break;
        default:
          // TODO
          sqErrorMacro(std::cerr,"Not currently using vtkTemplateMacro");
          return -1;
        }
      break;

    default:
      sqErrorMacro(pCerr(),"Invalid optimzation code " << this->Optimization);
      return -1;
    }

  return 0;
}
