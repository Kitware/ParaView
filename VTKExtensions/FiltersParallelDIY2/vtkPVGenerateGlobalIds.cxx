// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVGenerateGlobalIds.h"

#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkGenerateGlobalIds.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGenerateGlobalIds.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVGenerateGlobalIds);

//----------------------------------------------------------------------------
void vtkPVGenerateGlobalIds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Tolerance: " << this->Tolerance << endl;
}

//----------------------------------------------------------------------------
int vtkPVGenerateGlobalIds::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* inputDS = vtkDataSet::GetData(inInfo);
  vtkDataSet* outputDS = vtkDataSet::GetData(outInfo);

  if (inputDS && outputDS)
  {
    vtkNew<vtkGenerateGlobalIds> generateGlobalIds;
    generateGlobalIds->SetInputData(inputDS);
    generateGlobalIds->SetTolerance(this->GetTolerance());
    generateGlobalIds->Update();

    outputDS->ShallowCopy(generateGlobalIds->GetOutput(0));
    return 1;
  }

  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::GetData(inInfo);
  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::GetData(outInfo);

  if (inputHTG && outputHTG)
  {
    vtkNew<vtkHyperTreeGridGenerateGlobalIds> htgGenerateGlobalIds;
    htgGenerateGlobalIds->SetInputData(inputHTG);
    htgGenerateGlobalIds->Update();

    outputHTG->ShallowCopy(htgGenerateGlobalIds->GetOutput(0));
    return 1;
  }

  vtkErrorMacro(<< "Unable to retrieve input / output as supported type.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVGenerateGlobalIds::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}
