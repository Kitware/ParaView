/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGradientFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVGradientFilter.h"

#include "vtkDemandDrivenPipeline.h"
#include "vtkImageData.h"
#include "vtkImageGradient.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVGradientFilter);

//----------------------------------------------------------------------------
void vtkPVGradientFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
}

//----------------------------------------------------------------------------
int vtkPVGradientFilter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVGradientFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (!inInfo)
  {
    vtkErrorMacro(<< "Failed to get input information.");
    return 0;
  }

  vtkDataObject* inDataObj = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!inDataObj)
  {
    vtkErrorMacro(<< "Failed to get input data object.");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!outInfo)
  {
    vtkErrorMacro(<< "Failed to get output information.");
  }

  vtkDataObject* outDataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!outDataObj)
  {
    vtkErrorMacro(<< "Failed to get output data object.");
  }

  // vtkImageGradient is used by default for vtkImageData
  if (vtkImageData::SafeDownCast(inDataObj) && this->BoundaryMethod == CENTRAL_DIFFERENCING)
  {
    vtkNew<vtkImageGradient> imageGradFilter;
    imageGradFilter->SetInputData(0, inDataObj);
    imageGradFilter->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
    imageGradFilter->SetDimensionality(this->Dimensionality);
    imageGradFilter->Update();
    outDataObj->ShallowCopy(imageGradFilter->GetOutput(0));

    return 1;
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVGradientFilter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);

  if (input)
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkDataObject* output = vtkDataObject::GetData(outputVector);

    if (!output || !output->IsA(input->GetClassName()))
    {
      vtkDataObject* newOutput = input->NewInstance();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }

    return 1;
  }

  return 0;
}
