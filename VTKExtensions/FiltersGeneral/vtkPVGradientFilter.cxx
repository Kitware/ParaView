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
int vtkPVGradientFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inDataObj = vtkDataObject::GetData(inputVector[0]);

  if (!inDataObj)
  {
    vtkErrorMacro(<< "Failed to get input data object.");
    return 0;
  }

  vtkDataObject* outDataObj = vtkDataObject::GetData(outputVector);

  if (!outDataObj)
  {
    vtkErrorMacro(<< "Failed to get output data object.");
    return 0;
  }

  // vtkImageGradient is used by default for vtkImageData
  if (vtkImageData::SafeDownCast(inDataObj) && this->BoundaryMethod == SMOOTHED)
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
