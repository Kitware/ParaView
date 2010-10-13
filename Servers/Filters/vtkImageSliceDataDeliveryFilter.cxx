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
#include "vtkImageSliceDataDeliveryFilter.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIMoveData.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderView.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkImageSliceDataDeliveryFilter);
//----------------------------------------------------------------------------
vtkImageSliceDataDeliveryFilter::vtkImageSliceDataDeliveryFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->MoveData = vtkMPIMoveData::New();
  this->MoveData->SetOutputDataType(VTK_IMAGE_DATA);

  // Discover process type and setup communication ivars.
  this->InitializeForCommunication();
}

//----------------------------------------------------------------------------
vtkImageSliceDataDeliveryFilter::~vtkImageSliceDataDeliveryFilter()
{
  this->MoveData->Delete();
}

//----------------------------------------------------------------------------
void vtkImageSliceDataDeliveryFilter::InitializeForCommunication()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  int process_type = pm->GetOptions()->GetProcessType();
  switch (process_type)
    {
  case vtkPVOptions::PVSERVER:
  case vtkPVOptions::PVDATA_SERVER:
    this->MoveData->SetServerToDataServer();
    break;

  case vtkPVOptions::PVRENDER_SERVER:
    this->MoveData->SetServerToRenderServer();
    break;

  default:
    if (pm->GetPartitionId() > 0)
      {
      this->MoveData->SetServerToDataServer();
      }
    else
      {
      this->MoveData->SetServerToClient();
      }
    }

  vtkMPIMToNSocketConnection* render_server_data_server_connector =
    vtkMPIMToNSocketConnection::SafeDownCast(
      pm->GetObjectFromID(pm->GetMPIMToNSocketConnectionID(), true));
  this->MoveData->SetMPIMToNSocketConnection(render_server_data_server_connector);
  this->MoveData->SetController(pm->GetController());
}

//----------------------------------------------------------------------------
int vtkImageSliceDataDeliveryFilter::FillInputPortInformation(
  int , vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkImageSliceDataDeliveryFilter::RequestDataObject(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkDataObject *output = vtkDataObject::GetData(outputVector, 0);
  if (!output || !output->IsA("vtkImageData"))
    {
    output = vtkImageData::New();
    output->SetPipelineInformation(outputVector->GetInformationObject(0));
    this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
      output->GetExtentType());
    output->FastDelete();
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageSliceDataDeliveryFilter::RequestData(
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
  this->MoveData->Modified();
  this->MoveData->Update();
  output->ShallowCopy(this->MoveData->GetOutputDataObject(0));
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageSliceDataDeliveryFilter::Modified()
{
  this->MoveData->Modified();
  this->Superclass::Modified();
}

//----------------------------------------------------------------------------
void vtkImageSliceDataDeliveryFilter::ProcessViewRequest(vtkInformation* info)
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
}

//----------------------------------------------------------------------------
unsigned long vtkImageSliceDataDeliveryFilter::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();
  unsigned long md_mtime = this->MoveData->GetMTime();
  mtime = mtime > md_mtime? mtime : md_mtime;
  return mtime;
}

//----------------------------------------------------------------------------
void vtkImageSliceDataDeliveryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
