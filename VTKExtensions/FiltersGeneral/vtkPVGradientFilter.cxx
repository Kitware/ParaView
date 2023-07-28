// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVGradientFilter.h"

#include "vtkDemandDrivenPipeline.h"
#include "vtkHyperTreeGrid.h"
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
  os << indent << "Dimensionality: " << this->Dimensionality << std::endl;
  os << indent << "HTG Mode: " << this->HTGMode << std::endl;
  os << indent << "HTG Extensive Computation: " << this->HTGExtensiveComputation << std::endl;
}

//----------------------------------------------------------------------------
int vtkPVGradientFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  auto res = this->Superclass::FillInputPortInformation(port, info);
  if (port == 0)
  {
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  }
  return res;
}

//----------------------------------------------------------------------------
int vtkPVGradientFilter::FillOutputPortInformation(int port, vtkInformation* info)
{
  auto res = this->Superclass::FillOutputPortInformation(port, info);
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  }
  return res;
}

//----------------------------------------------------------------------------
int vtkPVGradientFilter::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkHyperTreeGrid* inHTG = vtkHyperTreeGrid::GetData(inputVector[0]);

  if (inHTG)
  {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataObject* output = vtkDataObject::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
    if (!output || !output->IsA(inHTG->GetClassName()))
    {
      vtkHyperTreeGrid* newOutput = inHTG->NewInstance();
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
    return 1;
  }
  return this->Superclass::RequestDataObject(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVGradientFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inDataObj = vtkDataObject::GetData(inputVector[0]);
  vtkHyperTreeGrid* inHTG = vtkHyperTreeGrid::GetData(inputVector[0]);

  if (!inDataObj && !inHTG)
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

  // the vtkHyperTreeGrid has a specific processing
  if (inHTG)
  {
    vtkNew<vtkHyperTreeGridGradient> htgGradient;
    htgGradient->SetInputData(0, inHTG);
    htgGradient->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
    htgGradient->SetMode(this->HTGMode);
    htgGradient->SetExtensiveComputation(this->HTGExtensiveComputation);

    htgGradient->SetComputeGradient(this->GetComputeGradient());
    htgGradient->SetGradientArrayName(this->GetResultArrayName());
    htgGradient->SetComputeDivergence(this->GetComputeDivergence());
    htgGradient->SetDivergenceArrayName(this->GetDivergenceArrayName());
    htgGradient->SetComputeVorticity(this->GetComputeVorticity());
    htgGradient->SetVorticityArrayName(this->GetVorticityArrayName());
    htgGradient->SetComputeQCriterion(this->GetComputeQCriterion());
    htgGradient->SetQCriterionArrayName(this->GetQCriterionArrayName());

    htgGradient->Update();
    outDataObj->ShallowCopy(htgGradient->GetOutput(0));

    return 1;
  }

  // We create a new superclass instance instead of using `this->Superclass::` so as to
  // go through the object factory
  vtkNew<Superclass> instance;
  instance->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
  instance->SetResultArrayName(this->GetResultArrayName());
  instance->SetDivergenceArrayName(this->GetDivergenceArrayName());
  instance->SetVorticityArrayName(this->GetVorticityArrayName());
  instance->SetQCriterionArrayName(this->GetQCriterionArrayName());
  instance->SetFasterApproximation(this->GetFasterApproximation());
  instance->SetComputeGradient(this->GetComputeGradient());
  instance->SetComputeDivergence(this->GetComputeDivergence());
  instance->SetComputeVorticity(this->GetComputeVorticity());
  instance->SetComputeQCriterion(this->GetComputeQCriterion());
  instance->SetContributingCellOption(this->GetContributingCellOption());
  instance->SetReplacementValueOption(this->GetReplacementValueOption());

  instance->SetInputDataObject(inDataObj);
  instance->Update();
  outDataObj->ShallowCopy(instance->GetOutput());

  return 1;
}
