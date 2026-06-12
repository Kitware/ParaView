// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVGhostCellsGenerator.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSetRange.h"
#include "vtkConvertToPartitionedDataSetCollection.h"
#include "vtkDataAssembly.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGhostCellsGenerator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkRange.h"

vtkStandardNewMacro(vtkPVGhostCellsGenerator);

//----------------------------------------------------------------------------
void vtkPVGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::GhostCellsGeneratorUsingSuperclassInstance(
  vtkDataObject* inputDO, vtkDataObject* outputDO)
{
  if (!outputDO)
  {
    return 0;
  }
  vtkNew<Superclass> instance;
  instance->SetController(this->GetController());
  instance->SetBuildIfRequired(this->GetBuildIfRequired());
  instance->SetNumberOfGhostLayers(this->GetNumberOfGhostLayers());
  instance->SetGenerateGlobalIds(this->GetGenerateGlobalIds());
  instance->SetGenerateProcessIds(this->GetGenerateProcessIds());
  instance->SetSynchronizeOnly(this->GetSynchronizeOnly());
  instance->SetUseImplicitArrays(this->GetUseImplicitArrays());
  instance->SetInputDataObject(inputDO);
  const int result = instance->GetExecutive()->Update();
  if (result == 1)
  {
    outputDO->ShallowCopy(instance->GetOutput());
  }
  return result;
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::GhostCellsGeneratorUsingHyperTreeGrid(
  vtkDataObject* inputDO, vtkDataObject* outputDO)
{
  vtkNew<vtkHyperTreeGridGhostCellsGenerator> instance;
  instance->SetController(this->GetController());
  instance->SetInputDataObject(inputDO);
  const int result = instance->GetExecutive()->Update();
  if (outputDO && result == 1)
  {
    outputDO->ShallowCopy(instance->GetOutput());
  }
  return result;
}

bool vtkPVGhostCellsGenerator::HasHTG(vtkMultiProcessController* controller, vtkDataObject* object)
{
  auto tree = vtkDataObjectTree::SafeDownCast(object);
  if (!tree)
  {
    // Look for either a simple HTG or a composite dataset containing HTG.
    int isHTG = vtkHyperTreeGrid::SafeDownCast(object) != nullptr;
    int someAreHTG = 1;
    controller->AllReduce(&isHTG, &someAreHTG, 1, vtkCommunicator::LOGICAL_OR_OP);
    return someAreHTG;
  }

  int hasHTGLeaves = 0;
  vtkCompositeDataIterator* iter = vtkDataObjectTree::SafeDownCast(object)->NewIterator();
  vtkDataObjectTreeIterator* treeIter = vtkDataObjectTreeIterator::SafeDownCast(iter);
  treeIter->TraverseSubTreeOn();
  treeIter->VisitOnlyLeavesOn();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (vtkHyperTreeGrid::SafeDownCast(iter->GetCurrentDataObject()))
    {
      hasHTGLeaves = 1;
      break;
    }
  }
  iter->Delete();

  // Some ranks may have null pieces, so we detect HTG using a reduction operation:
  // If at least 1 rank has a HTG piece, this means we must forward to the HTG specialized filter.
  int someHaveHTGLeaves = 1;
  controller->AllReduce(&hasHTGLeaves, &someHaveHTGLeaves, 1, vtkCommunicator::LOGICAL_OR_OP);
  return someHaveHTGLeaves;
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkDataObject* output = vtkDataObject::GetData(outputVector);

  if (!input)
  {
    return 1;
  }
  if (!output)
  {
    vtkErrorMacro(<< "Failed to get output data object.");
  }

  return this->ProcessDataObject(input, output);
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::ProcessDataObject(vtkDataObject* input, vtkDataObject* output)
{
  // Fast forward it to the classic GCG filter.
  bool inputHasHTG = vtkPVGhostCellsGenerator::HasHTG(this->GetController(), input);
  if (!inputHasHTG)
  {
    return this->GhostCellsGeneratorUsingSuperclassInstance(input, output);
  }
  // Input dataset is neither directly a HTG or a composite structure containing HTG.
  auto inputMB = vtkMultiBlockDataSet::SafeDownCast(input);
  if (inputMB)
  {
    auto outputMB = vtkMultiBlockDataSet::SafeDownCast(output);
    if (!outputMB)
    {
      vtkErrorMacro(<< "Failed to get a multiblock output structure");
      return 0;
    }
    return this->ProcessMultiBlockDataSet(inputMB, outputMB);
  }

  auto compositeInput = vtkCompositeDataSet::SafeDownCast(input);
  if (compositeInput)
  {
    auto compositeOutput = vtkCompositeDataSet::SafeDownCast(output);
    if (!compositeOutput)
    {
      vtkErrorMacro(<< "Failed to get a composite output structure");
      return 0;
    }
    return this->ProcessPartitionedDataSet(compositeInput, compositeOutput);
  }

  else
  {
    // Generate an empty HTG if the input is null
    if (input == nullptr)
    {
      vtkNew<vtkHyperTreeGrid> emptyHTG;
      emptyHTG->Initialize();
      output = nullptr;
      return this->GhostCellsGeneratorUsingHyperTreeGrid(emptyHTG, nullptr);
    }
    return this->GhostCellsGeneratorUsingHyperTreeGrid(input, output);
  }
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::ProcessMultiBlockDataSet(
  vtkMultiBlockDataSet* input, vtkMultiBlockDataSet* output)
{
  int result = 1;

  output->CopyStructure(input);
  // For MultiBlockDataSet containing HTG, we process each block individually
  // We don't generate ghost cells between different blocks
  for (unsigned int i = 0; i < input->GetNumberOfBlocks(); i++)
  {
    auto inputDO = input->GetBlock(i);
    auto outputDO = output->GetBlock(i);

    result &= this->ProcessDataObject(inputDO, outputDO);
  }

  return result;
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::ProcessPartitionedDataSet(
  vtkCompositeDataSet* input, vtkCompositeDataSet* output)
{
  int result = 1;

  output->CopyStructure(input);
  auto outputRange = vtk::Range(output, vtk::CompositeDataSetOptions::None);
  auto inputRange = vtk::Range(input, vtk::CompositeDataSetOptions::None);

  // For PartitionedDataSet and PDC containing HTG, we skip empty nodes when computing ghost cells
  for (auto inIt = inputRange.begin(), outIt = outputRange.begin(); inIt != inputRange.end();
       ++inIt, ++outIt)
  {
    if (*inIt)
    {
      *outIt = vtkSmartPointer<vtkDataObject>::Take(inIt->NewInstance());

      result &= this->ProcessDataObject(*inIt, *outIt);
    }
    else
    {
      *outIt = nullptr;
    }
  }
  return result;
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}
