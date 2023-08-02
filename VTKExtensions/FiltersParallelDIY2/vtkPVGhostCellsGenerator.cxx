// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVGhostCellsGenerator.h"

#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkGhostCellsGenerator.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGhostCellsGenerator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

class vtkPVGhostCellsGenerator::vtkInternals
{
public:
  vtkNew<vtkGhostCellsGenerator> Generator;
  vtkNew<vtkHyperTreeGridGhostCellsGenerator> HTGGenerator;

  vtkInternals() = default;
};

vtkStandardNewMacro(vtkPVGhostCellsGenerator);

//----------------------------------------------------------------------------
vtkPVGhostCellsGenerator::vtkPVGhostCellsGenerator()
  : Internals(new vtkInternals)
{
  // Setup default configuration
  this->SetOutputType(VTK_POLY_DATA);

  this->RegisterFilter(this->Internals->Generator.GetPointer());
  this->RegisterFilter(this->Internals->HTGGenerator.GetPointer());

  this->Superclass::SetActiveFilter(0);
}

//----------------------------------------------------------------------------
vtkPVGhostCellsGenerator::~vtkPVGhostCellsGenerator() = default;

//----------------------------------------------------------------------------
void vtkPVGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVGhostCellsGenerator::SetController(vtkMultiProcessController* controller)
{
  this->Internals->Generator->SetController(controller);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVGhostCellsGenerator::SetBuildIfRequired(bool enable)
{
  this->Internals->Generator->SetBuildIfRequired(enable);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVGhostCellsGenerator::SetNumberOfGhostLayers(int nbGhostLayers)
{
  this->Internals->Generator->SetNumberOfGhostLayers(nbGhostLayers);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  vtkDataObject* output = vtkDataObject::GetData(outInfo);

  if (!input)
  {
    vtkErrorMacro("Missing input!");
    return 0;
  }

  int filterIndex = vtkHyperTreeGrid::SafeDownCast(input) == nullptr ? 0 : 1;
  this->SetActiveFilter(filterIndex);

  vtkSmartPointer<vtkDataObject> newOutput;
  if (!output || !output->IsA(input->GetClassName()))
  {
    newOutput.TakeReference(input->NewInstance());
  }

  if (newOutput)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
  }

  return 1;
}
