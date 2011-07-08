/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQHemisphereSource.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQHemisphereSource.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include "vtkSphereSource.h"

#include "vtkSQMetaDataKeys.h"
#include "GDAMetaDataKeys.h"
#include "Tuple.hxx"
#include "SQMacros.h"

#include <math.h>

//*****************************************************************************
void LocateHemisphere(float *pX, size_t nx,double *C, double *N)
{
  // allign the hemisphere with an aribtrary north and place it
  // an arbitrary center.

  float magN=sqrt(N[0]*N[0]+N[1]*N[1]+N[2]*N[2]);
  if (magN<1.0e-3)
    {
    sqErrorMacro(cerr,"Vector magniude must be non-zero.");
    return;
    }

  float v[3];
  v[0]=N[0]/magN;
  v[1]=N[1]/magN;
  v[2]=N[2]/magN;

  float p=sqrt(v[0]*v[0]+v[1]*v[1]);
  if (p<1e-03)
    {
    // North is coincident with z-axis nothing to do.
    return;
    }
  float q=v[2];
  float l=v[1]/p;
  float m=v[0]/p;

  for (size_t i=0; i<nx; ++i)
    {
    int ii=3*i;
    float x=pX[ii  ];
    float y=pX[ii+1];
    float z=pX[ii+2];

    // rotate twice and translate (see fig 3-7 rogers & adams)
    pX[ii  ] =  x*q*m - y*l + z*p*m + C[0];
    pX[ii+1] =  x*q*l + y*m + z*p*l + C[1];
    pX[ii+2] = -x*p         + z*q   + C[2];

    // cerr
    //   << Tuple<float>(x,y,z)
    //   << "->"
    //   << Tuple<float>(pX[ii],pX[ii+1],pX[ii+2])
    //   << endl;
    }
}


vtkCxxRevisionMacro(vtkSQHemisphereSource, "$Revision: 1.70 $");
vtkStandardNewMacro(vtkSQHemisphereSource);

//----------------------------------------------------------------------------
// Construct sphere with radius=0.5 and default resolution 8 in both Phi
// and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
vtkSQHemisphereSource::vtkSQHemisphereSource()
          :
      NorthHemisphereName(0),
      SouthHemisphereName(0)
{
  #ifdef vtkSQHemisphereSourceDEBUG
  cerr << "===============================vtkSQHemisphereSource" << endl;
  #endif
  this->Radius=1.0;

  this->Center[0]=0.0;
  this->Center[1]=0.0;
  this->Center[2]=0.0;

  this->North[0]=
  this->North[1]=0.0;
  this->North[2]=1.0;

  this->Resolution=32;

  this->SetNorthHemisphereName("North");
  this->SetSouthHemisphereName("South");

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(2);
}

//----------------------------------------------------------------------------
vtkSQHemisphereSource::~vtkSQHemisphereSource()
{
  #ifdef vtkSQHemisphereSourceDEBUG
  cerr << "===============================~vtkSQHemisphereSource" << endl;
  #endif
  this->SetNorthHemisphereName(0);
  this->SetSouthHemisphereName(0);
}

//----------------------------------------------------------------------------
int vtkSQHemisphereSource::FillInputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  #ifdef vtkSQHemisphereSourceDEBUG
  cerr << "===============================FillInputPortInformation" << endl;
  #endif
  // The input is optional, if used we'll look for some keys that define
  // the sphere's attriibutes coming from upstram (e.g from a reader).
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQHemisphereSource::RequestInformation(
      vtkInformation* /*req*/,
      vtkInformationVector** inInfos,
      vtkInformationVector* outInfos)
{
  #ifdef vtkSQHemisphereSourceDEBUG
  cerr << "===============================RequestInformation" << endl;
  #endif
  // The GDA meta data reader will insert information about
  // the center and radius of the dipole. If its there we'll
  // initialize the object from these.

  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);
  if (inInfo && inInfo->Has(GDAMetaDataKeys::DIPOLE_CENTER()))
      {
      inInfo->Get(GDAMetaDataKeys::DIPOLE_CENTER(),this->Center);
      double fakeCenter[3]={-1,-1,-1};
      inInfo->Set(GDAMetaDataKeys::DIPOLE_CENTER(),fakeCenter,3);
      inInfo->Set(GDAMetaDataKeys::DIPOLE_CENTER(),this->Center,3);
      inInfo->Modified();
      this->Modified();
      //inInfo->Modified((vtkInformationKey*)GDAMetaDataKeys::DIPOLE_CENTER());
      cerr << "Found DIPOLE_CENTER." << endl;
      }

//     if (inInfo->Has(GDAMetaDataKeys::CELL_SIZE_RE()))
//       {
//       double cellSizeRe;
// 
//       inInfo->Get(GDAMetaDataKeys::CELL_SIZE_RE(),cellSizeRe);
//       this->Radius=2.5/cellSizeRe; // sphere will have 2.5 re.
// 
//       cerr << "Found CELL_SIZE_RE." << endl;
//       }

  vtkInformation *outInfo = outInfos->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
  outInfo->Set(
      vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
      this->Center[0]-this->Radius,
      this->Center[0]+this->Radius,
      this->Center[1]-this->Radius,
      this->Center[1]+this->Radius,
      this->Center[2]-this->Radius,
      this->Center[2]+this->Radius);

  return 1;
}

//----------------------------------------------------------------------------
int vtkSQHemisphereSource::RequestData(
      vtkInformation * /*req*/,
      vtkInformationVector ** /*inInfos*/,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQHemisphereSourceDEBUG
  cerr << "===============================RequestData" << endl;
  this->Print(cerr);
  #endif

  vtkInformation *northInfo=outInfos->GetInformationObject(0);
  if (this->NorthHemisphereName && strlen(this->NorthHemisphereName))
    {
    northInfo->Set(vtkSQMetaDataKeys::DESCRIPTIVE_NAME(),this->NorthHemisphereName);
    }
  vtkPolyData *northPd
    = vtkPolyData::SafeDownCast(northInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSphereSource *ss=vtkSphereSource::New();
  ss->SetCenter(0.0,0.0,0.0);
  ss->SetRadius(this->Radius);
  ss->SetStartTheta(0.0);
  ss->SetEndTheta(360.0);
  ss->SetThetaResolution(this->Resolution);
  ss->SetPhiResolution(this->Resolution);

  // generate the northern Hemisphere
  ss->SetStartPhi(0.0);
  ss->SetEndPhi(90.0);
  ss->Update();
  ss->GetOutput()->Update();
  northPd->DeepCopy(ss->GetOutput());

  // orient it
  vtkFloatArray *X;
  float *pX;
  int nx;
  X=dynamic_cast<vtkFloatArray*>(northPd->GetPoints()->GetData());
  pX=X->GetPointer(0);
  nx=X->GetNumberOfTuples();
  LocateHemisphere(pX,nx,this->Center,this->North);


  vtkInformation *southInfo=outInfos->GetInformationObject(1);
  if (this->SouthHemisphereName && strlen(this->SouthHemisphereName))
    {
    southInfo->Set(vtkSQMetaDataKeys::DESCRIPTIVE_NAME(),this->SouthHemisphereName);
    }
  vtkPolyData *southPd
    = vtkPolyData::SafeDownCast(southInfo->Get(vtkDataObject::DATA_OBJECT()));

  // generate the southern Hemisphere
  ss->SetStartPhi(90.0);
  ss->SetEndPhi(180.0);
  ss->Update();
  ss->GetOutput()->Update();
  southPd->DeepCopy(ss->GetOutput());

  // orient it
  X=dynamic_cast<vtkFloatArray*>(southPd->GetPoints()->GetData());
  pX=X->GetPointer(0);
  nx=X->GetNumberOfTuples();
  LocateHemisphere(pX,nx,this->Center,this->North);

  ss->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQHemisphereSource::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQHemisphereSourceDEBUG
  cerr << "===============================PrintSelf" << endl;
  #endif
  // this->Superclass::PrintSelf(os,indent);

  os << indent << "Center "
     << this->Center[0] << ", "
     << this->Center[1] << ", "
     << this->Center[2] << endl
     << indent << "Radius "
     << this->Radius << endl
     << indent << "Resolution"
     << this->Resolution << endl
     << indent << "NorthHemisphereName "
     << this->NorthHemisphereName << endl
     << indent << "SouthHemisphereName "
     << this->SouthHemisphereName << endl;

  // TODO
}
