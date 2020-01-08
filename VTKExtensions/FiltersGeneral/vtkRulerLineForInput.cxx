/*=========================================================================

  Program:   ParaView
  Module:    vtkRulerLineForInput.cxx

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
#include "vtkRulerLineForInput.h"

#include "vtkBoundingBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOBBTree.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkRulerLineForInput);
vtkCxxSetObjectMacro(vtkRulerLineForInput, Controller, vtkMultiProcessController);

vtkRulerLineForInput::vtkRulerLineForInput()
  : Controller(NULL)
  , Axis(0)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

vtkRulerLineForInput::~vtkRulerLineForInput()
{
  this->SetController(NULL);
}

void vtkRulerLineForInput::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkRulerLineForInput::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
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
  vtkDataObject* inputData = vtkDataObject::GetData(inVectors[0], 0);
  vtkBoundingBox bbox;
  vtkDataSet* dataset;

  if ((dataset = vtkDataSet::SafeDownCast(inputData)))
  {
    double bounds[6];
    dataset->GetBounds(bounds);
    bbox.AddBounds(bounds);
  }
  else
  {
    vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::SafeDownCast(inputData);
    assert(multiBlock);
    vtkSmartPointer<vtkCompositeDataIterator> itr =
      vtkSmartPointer<vtkCompositeDataIterator>::Take(multiBlock->NewIterator());
    for (itr->InitTraversal(); !itr->IsDoneWithTraversal(); itr->GoToNextItem())
    {
      vtkDataObject* block = itr->GetCurrentDataObject();
      vtkDataSet* blockAsDataset = vtkDataSet::SafeDownCast(block);
      if (blockAsDataset)
      {
        double tmpBounds[6];
        blockAsDataset->GetBounds(tmpBounds);
        bbox.AddBounds(tmpBounds);
      }
    }
  }

  double globalBounds[6];
  if (this->Controller)
  {
    double processBounds[6];
    bbox.GetBounds(processBounds);
    for (int i = 0; i < 3; ++i)
    {
      processBounds[2 * i] *= -1;
    }
    this->Controller->Reduce(&processBounds[0], &globalBounds[0], 6, vtkCommunicator::MAX_OP, 0);
    for (int i = 0; i < 3; ++i)
    {
      globalBounds[2 * i] *= -1;
    }
  }
  else
  {
    bbox.GetBounds(globalBounds);
  }

  double corner[3], max[3], mid[3], min[3], size[3];
  vtkSmartPointer<vtkPoints> points;
  if (this->Axis >= 3 && this->Axis <= 5)
  {
    auto pointSet = vtkPointSet::SafeDownCast(inputData);
    if (pointSet)
    {
      points = pointSet->GetPoints();
      if (!points)
      {
        points = vtkSmartPointer<vtkPoints>::New();
      }
    }
    else if (auto mbds = vtkMultiBlockDataSet::SafeDownCast(inputData))
    {
      // Merge points from blocks into a new vtkPointsObject.
      points = vtkSmartPointer<vtkPoints>::New();
      vtkSmartPointer<vtkCompositeDataIterator> itr;
      itr.TakeReference(mbds->NewIterator());
      for (itr->InitTraversal(); !itr->IsDoneWithTraversal(); itr->GoToNextItem())
      {
        auto pointSetBlock = vtkPointSet::SafeDownCast(mbds->GetDataSet(itr));
        if (!pointSetBlock)
        {
          // No points, skip.
          continue;
        }
        auto srcPoints = pointSetBlock->GetPoints();
        if (srcPoints)
        {
          points->InsertPoints(
            points->GetNumberOfPoints(), srcPoints->GetNumberOfPoints(), 0, srcPoints);
        }
      }
    }
    else
    {
      vtkErrorMacro(<< "Input data set of type " << inputData->GetClassName()
                    << " is not currently supported.");
      return 0;
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
