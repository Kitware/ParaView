// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVGhostCellsGenerator.h"

#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGhostCellsGenerator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVGhostCellsGenerator);

//----------------------------------------------------------------------------
void vtkPVGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::GhostCellsGeneratorUsingSuperclassInstance(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkNew<Superclass> instance;

  instance->SetBuildIfRequired(this->GetBuildIfRequired());
  instance->SetController(this->GetController());
  instance->SetNumberOfGhostLayers(this->GetNumberOfGhostLayers());

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  instance->SetInputDataObject(inputDO);
  if (instance->GetExecutive()->Update())
  {
    outputDO->ShallowCopy(instance->GetOutput());
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
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
  }

  if (vtkHyperTreeGrid::SafeDownCast(input))
  {
    // Match behavior from vtkGhostCellsGenerator
    vtkNew<vtkHyperTreeGridGhostCellsGenerator> ghostCellsGenerator;
    ghostCellsGenerator->SetInputData(input);
    ghostCellsGenerator->Update();

    output->ShallowCopy(ghostCellsGenerator->GetOutput(0));
    return 1;
  }

  return this->GhostCellsGeneratorUsingSuperclassInstance(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}
