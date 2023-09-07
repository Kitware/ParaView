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
  if (controller != this->Internals->Generator->GetController())
  {
    this->Internals->Generator->SetController(controller);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGhostCellsGenerator::SetBuildIfRequired(bool enable)
{
  if (enable != this->Internals->Generator->GetBuildIfRequired())
  {
    this->Internals->Generator->SetBuildIfRequired(enable);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGhostCellsGenerator::SetNumberOfGhostLayers(int nbGhostLayers)
{
  if (nbGhostLayers != this->Internals->Generator->GetNumberOfGhostLayers())
  {
    this->Internals->Generator->SetNumberOfGhostLayers(nbGhostLayers);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGhostCellsGenerator::SetSynchronizeOnly(bool sync)
{
  if (sync != this->Internals->Generator->GetSynchronizeOnly())
  {
    this->Internals->Generator->SetSynchronizeOnly(sync);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGhostCellsGenerator::SetGenerateGlobalIds(bool set)
{
  if (set != this->Internals->Generator->GetGenerateGlobalIds())
  {
    this->Internals->Generator->SetGenerateGlobalIds(set);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGhostCellsGenerator::SetGenerateProcessIds(bool set)
{
  if (set != this->Internals->Generator->GetGenerateProcessIds())
  {
    this->Internals->Generator->SetGenerateProcessIds(set);
    this->Modified();
  }
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
    return 1;
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
