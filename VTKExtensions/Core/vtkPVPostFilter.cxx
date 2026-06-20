// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVPostFilter.h"

#include "Private/vtkPVPostFilterPrivateTools.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVPostFilterExecutive.h"

#include <string>

vtkStandardNewMacro(vtkPVPostFilter);
//----------------------------------------------------------------------------
vtkPVPostFilter::vtkPVPostFilter()
{
  vtkPVPostFilterExecutive* exec = vtkPVPostFilterExecutive::New();
  this->SetExecutive(exec);
  exec->FastDelete();

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVPostFilter::~vtkPVPostFilter() = default;

//----------------------------------------------------------------------------
std::string vtkPVPostFilter::DefaultComponentName(int componentNumber, int componentCount)
{
  return vtkPVPostFilterPrivateTools::DefaultComponentName(componentNumber, componentCount);
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVPostFilter::CreateDefaultExecutive()
{
  return vtkPVPostFilterExecutive::New();
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkDataObject* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // we need to just copy the data, so we can fixup the output as needed
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (output && input)
  {
    vtkCompositeDataSet* csInput = vtkCompositeDataSet::SafeDownCast(input);
    vtkCompositeDataSet* csOutput = vtkCompositeDataSet::SafeDownCast(output);
    if (!csInput && !csOutput)
    {
      // vtkDataSet
      output->ShallowCopy(input);
    }
    else
    {
      csOutput->CompositeShallowCopy(csInput);
    }
    if (this->Information->Has(vtkPVPostFilterExecutive::POST_ARRAYS_TO_PROCESS()))
    {
      vtkPVPostFilterPrivateTools::DoAnyNeededConversions(
        this->Information, vtkPVPostFilterExecutive::POST_ARRAYS_TO_PROCESS(), output);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPostFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
