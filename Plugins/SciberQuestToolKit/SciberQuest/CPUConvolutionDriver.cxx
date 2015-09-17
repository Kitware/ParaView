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
