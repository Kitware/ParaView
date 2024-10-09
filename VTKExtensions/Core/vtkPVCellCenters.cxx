// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVCellCenters.h"

#include "vtkExecutive.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridCellCenters.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

vtkStandardNewMacro(vtkPVCellCenters);

//----------------------------------------------------------------------------
void vtkPVCellCenters::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Generate Vertex Cells: " << (this->VertexCells ? "On" : "Off") << std::endl;
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVCellCenters::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPolyData* output = vtkPolyData::GetData(outInfo);

  if (output)
  {
    if (auto inputDS = vtkDataSet::GetData(inInfo))
    {
      vtkNew<vtkCellCenters> cellCenters;
      cellCenters->SetInputData(inputDS);
      cellCenters->SetVertexCells(this->GetVertexCells());

      if (cellCenters->GetExecutive()->Update())
      {
        output->ShallowCopy(cellCenters->GetOutput(0));
        return 1;
      }
    }
    else if (auto inputHTG = vtkHyperTreeGrid::GetData(inInfo))
    {
      vtkNew<vtkHyperTreeGridCellCenters> htgCellCenters;
      htgCellCenters->SetInputData(inputHTG);
      htgCellCenters->SetVertexCells(this->GetVertexCells());

      if (htgCellCenters->GetExecutive()->Update())
      {
        output->ShallowCopy(htgCellCenters->GetOutput(0));
        return 1;
      }
    }
  }
  vtkErrorMacro(<< "Unable to retrieve input / output as supported type.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVCellCenters::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}
