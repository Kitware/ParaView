// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright 2013 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkRulerLineForInput.h"

#include "vtkBoundingBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTree.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOBBTree.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkRulerLineForInput);
vtkCxxSetObjectMacro(vtkRulerLineForInput, Controller, vtkMultiProcessController);

vtkRulerLineForInput::vtkRulerLineForInput()
  : Controller(nullptr)
  , Axis(0)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

vtkRulerLineForInput::~vtkRulerLineForInput()
{
  this->SetController(nullptr);
}

void vtkRulerLineForInput::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkRulerLineForInput::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

int vtkRulerLineForInput::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inVectors), vtkInformationVector* outVector)
{
  vtkInformation* outInfo = outVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

int vtkRulerLineForInput::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inVectors, vtkInformationVector* outVector)
{
  auto inputDO = vtkDataObject::GetData(inVectors[0], 0);

  vtkBoundingBox localBox;
  for (auto dataset : vtkCompositeDataSet::GetDataSets<vtkDataSet>(inputDO))
  {
    double bounds[6];
    dataset->GetBounds(bounds);
    localBox.AddBounds(bounds);
  }

  vtkBoundingBox globalBox(localBox);
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    this->Controller->AllReduce(localBox, globalBox);
  }

  double globalBounds[6];
  globalBox.GetBounds(globalBounds);

  double corner[3], max[3], mid[3], min[3], size[3];
  vtkNew<vtkPoints> points;
  if (this->Axis >= 3 && this->Axis <= 5)
  {
    auto pointsets = vtkCompositeDataSet::GetDataSets<vtkPointSet>(inputDO);
    if (pointsets.size() == 1)
    {
      // just shallow copy the data.
      points->ShallowCopy(pointsets[0]->GetPoints());
    }
    else if (pointsets.size() > 1)
    {
      for (auto block : pointsets)
      {
        // Merge points from blocks into a new vtkPointsObject.
        if (auto srcPoints = block->GetPoints())
        {
          const auto count = points->GetNumberOfPoints();
          if (count == 0)
          {
            points->SetDataType(srcPoints->GetDataType());
            points->DeepCopy(srcPoints);
          }
          else
          {
            points->InsertPoints(count, srcPoints->GetNumberOfPoints(), 0, srcPoints);
          }
        }
      }
    }

    // Fetch all points across ranks.
    // WARNING: this will be very slow and may crash your system if the data
    // does not all fit on one rank.
    bool communicate = this->Controller && this->Controller->GetNumberOfProcesses() > 1;
    int myRank = this->Controller->GetLocalProcessId();
    if (communicate)
    {
      vtkSmartPointer<vtkDataArray> globalPointsArray;
      globalPointsArray.TakeReference(points->GetData()->NewInstance());
      this->Controller->GatherV(points->GetData(), globalPointsArray, 0);
      if (myRank == 0)
      {
        points->SetData(globalPointsArray);
      }
    }

    if (!this->Controller || myRank == 0)
    {
      vtkOBBTree::ComputeOBB(points, corner, max, mid, min, size);
    }
  }

  vtkInformation* outInfo = outVector->GetInformationObject(0);
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    return 1;
  }

  vtkNew<vtkLineSource> line;
  line->SetPoint1(globalBounds[0], globalBounds[2], globalBounds[4]);
  switch (static_cast<AxisType>(this->Axis))
  {
    case AxisType::Y:
      line->SetPoint2(globalBounds[0], globalBounds[3], globalBounds[4]);
      break;
    case AxisType::Z:
      line->SetPoint2(globalBounds[0], globalBounds[2], globalBounds[5]);
      break;
    case AxisType::OrientedBoundingBoxMajorAxis:
      line->SetPoint1(corner);
      vtkMath::Add(corner, max, corner);
      line->SetPoint2(corner);
      break;
    case AxisType::OrientedBoundingBoxMediumAxis:
      line->SetPoint1(corner);
      vtkMath::Add(corner, mid, corner);
      line->SetPoint2(corner);
      break;
    case AxisType::OrientedBoundingBoxMinorAxis:
      line->SetPoint1(corner);
      vtkMath::Add(corner, min, corner);
      line->SetPoint2(corner);
      break;
    case AxisType::X:
    default:
      line->SetPoint2(globalBounds[1], globalBounds[2], globalBounds[4]);
      break;
  }
  line->Update();
  vtkPolyData* outputDataObject = vtkPolyData::GetData(outVector, 0);
  outputDataObject->ShallowCopy(line->GetOutput());
  return 1;
}
