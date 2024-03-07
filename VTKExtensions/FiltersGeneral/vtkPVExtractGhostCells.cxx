// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVExtractGhostCells.h"

#include "vtkDataObject.h"
#include "vtkExecutive.h"
#include "vtkExtractGhostCells.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridExtractGhostCells.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkPVExtractGhostCells);

//----------------------------------------------------------------------------
void vtkPVExtractGhostCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputGhostArrayName: " << this->OutputGhostArrayName << std::endl;
}

//----------------------------------------------------------------------------
int vtkPVExtractGhostCells::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  auto inputDO = vtkDataObject::GetData(inInfo);
  if (!inInfo)
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (vtkHyperTreeGrid::SafeDownCast(inputDO))
  {
    vtkHyperTreeGrid* output = vtkHyperTreeGrid::GetData(outInfo);
    if (!output)
    {
      output = vtkHyperTreeGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->FastDelete();
    }
    return 1;
  }
  else
  {
    vtkDataSet* output = vtkDataSet::GetData(outInfo);
    if (!output)
    {
      output = vtkUnstructuredGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->FastDelete();
    }
    return 1;
  }
}

//----------------------------------------------------------------------------
int vtkPVExtractGhostCells::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  vtkDataObject* output = vtkDataObject::GetData(outInfo);

  if (!input)
  {
    return 1;
  }
  if (!output)
  {
    vtkErrorMacro(<< "Failed to get output data object.");
    return 0;
  }

  // Set array name to default value when the field is empty
  if (this->GetOutputGhostArrayName().empty())
  {
    vtkWarningMacro("Empty output ghost array name, using default value 'GhostType'");
    this->SetOutputGhostArrayName("GhostType");
  }

  if (vtkHyperTreeGrid::SafeDownCast(input))
  {
    vtkNew<vtkHyperTreeGridExtractGhostCells> htgGhostCellsExtractor;
    htgGhostCellsExtractor->SetOutputGhostArrayName(this->GetOutputGhostArrayName().c_str());
    htgGhostCellsExtractor->SetInputData(0, input);
    if (htgGhostCellsExtractor->GetExecutive()->Update())
    {
      output->ShallowCopy(htgGhostCellsExtractor->GetOutput(0));
      return 1;
    }

    return 0;
  }
  else
  {
    vtkNew<vtkExtractGhostCells> ghostCellsExtractor;
    ghostCellsExtractor->SetOutputGhostArrayName(this->GetOutputGhostArrayName().c_str());
    ghostCellsExtractor->SetInputDataObject(input);
    if (ghostCellsExtractor->GetExecutive()->Update())
    {
      output->ShallowCopy(ghostCellsExtractor->GetOutput());
      return 1;
    }

    return 0;
  }
}

//----------------------------------------------------------------------------
int vtkPVExtractGhostCells::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}
