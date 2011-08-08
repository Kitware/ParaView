/*=========================================================================

  Program:   ParaView
  Module:    vtkUnstructuredDataDeliveryFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredDataDeliveryFilter.h"

#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIMoveData.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkUnstructuredDataDeliveryFilter);
//----------------------------------------------------------------------------
vtkUnstructuredDataDeliveryFilter::vtkUnstructuredDataDeliveryFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->MoveData = vtkMPIMoveData::New();

  this->OutputDataType = VTK_VOID;
  this->SetOutputDataType(VTK_POLY_DATA);

  this->LODMode = false;

  // Discover process type and setup communication ivars.
  this->InitializeForCommunication();
}

//----------------------------------------------------------------------------
vtkUnstructuredDataDeliveryFilter::~vtkUnstructuredDataDeliveryFilter()
{
  this->MoveData->Delete();
}

//----------------------------------------------------------------------------
int vtkUnstructuredDataDeliveryFilter::FillInputPortInformation(
  int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::SetOutputDataType(int type)
{
  if (this->OutputDataType != type)
    {
    this->OutputDataType = type;
    this->MoveData->SetOutputDataType(type);
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
int vtkUnstructuredDataDeliveryFilter::RequestDataObject(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkDataObject *output = vtkDataObject::GetData(outputVector, 0);
  if (!output || !output->IsA(
      vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataType)))
    {
    output = vtkDataObjectTypes::NewDataObject(this->OutputDataType);
    if (!output)
      {
      return 0;
      }
    output->SetPipelineInformation(outputVector->GetInformationObject(0));
    this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
      output->GetExtentType());
    output->FastDelete();
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::InitializeForCommunication()
{
  this->MoveData->InitializeForCommunicationForParaView();
}

//----------------------------------------------------------------------------
int vtkUnstructuredDataDeliveryFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkDataObject* input = (inputVector[0]->GetNumberOfInformationObjects() == 1)?
    vtkDataObject::GetData(inputVector[0], 0) : NULL;
  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);

  vtkSmartPointer<vtkDataObject> inputClone;
  if (input)
    {
    inputClone.TakeReference(input->NewInstance());
    inputClone->ShallowCopy(input);
    }
  this->MoveData->SetInput(inputClone);
  this->MoveData->Update();
  output->ShallowCopy(this->MoveData->GetOutputDataObject(0));
  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::Modified()
{
  this->MoveData->Modified();
  this->Superclass::Modified();
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::ProcessViewRequest(vtkInformation* info)
{
  if (info->Has(vtkPVRenderView::DATA_DISTRIBUTION_MODE()))
    {
    this->MoveData->SetMoveMode(
      info->Get(vtkPVRenderView::DATA_DISTRIBUTION_MODE()));
    }
  else
    {
    // default mode is pass-through.
    this->MoveData->SetMoveModeToPassThrough();
    }

  bool deliver_outline =
    (info->Has(vtkPVRenderView::DELIVER_OUTLINE_TO_CLIENT()) != 0);
  if (this->LODMode)
    {
    deliver_outline |=
      (info->Has(vtkPVRenderView::DELIVER_OUTLINE_TO_CLIENT_FOR_LOD())!=0);
    }
  if (deliver_outline)
    {
    this->MoveData->SetDeliverOutlineToClient(1);
    }
  else
    {
    this->MoveData->SetDeliverOutlineToClient(0);
    }
}

//----------------------------------------------------------------------------
unsigned long vtkUnstructuredDataDeliveryFilter::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();
  unsigned long md_mtime = this->MoveData->GetMTime();
  mtime = mtime > md_mtime? mtime : md_mtime;
  return mtime;
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

