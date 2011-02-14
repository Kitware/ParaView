/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtractPieces.h"

#include "vtkExtractPiece.h"
#include "vtkExtractPolyDataPiece.h"
#include "vtkExtractUnstructuredGridPiece.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransmitPolyDataPiece.h"
#include "vtkTransmitUnstructuredGridPiece.h"

#include <assert.h>

vtkStandardNewMacro(vtkPVExtractPieces);
//----------------------------------------------------------------------------
vtkPVExtractPieces::vtkPVExtractPieces()
{
}

//----------------------------------------------------------------------------
vtkPVExtractPieces::~vtkPVExtractPieces()
{
}

#define ENSURE(classname)\
  if (this->RealAlgorithm && this->RealAlgorithm->IsA(#classname)) { } \
  else { this->RealAlgorithm = vtkSmartPointer<classname>::New(); }

//----------------------------------------------------------------------------
bool vtkPVExtractPieces::EnsureRealAlgorithm(vtkInformation* inputInformation)
{
  vtkMultiProcessController* controller=
    vtkMultiProcessController::GetGlobalController();
  if (controller == NULL || controller->GetNumberOfProcesses() <= 1)
    {
    this->RealAlgorithm = 0;
    return false;
    }

  int max_num_pieces = -1;
  if (inputInformation->Has(
      vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()))
    {
    max_num_pieces = inputInformation->Get(
      vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES());
    }

  if (max_num_pieces == -1)
    {
    // The source can already produce pieces.
    this->RealAlgorithm = 0;
    return false;
    }

  vtkDataObject* dobj = vtkDataObject::GetData(inputInformation);
  if (!dobj)
    {
    this->RealAlgorithm = 0;
    return false;
    }

  if (dobj->IsA("vtkPolyData"))
    {
    if (getenv("PV_LOCK_SAFE") != NULL)
      {
      ENSURE(vtkExtractPolyDataPiece);
      }
    else
      {
      ENSURE(vtkTransmitPolyDataPiece);
      }
    return true;
    }
  else if (dobj->IsA("vtkUnstructuredGrid"))
    {
    if (getenv("PV_LOCK_SAFE") != NULL)
      {
      ENSURE(vtkExtractUnstructuredGridPiece);
      }
    else
      {
      ENSURE(vtkTransmitUnstructuredGridPiece);
      }
    return true;
    }
  else if (dobj->IsA("vtkHierarchicalBoxDataSet") ||
    dobj->IsA("vtkMultiBlockDataSet"))
    {
    ENSURE(vtkExtractPiece);
    }

  this->RealAlgorithm = 0;
  return false;
}

//----------------------------------------------------------------------------
int vtkPVExtractPieces::RequestDataObject(vtkInformation* req,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  return this->Superclass::RequestDataObject(req, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVExtractPieces::RequestInformation(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->EnsureRealAlgorithm(inputVector[0]->GetInformationObject(0)))
    {
    return this->Superclass::RequestInformation(request, inputVector,
      outputVector);
    }

  assert(this->RealAlgorithm);
  return this->RealAlgorithm->ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVExtractPieces::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->RealAlgorithm)
    {
    vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
    vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
    output->ShallowCopy(input);
    return 1;
    }

  return this->RealAlgorithm->ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVExtractPieces::RequestUpdateExtent(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->RealAlgorithm)
    {
    return this->Superclass::RequestUpdateExtent(request, inputVector,
      outputVector);
    }
  return this->RealAlgorithm->ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVExtractPieces::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // We cannot simply cay we accept all data-types since that causes issues when
  // dealing with vtkTemporalDataSet. So we accept anything but
  // vtkTemporalDataSet.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiPieceDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiPieceDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}
//----------------------------------------------------------------------------
void vtkPVExtractPieces::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
