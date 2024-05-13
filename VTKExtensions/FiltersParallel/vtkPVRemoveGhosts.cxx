// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVRemoveGhosts.h"

#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridRemoveGhostCells.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRemoveGhosts.h"

vtkStandardNewMacro(vtkPVRemoveGhosts);

//----------------------------------------------------------------------------
int vtkPVRemoveGhosts::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* inputDS = vtkDataSet::GetData(inInfo);
  vtkDataSet* outputDS = vtkDataSet::GetData(outInfo);

  if (inputDS && outputDS)
  {
    vtkNew<vtkRemoveGhosts> removeGhosts;
    removeGhosts->SetInputData(inputDS);
    removeGhosts->Update();

    outputDS->ShallowCopy(removeGhosts->GetOutput(0));
    return 1;
  }

  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::GetData(inInfo);
  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::GetData(outInfo);

  if (inputHTG && outputHTG)
  {
    vtkNew<vtkHyperTreeGridRemoveGhostCells> htgRemoveGhosts;
    htgRemoveGhosts->SetInputData(inputHTG);
    htgRemoveGhosts->Update();

    outputHTG->ShallowCopy(htgRemoveGhosts->GetOutput(0));
    return 1;
  }

  vtkErrorMacro(<< "Unable to retrieve input / output as supported type.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVRemoveGhosts::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}
