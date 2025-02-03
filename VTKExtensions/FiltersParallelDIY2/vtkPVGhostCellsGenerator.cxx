// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVGhostCellsGenerator.h"

#include "vtkConvertToPartitionedDataSetCollection.h"
#include "vtkDataAssembly.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGhostCellsGenerator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"

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
  return this->GhostCellsGeneratorUsingSuperclassInstance(
    vtkDataObject::GetData(inputVector[0]), vtkDataObject::GetData(outputVector));
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
  if (!outputDO)
  {
    return 0;
  }
  vtkNew<vtkHyperTreeGridGhostCellsGenerator> instance;
  instance->SetController(this->GetController());
  instance->SetInputDataObject(inputDO);
  const int result = instance->GetExecutive()->Update();
  if (result == 1)
  {
    outputDO->ShallowCopy(instance->GetOutput());
  }
  return result;
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkDataObject* output = vtkDataObject::GetData(outputVector);

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  if (!input)
  {
    return 1;
  }
  if (!output)
  {
    vtkErrorMacro(<< "Failed to get output data object.");
  }

  // Some ranks can have null pieces, so we detect HTG using a reduction operation:
  // If at least 1 rank has a HTG piece, this means we must forward to the HTG specialized filter.
  // The same logic is used for composite datasets, but one partitioned dataset at a time.
  int isHTG = vtkHyperTreeGrid::SafeDownCast(input) != nullptr;
  int someAreHTG = false;
  controller->AllReduce(&isHTG, &someAreHTG, 1, vtkCommunicator::LOGICAL_OR_OP);
  if (someAreHTG)
  {
    const int result = this->GhostCellsGeneratorUsingHyperTreeGrid(input, output);
    if (result != 1)
    {
      vtkErrorMacro(<< "Failed to generate ghost cells for input HyperTreeGrid");
      return 0;
    }

    return 1;
  }

  // Check if our composite input contains HTG.
  // If it does, dispatch the blocks to the right implementation of the filter.
  auto inputComposite = vtkDataObjectTree::SafeDownCast(input);
  int isCompositeHTG = inputComposite && this->HasCompositeHTG;
  int someAreCompositeHTG = false;
  controller->AllReduce(&isCompositeHTG, &someAreCompositeHTG, 1, vtkCommunicator::LOGICAL_OR_OP);

  if (someAreCompositeHTG)
  {
    auto outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(output);
    if (!outputPDC)
    {
      vtkErrorMacro(<< "Unable to retrieve output as vtkPartitionedDataSetCollection.");
      return 0;
    }

    // Convert the input structure to PDC if necessary
    vtkNew<vtkConvertToPartitionedDataSetCollection> converter;
    converter->SetInputDataObject(inputComposite);
    converter->Update();
    vtkPartitionedDataSetCollection* inputPDC = converter->GetOutput();

    if (!inputPDC)
    {
      vtkErrorMacro(<< "Unable to convert composite input to Partitioned DataSet Collection");
      return 0;
    }

    outputPDC->Initialize();
    vtkDataAssembly* inputHierarchy = inputPDC->GetDataAssembly();
    if (inputHierarchy)
    {
      vtkNew<vtkDataAssembly> outputHierarchy;
      outputHierarchy->DeepCopy(inputHierarchy);
      outputPDC->SetDataAssembly(outputHierarchy);
    }

    outputPDC->SetNumberOfPartitionedDataSets(inputPDC->GetNumberOfPartitionedDataSets());

    // Dispatch individual partitioned datasets from the input PDC
    vtkWarningMacro(<< "#PDS " << inputPDC->GetNumberOfPartitionedDataSets());
    for (unsigned int pdsIdx = 0; pdsIdx < inputPDC->GetNumberOfPartitionedDataSets(); pdsIdx++)
    {
      auto currentPDS = inputPDC->GetPartitionedDataSet(pdsIdx);
      if (!currentPDS)
      {
        continue;
      }

      vtkWarningMacro(<< "PDS # " << pdsIdx << " has " << currentPDS->GetNumberOfPartitions()
                      << " partitions");

      auto pdsHTG = vtkHyperTreeGrid::SafeDownCast(currentPDS->GetPartitionAsDataObject(0));
      vtkWarningMacro(<< "PDS #" << pdsIdx << (pdsHTG ? " is HTG" : " is not HTG"));

      const int pdsIsHTG = pdsHTG != nullptr; // Needs to be int to be reduced
      int somePartitionedDSAreHTG = true;
      controller->AllReduce(&pdsIsHTG, &somePartitionedDSAreHTG, 1, vtkCommunicator::LOGICAL_OR_OP);
      vtkWarningMacro("REDUCED ALL PDS IS HTG");
      if (somePartitionedDSAreHTG)
      {
        // HTG GCG cannot handle PDS input, so we apply the filter to each individual partition
        vtkNew<vtkPartitionedDataSet> outputPDS;
        outputPDS->SetNumberOfPartitions(currentPDS->GetNumberOfPartitions());

        vtkWarningMacro(<< "PDS #" << pdsIdx << " has " << currentPDS->GetNumberOfPartitions()
                        << " partitions");

        vtkWarningMacro("Forward pds to HTG GCG");

        const int result = this->GhostCellsGeneratorUsingHyperTreeGrid(currentPDS, outputPDS);
        if (result != 1)
        {
          vtkErrorMacro(<< "Failed to generate ghost cells for partitioned dataset #" << pdsIdx);
          return 0;
        }

        vtkWarningMacro(<< "output PDS #" << pdsIdx << " has " << outputPDS->GetNumberOfPartitions()
                        << " partitions");

        outputPDC->SetPartitionedDataSet(pdsIdx, outputPDS);
      }
      else
      {
        // Forward the whole PDS to non-HTG GCG.
        vtkWarningMacro("Forward pds to HTG GCG");
        vtkNew<vtkPartitionedDataSet> outputPDS;
        const int result = this->GhostCellsGeneratorUsingSuperclassInstance(currentPDS, outputPDS);
        if (result != 1)
        {
          vtkErrorMacro(<< "Failed to generate ghost cells for partitioned dataset #" << pdsIdx);
          return 0;
        }
        outputPDC->SetPartitionedDataSet(pdsIdx, outputPDS);
      }
    }
    return 1;
  }

  return this->GhostCellsGeneratorUsingSuperclassInstance(input, output);
}

//----------------------------------------------------------------------------
int vtkPVGhostCellsGenerator::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}
