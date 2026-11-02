// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVGenerateGlobalIds.h"

#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSetRange.h"
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

  vtkDataObject* inputDO = vtkDataObject::GetData(inInfo);
  vtkDataObject* outputDO = vtkDataObject::GetData(outInfo);

  if (inputDO && outputDO &&
    (vtkDataSet::SafeDownCast(inputDO) || vtkCompositeDataSet::SafeDownCast(inputDO)))
  {
    // The whole input is given at once, so that the ids are numbered across all the data sets of a
    // composite input, and not partition by partition.
    vtkNew<vtkGenerateGlobalIds> generateGlobalIds;
    generateGlobalIds->SetInputData(inputDO);
    generateGlobalIds->SetTolerance(this->GetTolerance());
    generateGlobalIds->Update();

    outputDO->ShallowCopy(generateGlobalIds->GetOutput(0));

    // vtkGenerateGlobalIds only handles the vtkDataSet of a composite input.
    if (auto outputCD = vtkCompositeDataSet::SafeDownCast(outputDO))
    {
      for (vtkDataObject* leaf : vtk::Range(outputCD))
      {
        if (auto leafHTG = vtkHyperTreeGrid::SafeDownCast(leaf))
        {
          vtkNew<vtkHyperTreeGridGenerateGlobalIds> htgGenerateGlobalIds;
          htgGenerateGlobalIds->SetInputData(leafHTG);
          htgGenerateGlobalIds->Update();

          leafHTG->ShallowCopy(htgGenerateGlobalIds->GetOutput(0));
        }
      }
    }
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
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}
