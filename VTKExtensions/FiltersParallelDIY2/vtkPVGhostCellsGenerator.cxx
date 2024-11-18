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
int vtkPVGhostCellsGenerator::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  this->HasCompositeHTG = false;

  vtkDataObjectTree* inputComposite = vtkDataObjectTree::GetData(inInfo);
  if (inputComposite)
  {
    vtkSmartPointer<vtkDataObjectTreeIterator> iter;
    iter.TakeReference(inputComposite->NewTreeIterator());
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto pds = vtkPartitionedDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (vtkHyperTreeGrid::SafeDownCast(iter->GetCurrentDataObject()) ||
        (pds && pds->GetNumberOfPartitions() > 0 &&
          vtkHyperTreeGrid::SafeDownCast(pds->GetPartitionAsDataObject(0))))
      {
        const bool outputAlreadyCreated = vtkPartitionedDataSetCollection::GetData(outInfo);
        if (!outputAlreadyCreated)
        {
          vtkNew<vtkPartitionedDataSetCollection> outputPDC;
          this->GetExecutive()->SetOutputData(0, outputPDC);
          this->GetOutputPortInformation(0)->Set(
            vtkDataObject::DATA_EXTENT_TYPE(), outputPDC->GetExtentType());
          this->HasCompositeHTG = true;
        }
        return 1;
      }
    }
  }

  return Superclass::RequestDataObject(request, inputVector, outputVector);
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

  int isHTG = vtkHyperTreeGrid::SafeDownCast(input) != nullptr;
  int allAreHTG = false;
  int someAreHTG = false;
  controller->AllReduce(&isHTG, &allAreHTG, 1, vtkCommunicator::LOGICAL_AND_OP);
  controller->AllReduce(&isHTG, &someAreHTG, 1, vtkCommunicator::LOGICAL_OR_OP);
  if (allAreHTG)
  {
    // Match behavior from vtkGhostCellsGenerator
    vtkNew<vtkHyperTreeGridGhostCellsGenerator> ghostCellsGenerator;
    ghostCellsGenerator->SetInputData(input);
    ghostCellsGenerator->Update();

    if (ghostCellsGenerator->GetExecutive()->Update())
    {
      output->ShallowCopy(ghostCellsGenerator->GetOutput(0));
      return 1;
    }

    return 0;
  }
  else if (someAreHTG && !allAreHTG)
  {
    vtkWarningMacro(
      "All ranks need to contain a HyperTreeGrid piece in order to generate Ghost Cells. One "
      "or more ranks do not contain HyperTreeGrid while some do.");
    if (isHTG)
    {
      vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::SafeDownCast(output);
      outputHTG->ShallowCopy(input);
    }
    return 1;
  }

  // Check if our composite input contains HTG.
  // If it does, dispatch the blocks to the right implementation of the filter.
  auto inputComposite = vtkDataObjectTree::SafeDownCast(input);
  int isCompositeHTG = inputComposite && this->HasCompositeHTG;
  int allAreCompositeHTG = false;
  int someAreCompositeHTG = false;
  controller->AllReduce(&isCompositeHTG, &allAreCompositeHTG, 1, vtkCommunicator::LOGICAL_AND_OP);
  controller->AllReduce(&isCompositeHTG, &someAreCompositeHTG, 1, vtkCommunicator::LOGICAL_OR_OP);

  if (allAreCompositeHTG)
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
    for (unsigned int pdsIdx = 0; pdsIdx < inputPDC->GetNumberOfPartitionedDataSets(); pdsIdx++)
    {
      auto currentPDS = inputPDC->GetPartitionedDataSet(pdsIdx);
      if (!currentPDS)
      {
        continue;
      }

      auto pdsHTG = vtkHyperTreeGrid::SafeDownCast(currentPDS->GetPartitionAsDataObject(0));
      const int pdsIsHTG = pdsHTG != nullptr; // Needs to be int to be reduced
      int allPartitionedDSAreHTG = true;
      int somePartitionedDSAreHTG = false;

      controller->AllReduce(&pdsIsHTG, &allPartitionedDSAreHTG, 1, vtkCommunicator::LOGICAL_AND_OP);
      controller->AllReduce(&pdsIsHTG, &somePartitionedDSAreHTG, 1, vtkCommunicator::LOGICAL_OR_OP);

      if (allPartitionedDSAreHTG)
      {
        // HTG GCG cannot handle PDS input, so we apply the filter to each individual partition
        vtkNew<vtkPartitionedDataSet> outputPDS;
        outputPDS->SetNumberOfPartitions(currentPDS->GetNumberOfPartitions());

        for (unsigned int partId = 0; partId < currentPDS->GetNumberOfPartitions(); partId++)
        {
          vtkNew<vtkHyperTreeGridGhostCellsGenerator> ghostCellsGenerator;
          ghostCellsGenerator->SetInputData(currentPDS->GetPartitionAsDataObject(partId));
          ghostCellsGenerator->Update();

          outputPDS->SetPartition(partId, ghostCellsGenerator->GetOutput(0));
        }
        outputPDC->SetPartitionedDataSet(pdsIdx, outputPDS);
      }
      else if (somePartitionedDSAreHTG)
      {
        // Some ranks have PartitionedDatasets that are not HTG. In this case, ghost cells cannot be
        // computed, so simply copy the input.
        vtkWarningMacro("Cannot generate ghost cells for partitioned dataset #"
          << pdsIdx << ", because some MPI ranks do not contain any data.");
        vtkNew<vtkPartitionedDataSet> outputPDS;
        if (currentPDS)
        {
          outputPDS->ShallowCopy(currentPDS);
        }
        outputPDC->SetPartitionedDataSet(pdsIdx, outputPDS);
      }
      else
      {
        // Forward the whole PDS to non-HTG GCG.
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
  else if (someAreCompositeHTG && !allAreCompositeHTG)
  {
    vtkWarningMacro(
      "All ranks need to contain composite HyperTreeGrids in order to generate Ghost Cells. One "
      "or more ranks do not contain composite HyperTreeGrid while some do.");
    return Superclass::RequestData(request, inputVector, outputVector);
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
