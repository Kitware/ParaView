/*=========================================================================

  Program:   ParaView
  Module:    vtkImageSliceDataDeliveryFilter.cxx

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
#include "vtkMultiClientMPIMoveData.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkImageSliceDataDeliveryFilter);
//----------------------------------------------------------------------------
vtkImageSliceDataDeliveryFilter::vtkImageSliceDataDeliveryFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->DeliveryHelper = vtkMultiClientMPIMoveData::New();
  this->DeliveryHelper->SetOutputDataType(VTK_IMAGE_DATA);
}

//----------------------------------------------------------------------------
vtkImageSliceDataDeliveryFilter::~vtkImageSliceDataDeliveryFilter()
{
  this->DeliveryHelper->Delete();
  this->DeliveryHelper->Delete();
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
  this->DeliveryHelper->Deliver(input, output);
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageSliceDataDeliveryFilter::Modified()
{
  this->DeliveryHelper->Reset();
  this->Superclass::Modified();
}

//----------------------------------------------------------------------------
void vtkImageSliceDataDeliveryFilter::ProcessViewRequest(vtkInformation* info)
{
  this->DeliveryHelper->ProcessViewRequest(info);
}

//----------------------------------------------------------------------------
unsigned long vtkImageSliceDataDeliveryFilter::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();
  unsigned long md_mtime = this->DeliveryHelper->GetMTime();
  mtime = mtime > md_mtime? mtime : md_mtime;
  return mtime;
}

//----------------------------------------------------------------------------
void vtkImageSliceDataDeliveryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
