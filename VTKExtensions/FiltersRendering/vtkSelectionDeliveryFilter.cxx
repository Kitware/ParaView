/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionDeliveryFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectionDeliveryFilter.h"

#include "vtkAppendSelection.h"
#include "vtkClientServerMoveData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkReductionFilter.h"
#include "vtkSelection.h"

vtkStandardNewMacro(vtkSelectionDeliveryFilter);
//----------------------------------------------------------------------------
vtkSelectionDeliveryFilter::vtkSelectionDeliveryFilter()
{
  this->ReductionFilter = vtkReductionFilter::New();
  this->ReductionFilter->SetController(vtkMultiProcessController::GetGlobalController());

  vtkAppendSelection* post_gather_algo = vtkAppendSelection::New();
  post_gather_algo->SetAppendByUnion(0);
  this->ReductionFilter->SetPostGatherHelper(post_gather_algo);
  post_gather_algo->FastDelete();

  this->DeliveryFilter = vtkClientServerMoveData::New();
  this->DeliveryFilter->SetOutputDataType(VTK_SELECTION);
}

//----------------------------------------------------------------------------
vtkSelectionDeliveryFilter::~vtkSelectionDeliveryFilter()
{
  this->ReductionFilter->Delete();
  this->DeliveryFilter->Delete();
}

//----------------------------------------------------------------------------
int vtkSelectionDeliveryFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSelectionDeliveryFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSelection* input = (inputVector[0]->GetNumberOfInformationObjects() == 1)
    ? vtkSelection::GetData(inputVector[0], 0)
    : nullptr;
  vtkSelection* output = vtkSelection::GetData(outputVector, 0);

  if (input)
  {
    vtkSelection* clone = vtkSelection::New();
    clone->ShallowCopy(input);
    this->ReductionFilter->SetInputData(clone);
    this->DeliveryFilter->SetInputConnection(this->ReductionFilter->GetOutputPort());
    clone->FastDelete();
  }
  else
  {
    this->DeliveryFilter->RemoveAllInputs();
  }
  this->DeliveryFilter->Modified();
  this->DeliveryFilter->Update();
  output->ShallowCopy(this->DeliveryFilter->GetOutputDataObject(0));
  this->DeliveryFilter->RemoveAllInputs();
  return 1;
}

//----------------------------------------------------------------------------
void vtkSelectionDeliveryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
