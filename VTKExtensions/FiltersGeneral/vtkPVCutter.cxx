// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVCutter.h"

#include "vtkAMRDataObject.h"
#include "vtkAppendDataSets.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataObjectMeshCache.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMeshCacheRunner.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVPlaneCutter.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkType.h"

#include "Private/vtkEdgesCacheInternal.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCutter);

//----------------------------------------------------------------------------
vtkPVCutter::vtkPVCutter()
  : EdgesCache(std::make_unique<vtkEdgesCacheInternal>())
{
  this->MeshCache->SetConsumer(this);
  this->MeshCache->ForwardAttribute(vtkDataObject::CELL);
}

//----------------------------------------------------------------------------
vtkPVCutter::~vtkPVCutter() = default;

//----------------------------------------------------------------------------
void vtkPVCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVCutter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  vtkDataObject* output = vtkDataObject::GetData(outInfo);

  vtkMeshCacheRunner runner{ this->MeshCache, input, output, true };
  if (runner.GetCacheLoaded())
  {
    return this->EdgesCache->UpdateAttributes(input, output) ? 1 : 0;
  }

  this->EdgesCache->InvalidateCache();

  return this->CutDataObject(request, inputVector, outputVector) ? 1 : 0;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkInformationVector> vtkPVCutter::DuplicateInfoForLeaf(
  vtkInformationVector* inputInfo, vtkDataObject* leaf)
{
  vtkNew<vtkInformationVector> leafVector;
  leafVector->Copy(inputInfo, 1);
  vtkInformation* leafInfo = leafVector->GetInformationObject(0);
  leafInfo->Set(vtkDataObject::DATA_OBJECT(), leaf);

  return leafVector;
}

//----------------------------------------------------------------------------
void vtkPVCutter::InitializeOutput(vtkDataObjectTree* treeInput, vtkDataObjectTree* treeOutput)
{
  // initialize output: polydata everywhere
  treeOutput->CopyStructure(treeInput);

  vtkSmartPointer<vtkDataObjectTreeIterator> outIter;
  outIter.TakeReference(treeOutput->NewTreeIterator());
  outIter->SkipEmptyNodesOff();

  for (outIter->InitTraversal(); !outIter->IsDoneWithTraversal(); outIter->GoToNextItem())
  {
    vtkNew<vtkPolyData> newOutput;
    treeOutput->SetDataSet(outIter, newOutput);
  }
}

//----------------------------------------------------------------------------
bool vtkPVCutter::CutDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto treeInput = vtkDataObjectTree::GetData(inputVector[0]);
  auto treeOutput = vtkDataObjectTree::GetData(outputVector);

  if (treeInput && treeOutput)
  {
    this->InitializeOutput(treeInput, treeOutput);

    bool ret = true;

    vtkSmartPointer<vtkDataObjectTreeIterator> outIter;
    outIter.TakeReference(treeOutput->NewTreeIterator());
    outIter->SkipEmptyNodesOff();

    vtkSmartPointer<vtkDataObjectTreeIterator> inputIter;
    inputIter.TakeReference(treeInput->NewTreeIterator());
    inputIter->SkipEmptyNodesOff();

    outIter->InitTraversal();
    for (inputIter->InitTraversal(); !inputIter->IsDoneWithTraversal();
         inputIter->GoToNextItem(), outIter->GoToNextItem())
    {
      if (!inputIter->GetCurrentDataObject())
      {
        // nothing to do
        continue;
      }
      // Internal API for leaf processing uses vtkInformationVector.
      // As our input is (now) a Composite, we should replace the DATA_OBJECT key content
      // by the current leaf dataset.
      auto inLeafVector =
        this->DuplicateInfoForLeaf(inputVector[0], inputIter->GetCurrentDataObject());
      std::vector<vtkInformationVector*> inLeafVectors;
      inLeafVectors.push_back(inLeafVector);
      auto outLeafVector =
        this->DuplicateInfoForLeaf(outputVector, outIter->GetCurrentDataObject());

      ret = ret && this->CutLeaf(request, inLeafVectors.data(), outLeafVector);
    }
    return ret;
  }
  else
  {
    return this->CutLeaf(request, inputVector, outputVector);
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkPVCutter::CutLeaf(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkDataObject* output = vtkDataObject::GetData(outputVector);

  vtkPlane* plane = vtkPlane::SafeDownCast(this->CutFunction);

  auto executePlaneCutter = [&](double offset)
  {
    // Create a copy of the original plane and apply the offsets value
    // (global offset + "contours")
    vtkNew<vtkPlane> newPlane;
    newPlane->DeepCopy(plane);
    newPlane->SetOffset(plane->GetOffset() + offset);

    this->PlaneCutter->SetInputData(input);
    this->PlaneCutter->SetPlane(newPlane);
    bool mergePoints = this->GetLocator() && !this->GetLocator()->IsA("vtkNonMergingPointLocator");
    this->PlaneCutter->SetMergePoints(mergePoints);
    this->PlaneCutter->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
    this->PlaneCutter->SetGeneratePolygons(!this->GetGenerateTriangles());
    this->PlaneCutter->SetDual(this->GetDual());
    this->PlaneCutter->BuildTreeOff();
    this->PlaneCutter->ComputeNormalsOff();
    this->PlaneCutter->Update();
  };

  if (plane && this->GetGenerateCutScalars() == 0)
  {
    if (vtkHyperTreeGrid::SafeDownCast(input))
    {
      if (this->Locator == nullptr)
      {
        this->CreateDefaultLocator();
      }

      const bool multipleSlices = this->GetNumberOfContours() > 1;
      if (multipleSlices)
      {
        vtkNew<vtkAppendDataSets> append;
        append->SetContainerAlgorithm(this);
        append->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
        append->MergePointsOff();
        append->SetOutputDataSetType(VTK_POLY_DATA);

        for (vtkIdType i = 0; i < this->GetNumberOfContours(); ++i)
        {
          auto offset = this->GetValue(i);
          executePlaneCutter(offset);

          vtkNew<vtkPolyData> pd;
          pd->ShallowCopy(this->PlaneCutter->GetOutput());
          append->AddInputData(pd);
        }
        append->Update();
        output->ShallowCopy(append->GetOutput());
      }
      else
      {
        executePlaneCutter(0);
        output->ShallowCopy(this->PlaneCutter->GetOutput());
      }
      return true;
    }
    else
    {
      return this->CutUsingSuperclassInstance(request, inputVector, outputVector) == 1;
    }
  }
  else
  {
    // vtkHyperTreeGrid support only plane as cut function
    if (vtkHyperTreeGrid::SafeDownCast(input))
    {
      vtkErrorMacro(<< input->GetClassName()
                    << " doesn't support the cut function: " << this->CutFunction->GetClassName());
      return false;
    }
    else
    {
      return this->CutUsingSuperclassInstance(request, inputVector, outputVector) == 1;
    }
  }
}

//----------------------------------------------------------------------------
int vtkPVCutter::CutUsingSuperclassInstance(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkNew<Superclass> instance;

  for (vtkIdType i = 0; i < this->GetNumberOfContours(); ++i)
  {
    instance->SetValue(i, this->GetValue(i));
  }

  instance->SetCutFunction(this->GetCutFunction());
  instance->SetGenerateCutScalars(this->GetGenerateCutScalars());
  instance->SetGenerateTriangles(this->GetGenerateTriangles());
  instance->SetLocator(this->GetLocator());
  instance->SetSortBy(this->GetSortBy());
  instance->SetOutputPointsPrecision(this->GetOutputPointsPrecision());

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  instance->SetInputDataObject(inputDO);
  instance->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
  if (instance->GetExecutive()->Update())
  {
    outputDO->ShallowCopy(instance->GetOutput());
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPVCutter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVCutter::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVCutter::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVCutter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  auto inputDO = vtkDataObject::GetData(inInfo);
  if (!this->GetCutFunction())
  {
    vtkErrorMacro(<< "No cut function");
    return 0;
  }

  vtkDataObject* newOutput = nullptr;
  if (vtkHyperTreeGrid::SafeDownCast(inputDO))
  {
    vtkPlane* plane = vtkPlane::SafeDownCast(this->GetCutFunction());
    if (!plane)
    {
      vtkErrorMacro(<< "Cut function not supported for vtkHyperTreeGrid");
      return 0;
    }
    if (!vtkPolyData::GetData(outInfo))
    {
      newOutput = vtkPolyData::New();
    }
  }
  else if (vtkAMRDataObject::SafeDownCast(inputDO))
  {
    // We do not produces AMR in output. for compat with the CompositePipeline, turn it into
    // MultiBlock.
    if (!vtkMultiBlockDataSet::GetData(outInfo))
    {
      newOutput = vtkMultiBlockDataSet::New();
    }
  }
  else if (vtkDataSet::SafeDownCast(inputDO))
  {
    if (!vtkPolyData::GetData(outInfo))
    {
      newOutput = vtkPolyData::New();
    }
  }
  else if (vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    vtkCompositeDataSet* compositeInput = vtkCompositeDataSet::SafeDownCast(inputDO);
    if (!vtkCompositeDataSet::GetData(outInfo))
    {
      newOutput = compositeInput->NewInstance();
    }
  }

  if (newOutput)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    this->GetOutputPortInformation(0)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
    newOutput->FastDelete();
  }

  return 1;
}
