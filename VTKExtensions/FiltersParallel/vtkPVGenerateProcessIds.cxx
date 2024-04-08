// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVGenerateProcessIds.h"

#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkGenerateProcessIds.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGenerateProcessIds.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVGenerateProcessIds);

//----------------------------------------------------------------------------
void vtkPVGenerateProcessIds::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Generate for PointData: " << (this->GeneratePointData ? "On" : "Off")
     << std::endl;
  os << indent << "Generate for CellData: " << (this->GenerateCellData ? "On" : "Off") << std::endl;
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVGenerateProcessIds::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* inputDS = vtkDataSet::GetData(inInfo);
  vtkDataSet* outputDS = vtkDataSet::GetData(outInfo);

  if (inputDS && outputDS)
  {
    vtkNew<vtkGenerateProcessIds> generateProcessIds;
    generateProcessIds->SetInputData(inputDS);
    generateProcessIds->SetGeneratePointData(this->GetGeneratePointData());
    generateProcessIds->SetGenerateCellData(this->GetGenerateCellData());
    generateProcessIds->Update();

    outputDS->ShallowCopy(generateProcessIds->GetOutput(0));
    return 1;
  }

  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::GetData(inInfo);
  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::GetData(outInfo);

  if (inputHTG && outputHTG)
  {
    vtkNew<vtkHyperTreeGridGenerateProcessIds> htgGenerateProcessIds;
    htgGenerateProcessIds->SetInputData(inputHTG);
    htgGenerateProcessIds->Update();

    outputHTG->ShallowCopy(htgGenerateProcessIds->GetOutput(0));
    return 1;
  }

  vtkErrorMacro(<< "Unable to retrieve input / output as supported type.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVGenerateProcessIds::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}
