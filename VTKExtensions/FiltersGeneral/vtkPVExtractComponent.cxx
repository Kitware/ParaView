/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtractComponent.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtractComponent.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkPVExtractComponent);

//----------------------------------------------------------------------------
vtkPVExtractComponent::vtkPVExtractComponent()
{
  this->InputArrayComponent = -1;
  this->OutputArrayName = 0;
}

//----------------------------------------------------------------------------
vtkPVExtractComponent::~vtkPVExtractComponent()
{
  this->SetOutputArrayName(0);
}

//----------------------------------------------------------------------------
int vtkPVExtractComponent::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVExtractComponent::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);

  vtkDataArray* inVectors = this->GetInputArrayToProcess(0, inputVector);

  if (!inVectors)
  {
    vtkErrorMacro(<< "No data to extract");
    return 0;
  }

  if (this->InputArrayComponent >= inVectors->GetNumberOfComponents() ||
    this->InputArrayComponent < 0)
  {
    vtkErrorMacro(<< "Invalid component");
    return 0;
  }

  if (!this->OutputArrayName)
  {
    vtkErrorMacro(<< "No output array name");
    return 0;
  }

  vtkDataArray* outScalars = inVectors->NewInstance();
  outScalars->SetName(this->OutputArrayName);
  outScalars->SetNumberOfComponents(1);
  outScalars->SetNumberOfTuples(inVectors->GetNumberOfTuples());
  outScalars->CopyComponent(0, inVectors, this->InputArrayComponent);

  if (inVectors->GetNumberOfTuples() == input->GetNumberOfPoints())
  {
    output->GetPointData()->AddArray(outScalars);
  }
  else if (inVectors->GetNumberOfTuples() == input->GetNumberOfCells())
  {
    output->GetCellData()->AddArray(outScalars);
  }
  else
  {
    output->GetFieldData()->AddArray(outScalars);
  }

  outScalars->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVExtractComponent::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InputArrayComponent: " << this->InputArrayComponent << endl;
  os << indent << "OutputArrayName: " << this->OutputArrayName << endl;
}
