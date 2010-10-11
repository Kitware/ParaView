/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamingHarness.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkStreamingHarness.h"

#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPieceCacheFilter.h"
#include "vtkPieceList.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkStreamingHarness);

vtkCxxSetObjectMacro(vtkStreamingHarness, PieceList1, vtkPieceList);
vtkCxxSetObjectMacro(vtkStreamingHarness, PieceList2, vtkPieceList);
vtkCxxSetObjectMacro(vtkStreamingHarness, CacheFilter, vtkPieceCacheFilter);

//----------------------------------------------------------------------------
vtkStreamingHarness::vtkStreamingHarness()
{
  this->Pass = 0;
  this->Piece = 0;
  this->NumberOfPieces = 2;
  this->Resolution = 1.0;
  this->ForOther = false;
  this->PieceList1 = NULL;
  this->PieceList2 = NULL;
  this->NoneToRefine = false;
  this->CacheFilter = NULL;
}

//----------------------------------------------------------------------------
vtkStreamingHarness::~vtkStreamingHarness()
{
  this->SetPieceList1(NULL);
  this->SetPieceList2(NULL);
  this->SetCacheFilter(NULL);
}

//----------------------------------------------------------------------------
void vtkStreamingHarness::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Pass: " << this->Pass << endl;
  os << indent << "Piece: " << this->Piece << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
  os << indent << "Resolution: " << this->Resolution << endl;
  os << indent << "PieceList1: " << this->PieceList1 << endl;
  os << indent << "PieceList2: " << this->PieceList2 << endl;
  os << indent << "NoneToRefine: " << (this->NoneToRefine?"true":"false") << endl;
  os << indent << "CacheFilter: " << this->CacheFilter << endl;
}

//----------------------------------------------------------------------------
void vtkStreamingHarness::SetResolution(double newRes)
{
  if (newRes != this->Resolution)
    {
    this->Resolution = newRes;
    this->Modified();

    //TODO: GetOutput should not be necessary, but it is
    vtkDataObject *output = this->GetOutput();

    //tell the pipeline what resolution level to work at
    vtkInformationVector **inVec =
      this->GetExecutive()->GetInputInformation();
    vtkInformationVector *outVec =
      this->GetExecutive()->GetOutputInformation();

    vtkInformation* rqst = vtkInformation::New();
    rqst->Set(vtkStreamingDemandDrivenPipeline::REQUEST_RESOLUTION_PROPAGATE());
    rqst->Set(vtkExecutive::FORWARD_DIRECTION(),
              vtkExecutive::RequestUpstream);
    rqst->Set(vtkExecutive::ALGORITHM_BEFORE_FORWARD(), 1);
    rqst->Set(vtkExecutive::FROM_OUTPUT_PORT(), 0);
    this->GetExecutive()->ProcessRequest(rqst, inVec, outVec);
    rqst->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkStreamingHarness::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  //send the adjusted piece at my resolution upstream

  if (!this->ForOther)
    {
    //TODO: Does this have to be shoved into every update? Seems like a hack.
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Set
      (vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION(), this->Resolution);
    }

  return this->Superclass::ProcessRequest
    (request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkStreamingHarness::RequestUpdateExtent(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->ForOther)
    {

    //get downstream numpasses
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    int P = outInfo->Get
      (vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    int NP = outInfo->Get
    (vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

    //split each downstream piece into many
    P = P * this->NumberOfPieces + this->Piece;
    NP = NP * this->NumberOfPieces;

  //send the adjusted piece at my resolution upstream
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Set
      (vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), P);
    inInfo->Set
      (vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), NP);
    inInfo->Set
      (vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION(), this->Resolution);
    }
  return 1;
}


//----------------------------------------------------------------------------
int vtkStreamingHarness::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);
  return 1;
}

//----------------------------------------------------------------------------
double vtkStreamingHarness::ComputePriority(
  int Piece, int NumPieces, double Resolution)
{
  //TODO: Can I do this without changing the pipeline's state and setting
  //modified flags everywhere?

  this->ForOther = true;
  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast
    (this->GetExecutive());

  //get initial pipeline setting
  vtkInformationVector *outputVector =
    this->GetExecutive()->GetOutputInformation();
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int oldPiece = sddp->GetUpdatePiece(outInfo);
  int oldNumPieces = sddp->GetUpdateNumberOfPieces(outInfo);
  double oldResolution = sddp->GetUpdateResolution(outInfo);

  //change to new setting
  sddp->SetUpdatePiece(outInfo, Piece);
  sddp->SetUpdateNumberOfPieces(outInfo, NumPieces);
  sddp->SetUpdateResolution(outInfo, Resolution);

  //ask the pipeline to compute the priority
  double result = sddp->ComputePriority(0);

  //restore the old setting
  sddp->SetUpdatePiece(outInfo, oldPiece);
  sddp->SetUpdateNumberOfPieces(outInfo, oldNumPieces);
  sddp->SetUpdateResolution(outInfo, oldResolution);

  this->ForOther = false;

  return result;
}

//----------------------------------------------------------------------------
void vtkStreamingHarness::ComputeMetaInformation(
  int Piece, int NumPieces, double Resolution,
  double bounds[6], double &gconfidence,
  double &min, double &max, double &aconfidence)
{
  this->ForOther = true;

  //TODO: Can I get all of the arrays, not just the active scalars?

  //TODO: Can I do this without changing the pipeline's state and setting
  //modified flags everywhere?

  bounds[0] = bounds[2] = bounds[4] = 0;
  bounds[1] = bounds[3] = bounds[5] = -1;
  gconfidence = 0.0;
  min = 0;
  max = -1;
  aconfidence = 0.0;

  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast
    (this->GetExecutive());

  //get initial pipeline setting
  vtkInformationVector *outputVector =
    this->GetExecutive()->GetOutputInformation();
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int oldPiece = sddp->GetUpdatePiece(outInfo);
  int oldNumPieces = sddp->GetUpdateNumberOfPieces(outInfo);
  double oldResolution = sddp->GetUpdateResolution(outInfo);

  //change to new setting
  sddp->SetUpdatePiece(outInfo, Piece);
  sddp->SetUpdateNumberOfPieces(outInfo, NumPieces);
  sddp->SetUpdateResolution(outInfo, Resolution);

  //ask the pipeline to compute the priority and thus forward meta info
  double result = sddp->ComputePriority(0);

  // compute the priority for this UE
  vtkInformationVector **inVec =
    this->GetExecutive()->GetInputInformation();
  vtkInformation *inInfo = inVec[0]->GetInformationObject(0);

  double *wBBox = NULL;
  wBBox =
    inInfo->
    Get(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX());
  if (wBBox)
    {
    bounds[0] = wBBox[0];
    bounds[1] = wBBox[1];
    bounds[2] = wBBox[2];
    bounds[3] = wBBox[3];
    bounds[4] = wBBox[4];
    bounds[5] = wBBox[5];
    gconfidence = 1.0;
    }

  // get the range of the input if available
  vtkInformation *fInfo =
    vtkDataObject::GetActiveFieldInformation
    (inInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS,
     vtkDataSetAttributes::SCALARS);
  if (fInfo && fInfo->Has(vtkDataObject::PIECE_FIELD_RANGE()))
    {
    double *range = fInfo->Get(vtkDataObject::PIECE_FIELD_RANGE());
    min = range[0];
    max = range[1];
    aconfidence = 1.0;
    }

  //restore the old setting
  sddp->SetUpdatePiece(outInfo, oldPiece);
  sddp->SetUpdateNumberOfPieces(outInfo, oldNumPieces);
  sddp->SetUpdateResolution(outInfo, oldResolution);

  this->ForOther = false;

}
