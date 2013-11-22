/* ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQAgyrotropyFilter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQAgyrotropyFilter.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"

#include "vtkSQLog.h"
#include "XMLUtils.h"
#include "SQMacros.h"

#include <algorithm>
#include <typeinfo>
#include <string>
#include <cmath>

// #define SQTK_DEBUG

// ****************************************************************************
template<typename T>
void Agyrotropy(T *pT, T *pV, T *pA, size_t n, T noiseThreshold)
{
  noiseThreshold=-T(fabs(noiseThreshold));

  for (size_t i=0; i<n; ++i)
    {
    T bx=pV[0];
    T by=pV[1];
    T bz=pV[2];

    T bxx = bx*bx;
    T bxy = bx*by;
    T bxz = bx*bz;
    T byy = by*by;
    T byz = by*bz;
    T bzz = bz*bz;

    T pxx=pT[0];
    T pxy=pT[1];
    T pxz=pT[2];
    T pyx=pT[3];
    T pyy=pT[4];
    T pyz=pT[5];
    T pzx=pT[6];
    T pzy=pT[7];
    T pzz=pT[8];

    T nxx =  byy*pzz - byz*pyz - byz*pzy + bzz*pyy;
    T nxy = -bxy*pzz + byz*pzx + bxz*pyz - bzz*pyx;
    T nxz =  bxy*pzy - byy*pzx - bxz*pyy + byz*pyx;
    T nyy =  bxx*pzz - bxz*pzx - bxz*pxz + bzz*pxx;
    T nyz = -bxx*pzy + bxy*pzx + bxz*pxy - byz*pxx;
    T nzz =  bxx*pyy - bxy*pyx - bxy*pxy + byy*pxx;

    T a = nxx+nyy+nzz;
    T b = -(nxy*nxy + nxz*nxz + nyz*nyz - nxx*nyy - nxx*nzz - nyy*nzz);

    T d = a*a-T(4)*b;
    if ((d<=T(0)) && (d>=noiseThreshold))
      {
      d=T(0);
      }
    else
    if (d<=T(0))
      {
      vtkGenericWarningMacro(
        << "point " << i << " has negative descriminant. "
           "In PIC data this may be due to noise and maybe "
           "corrected by increasing the noiseThreshold." << endl
        << "a=" << a << endl
        << "b=" << b << endl
        << "d=" << d << endl);
      d*=T(-1);
      }

    pA[i]=T(2)*T(sqrt(d))/a;

    pV+=3;
    pT+=9;
    }
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQAgyrotropyFilter);

//-----------------------------------------------------------------------------
vtkSQAgyrotropyFilter::vtkSQAgyrotropyFilter()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQAgyrotropyFilter::vtkSQAgyrotropyFilter" << std::endl;
  #endif

  this->NoiseThreshold=1.0e-4;
  this->LogLevel=0;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQAgyrotropyFilter::~vtkSQAgyrotropyFilter()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQAgyrotropyFilter::~vtkSQAgyrotropyFilter" << std::endl;
  #endif
}

//-----------------------------------------------------------------------------
int vtkSQAgyrotropyFilter::Initialize(vtkPVXMLElement *root)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQAgyrotropyFilter::Initialize" << std::endl;
  #endif

  vtkPVXMLElement *elem=0;
  elem=GetOptionalElement(root,"vtkSQAgyrotropyFilter");
  if (elem==0)
    {
    return -1;
    }

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->GetHeader()
      << "# ::vtkSQAgyrotropyFilter" << "\n";
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkSQAgyrotropyFilter::RequestData(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **inputVector,
                vtkInformationVector *outputVector)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQAgyrotropyFilter::RequestData" << std::endl;
  #endif

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->StartEvent("vtkSQAgyrotropyFilter::RequestData");
    }

  vtkInformation *info;

  // get output
  info=outputVector->GetInformationObject(0);
  vtkDataSet *out
    = dynamic_cast<vtkDataSet*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (out==0)
    {
    vtkErrorMacro("output dataset was not present.");
    return 1;
    }

  /// get input
  info=inputVector[0]->GetInformationObject(0);
  vtkDataSet *in
    = dynamic_cast<vtkDataSet*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (in==0)
    {
    vtkErrorMacro("input dataset was not present.");
    return 1;
    }

  /// construct the output from a shallow copy
  out->ShallowCopy(in);

  /// get electron pressure tensor and magnetic field
  vtkDataArray *T=this->GetInputArrayToProcess(0,inputVector);
  if (T==0)
    {
    vtkErrorMacro("pressure tensor not found.");
    }
  std::string TName=T->GetName();

  vtkDataArray *V=this->GetInputArrayToProcess(1,inputVector);
  if (V==0)
    {
    vtkErrorMacro("magnetic field vector  not found.");
    }
  std::string VName=V->GetName();

  size_t nTups = V->GetNumberOfTuples();

  // add the agyrotropy array to the output
  vtkDataArray *A=V->NewInstance();
  std::string AName;
  AName+="agyrotropy-";
  AName+=TName;
  AName+="-";
  AName+=VName;
  A->SetName(AName.c_str());
  A->SetNumberOfTuples(nTups);
  out->GetPointData()->AddArray(A);

  // compute the agyrotropy
  switch(A->GetDataType())
    {
    vtkFloatTemplateMacro(
      Agyrotropy(
          (VTK_TT*)T->GetVoidPointer(0),
          (VTK_TT*)V->GetVoidPointer(0),
          (VTK_TT*)A->GetVoidPointer(0),
          nTups,
          (VTK_TT)this->NoiseThreshold));
      default:
        vtkErrorMacro(
          << "Cannot compute agyrotropy on type "
          << V->GetClassName());
    }

  if (this->LogLevel || globalLogLevel)
    {
    log->EndEvent("vtkSQAgyrotropyFilter::RequestData");
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQAgyrotropyFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
