// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVCutter.h"

#include "vtkAppendDataSets.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVPlane.h"
#include "vtkPVPlaneCutter.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGridBase.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCutter);

//----------------------------------------------------------------------------
vtkPVCutter::vtkPVCutter() = default;

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

  vtkPVPlane* plane = vtkPVPlane::SafeDownCast(this->CutFunction);

  auto executePlaneCutter = [&](double offset) {
    // Create a copy of the original plane and apply the offsets value
    // (global offset + "contours")
    vtkNew<vtkPVPlane> newPlane;
    newPlane->SetNormal(plane->GetNormal());
    newPlane->SetOrigin(plane->GetOrigin());
    // We should not use `Push` here since it does not apply on
    // the internal plane of vtkPVPlane
    newPlane->SetOffset(plane->GetOffset() + offset);
    newPlane->SetAxisAligned(plane->GetAxisAligned());

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

      bool multipleSlices = this->GetNumberOfContours() > 1;

      // PARAVIEW_DEPRECATED_IN_5_13_0("Use vtkAxisAlignedCutter instead")
      // We do not support multiple axis-aligned slices in this filter. Following assertion can be
      // removed once the specific axis-aligned codepath is dropped from vtkPVPlaneCutter.
      if (plane->GetAxisAligned())
      {
        multipleSlices = false;
      }

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
      return 1;
    }
    else
    {
      return this->CutUsingSuperclassInstance(request, inputVector, outputVector);
    }
  }
  else
  {
    // vtkHyperTreeGrid support only plane as cut function
    if (vtkHyperTreeGrid::SafeDownCast(input))
    {
      vtkErrorMacro(<< input->GetClassName()
                    << " doesn't support the cut function: " << this->CutFunction->GetClassName());
      return 0;
    }
    else
    {
      return this->CutUsingSuperclassInstance(request, inputVector, outputVector);
    }
  }
}

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

  if (vtkHyperTreeGrid::SafeDownCast(inputDO))
  {
    vtkPVPlane* plane = vtkPVPlane::SafeDownCast(this->GetCutFunction());
    if (!plane)
    {
      vtkErrorMacro(<< "Cut function not supported for vtkHyperTreeGrid");
      return 0;
    }
    if (plane->GetAxisAligned())
    {
      // PARAVIEW_DEPRECATED_IN_5_13_0("Use vtkAxisAlignedCutter instead")
      vtkHyperTreeGrid* output = vtkHyperTreeGrid::GetData(outInfo);
      if (!output)
      {
        output = vtkHyperTreeGrid::New();
        outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
        this->GetOutputPortInformation(0)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
        output->FastDelete();
      }
    }
    else
    {
      vtkPolyData* output = vtkPolyData::GetData(outInfo);
      if (!output)
      {
        output = vtkPolyData::New();
        outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
        this->GetOutputPortInformation(0)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
        output->FastDelete();
      }
    }
    return 1;
  }
  else if (vtkDataSet::SafeDownCast(inputDO))
  {
    vtkPolyData* output = vtkPolyData::GetData(outInfo);
    if (!output)
    {
      output = vtkPolyData::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->FastDelete();
    }
    return 1;
  }
  return 0;
}
