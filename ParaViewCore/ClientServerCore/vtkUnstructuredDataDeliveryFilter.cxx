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
#include "vtkMultiClientMPIMoveData.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkUnstructuredDataDeliveryFilter);
//----------------------------------------------------------------------------
vtkUnstructuredDataDeliveryFilter::vtkUnstructuredDataDeliveryFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->DeliveryHelper = vtkMultiClientMPIMoveData::New();

  this->OutputDataType = VTK_VOID;
  this->SetOutputDataType(VTK_POLY_DATA);

  this->LODMode = false;
}

//----------------------------------------------------------------------------
vtkUnstructuredDataDeliveryFilter::~vtkUnstructuredDataDeliveryFilter()
{
  this->DeliveryHelper->Delete();
  this->DeliveryHelper = NULL;
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
    this->DeliveryHelper->SetOutputDataType(type);
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
int vtkUnstructuredDataDeliveryFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkDataObject* input = (inputVector[0]->GetNumberOfInformationObjects() == 1)?
    vtkDataObject::GetData(inputVector[0], 0) : NULL;
  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);

  this->DeliveryHelper->Deliver(input, output);
  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::Modified()
{
  this->DeliveryHelper->Reset();
  this->Superclass::Modified();
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::ProcessViewRequest(vtkInformation* info)
{
  this->DeliveryHelper->SetLODMode(this->LODMode);
  this->DeliveryHelper->ProcessViewRequest(info);
}

//----------------------------------------------------------------------------
unsigned long vtkUnstructuredDataDeliveryFilter::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();

  unsigned long md_mtime = this->DeliveryHelper->GetMTime();
  mtime = mtime > md_mtime? mtime : md_mtime;
  return mtime;
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

