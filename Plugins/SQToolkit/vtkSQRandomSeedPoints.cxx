/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQRandomSeedPoints.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQRandomSeedPoints.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkType.h"

#include <time.h>

// #define vtkSQRandomSeedPointsDEBUG

vtkCxxRevisionMacro(vtkSQRandomSeedPoints, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQRandomSeedPoints);

//----------------------------------------------------------------------------
vtkSQRandomSeedPoints::vtkSQRandomSeedPoints()
      :
  NumberOfPoints(10)
{
  this->Bounds[0]=this->Bounds[2]=this->Bounds[4]=0.0;
  this->Bounds[1]=this->Bounds[3]=this->Bounds[5]=1.0;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkSQRandomSeedPoints::~vtkSQRandomSeedPoints()
{}

//----------------------------------------------------------------------------
int vtkSQRandomSeedPoints::FillInputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  // The input is optional,if present it will be used 
  // for bounds.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQRandomSeedPoints::RequestInformation(
    vtkInformation * /*req*/,
    vtkInformationVector ** /*inInfos*/,
    vtkInformationVector *outInfos)
{
  // tell the excutive that we are handling our own paralelization.
  vtkInformation *outInfo=outInfos->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQRandomSeedPoints::RequestData(
    vtkInformation * /*req*/,
    vtkInformationVector **inInfos,
    vtkInformationVector *outInfos)
{
  vtkInformation *outInfo=outInfos->GetInformationObject(0);

  vtkPolyData *output
    = dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (output==NULL)
    {
    vtkErrorMacro("Empty output.");
    return 1;
    }

  // paralelize by piece information.
  int pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // sanity - the requst cannot be fullfilled
  int rank=vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  if (rank>=nPieces)
    {
    output->Initialize();
    return 1;
    }

  // we don't care which points are ours, rather the number we need to generate
  int nLarge=this->NumberOfPoints%nPieces;
  int nLocal=this->NumberOfPoints/nPieces+(pieceNo<nLarge?1:0);

  #ifdef vtkSQRandomSeedPointsDEBUG
  cerr
    << "pieceNo = " << pieceNo << endl
    << "nPieces = " << nPieces << endl
    << "rank    = " << rank << endl
    << "nLocal  = " << nLocal << endl;
  #endif

  // If the input is present then use it for bounds
  // otherwise we assume the user has previously set
  // desired bounds.
  vtkInformation *inInfo=0;
  if (inInfos[0]->GetNumberOfInformationObjects())
    {
    inInfo=inInfos[0]->GetInformationObject(0);
    vtkDataSet *input
      = dynamic_cast<vtkDataSet*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (input)
      {
      if (!inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX()))
        {
        vtkErrorMacro("Input must have WHOLE_BOUNDING_BOX set.");
        return 1;
        }
      double bounds[6];
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),bounds);
      this->SetBounds(bounds);
      }
    }

  // Configure the output
  vtkFloatArray *X=vtkFloatArray::New();
  X->SetNumberOfComponents(3);
  X->SetNumberOfTuples(nLocal);
  float *pX=X->GetPointer(0);

  vtkPoints *pts=vtkPoints::New();
  pts->SetData(X);
  X->Delete();

  output->SetPoints(pts);
  pts->Delete();

  vtkIdTypeArray *ia=vtkIdTypeArray::New();
  ia->SetNumberOfComponents(1);
  ia->SetNumberOfTuples(2*nLocal);
  vtkIdType *pIa=ia->GetPointer(0);

  vtkCellArray *verts=vtkCellArray::New();
  verts->SetCells(nLocal,ia);
  ia->Delete();

  output->SetVerts(verts);
  verts->Delete();

  float dX[3];
  dX[0]=this->Bounds[1]-this->Bounds[0];
  dX[1]=this->Bounds[3]-this->Bounds[2];
  dX[2]=this->Bounds[5]-this->Bounds[4];

  float eps[3];
  eps[0]=dX[0]/100.0;
  eps[1]=dX[1]/100.0;
  eps[2]=dX[2]/100.0;

  dX[0]-=2.0*eps[0];
  dX[1]-=2.0*eps[1];
  dX[2]-=2.0*eps[2];

  float X0[3];
  X0[0]=this->Bounds[0]+eps[0];
  X0[1]=this->Bounds[2]+eps[1];
  X0[2]=this->Bounds[4]+eps[2];

  double prog=0.0;
  double progUnit=1.0/nLocal;
  double progRepUnit=0.1;
  double progRepLevel=0.1;

  // generate the point set
  srand(rank+time(0));
  for (int q=0; q<nLocal; ++q, prog+=progUnit)
    {
    // update PV progress
    if (prog>=progRepLevel)
      {
      this->UpdateProgress(prog);
      progRepLevel+=progRepUnit;
      }

    // new random point
    const float rmax=RAND_MAX;
    pX[0]=X0[0]+dX[0]*rand()/rmax;
    pX[1]=X0[1]+dX[1]*rand()/rmax;
    pX[2]=X0[2]+dX[2]*rand()/rmax;

    pX+=3;

    // insert the cell
    pIa[0]=1;
    pIa[1]=q;
    pIa+=2;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQRandomSeedPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfPoints: " << this->NumberOfPoints << "\n";
}
