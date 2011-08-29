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
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPieceCacheFilter.h"
#include "vtkPieceList.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkStreamingHarness);

vtkCxxSetObjectMacro(vtkStreamingHarness, PieceList1, vtkPieceList);
vtkCxxSetObjectMacro(vtkStreamingHarness, PieceList2, vtkPieceList);
vtkCxxSetObjectMacro(vtkStreamingHarness, CacheFilter, vtkPieceCacheFilter);

//----------------------------------------------------------------------------
vtkStreamingHarness::vtkStreamingHarness()
{
  this->Enabled = true;
  this->Pass = 0;
  this->Piece = 0;
  this->NumberOfPieces = 32;
  this->Resolution = 1.0;
  this->ForOther = false;
  this->PieceList1 = NULL;
  this->PieceList2 = NULL;
  this->CacheFilter = NULL;
  this->LockRefinement = 0;
  this->TryAppended = false;
}

//----------------------------------------------------------------------------
vtkStreamingHarness::~vtkStreamingHarness()
{
  this->SetPieceList1(NULL);
  this->SetPieceList2(NULL);
  this->SetCacheFilter(NULL);
}

//-----------------------------------------------------------------------------
void vtkStreamingHarness::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->CacheFilter, "CacheFilter");
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
    //vtkDataObject *output =
    this->GetOutput();

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
int vtkStreamingHarness::RequestUpdateExtent
(
 vtkInformation* vtkNotUsed(request),
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

    //cerr << "HARNESS(" << this <<") RUE "
    //     << "P/NP " << P << "/" << NP << "->";
    //split each downstream piece into many
    P = P * this->NumberOfPieces + this->Piece;
    NP = NP * this->NumberOfPieces;
    //cerr << "P/NP " << P << "/" << NP << endl;

    //send the adjusted piece at my resolution upstream
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Set
      (vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), P);
    inInfo->Set
      (vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), NP);
    inInfo->Set
      (vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION(),
       this->Resolution);

    static int emptyExtent[6] = {0,-1,0,-1,0,-1};
    inInfo->Set
      (vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), emptyExtent, 6);
    inInfo->Set
      (vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 0);

    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkStreamingHarness::Append()
{
  if (this->CacheFilter)
    {
    this->CacheFilter->AppendPieces();
    this->TryAppended = true;
    }
}

//----------------------------------------------------------------------------
int vtkStreamingHarness::RequestData
(
 vtkInformation* vtkNotUsed(request),
 vtkInformationVector** inputVector,
 vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkPolyData *ipd = vtkPolyData::SafeDownCast(input);

  bool usedAppended = false;
  if (this->TryAppended)
    {
    this->TryAppended = false;
    if (ipd && this->CacheFilter)
      {
      vtkPolyData *apd = this->CacheFilter->GetAppendedData();
      vtkPolyData *opd = vtkPolyData::SafeDownCast(output);
      if (apd && opd)
        {
        usedAppended = true;
        //cerr << "HARNESS(" << this <<") RD APPEND SLOT" << endl;
        opd->ShallowCopy(apd);
        }
      }
    }

  if (!usedAppended)
    {
    /*
    cerr << "HARNESS(" << this <<") RD "
         << this->Piece << "/"
         << this->NumberOfPieces << "@"
         << this->Resolution  << endl;
    */
    output->ShallowCopy(input);
    }

  return 1;
}

//----------------------------------------------------------------------------
double vtkStreamingHarness::ComputePiecePriority(
  int piece, int NumPieces, double resolution)
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
  sddp->SetUpdatePiece(outInfo, piece);
  sddp->SetUpdateNumberOfPieces(outInfo, NumPieces);
  sddp->SetUpdateResolution(outInfo, resolution);

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
void vtkStreamingHarness::ComputePieceMetaInformation(
  int piece, int NumPieces, double resolution,
  double bounds[6], double &gconfidence,
  double &min, double &max, double &aconfidence)
{
  unsigned long ignore = 0;
  double ignoreN[3];
  double *normalResult = ignoreN;
  this->ComputePieceMetaInformation(piece, NumPieces, resolution,
                                    bounds, gconfidence,
                                    min, max, aconfidence,
                                    ignore, &normalResult);
}

//----------------------------------------------------------------------------
void vtkStreamingHarness::ComputePieceMetaInformation(
  int piece, int NumPieces, double resolution,
  double bounds[6], double &gconfidence,
  double &min, double &max, double &aconfidence,
  unsigned long &numCells, double **pNormal)
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
  sddp->SetUpdatePiece(outInfo, piece);
  sddp->SetUpdateNumberOfPieces(outInfo, NumPieces);
  sddp->SetUpdateResolution(outInfo, resolution);

  //ask the pipeline to compute the priority and thus forward meta info
  //double result =
  sddp->ComputePriority(0);

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

  numCells = 0;
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::ORIGINAL_NUMBER_OF_CELLS()))
    {
    numCells = inInfo->Get(vtkStreamingDemandDrivenPipeline::ORIGINAL_NUMBER_OF_CELLS());
    }

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::PIECE_NORMAL()))
    {
    double *normal = inInfo->Get(vtkStreamingDemandDrivenPipeline::PIECE_NORMAL());
    double *dest = *pNormal;
    dest[0] = normal[0];
    dest[1] = normal[1];
    dest[2] = normal[2];
    }
  else
    {
    *pNormal = NULL;
    }

  //restore the old setting
  sddp->SetUpdatePiece(outInfo, oldPiece);
  sddp->SetUpdateNumberOfPieces(outInfo, oldNumPieces);
  sddp->SetUpdateResolution(outInfo, oldResolution);

  this->ForOther = false;
}

//----------------------------------------------------------------------------
bool vtkStreamingHarness::InAppend(
  int piece, int NumPieces, double resolution)
{
  if (this->CacheFilter)
    {
    return this->CacheFilter->InAppend(piece, NumPieces, resolution);
    }
  return false;
}

//------------------------------------------------------------------------------
void vtkStreamingHarness::SetLockRefinement(int nv)
{
  if (nv == this->LockRefinement)
    {
    return;
    }
  this->LockRefinement = nv;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkStreamingHarness::RestartRefinement()
{
  //cerr << "restart refinement" << endl;
  this->SetPieceList1(NULL);
  this->SetPieceList2(NULL);
  if (this->CacheFilter)
    {
    this->CacheFilter->EmptyCache();
    }
  this->Modified();
}
