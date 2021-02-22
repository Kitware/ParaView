/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceAlongPolyPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSliceAlongPolyPlane.h"

#include "vtkAppendArcLength.h"
#include "vtkCellArray.h"
#include "vtkCleanPolyData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCutter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkMPIMoveData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyLine.h"
#include "vtkPolyPlane.h"
#include "vtkProbeFilter.h"
#include "vtkSmartPointer.h"
#include "vtkThreshold.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkType.h"

#include <algorithm>
#include <iostream>
#include <list>

vtkStandardNewMacro(vtkSliceAlongPolyPlane);

//----------------------------------------------------------------------------
void vtkSliceAlongPolyPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Tolerance: " << this->Tolerance << endl;
}

//----------------------------------------------------------------------------
vtkSliceAlongPolyPlane::vtkSliceAlongPolyPlane()
{
  this->SetNumberOfInputPorts(2);
  this->Tolerance = 10;
}

//----------------------------------------------------------------------------
vtkSliceAlongPolyPlane::~vtkSliceAlongPolyPlane() = default;

//----------------------------------------------------------------------------
int vtkSliceAlongPolyPlane::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // If input is vtkCompositeDataSet, output will be a vtkMultiBlockDataSet
  // otherwise it will be a vtkPolyData.
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  if (vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    if (vtkMultiBlockDataSet::GetData(outputVector, 0) == nullptr)
    {
      vtkNew<vtkMultiBlockDataSet> output;
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output.GetPointer());
    }
    return 1;
  }
  else if (vtkDataSet::GetData(inputVector[0], 0))
  {
    if (vtkPolyData::GetData(outputVector, 0) == nullptr)
    {
      vtkNew<vtkPolyData> output;
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output.GetPointer());
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkSliceAlongPolyPlane::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
// A struct that holds the id and direction of a polyline to be appended
typedef struct
{
  vtkIdType id;
  bool forward;
} linePiece;

//----------------------------------------------------------------------------
static bool operator==(const linePiece& a, const linePiece& b)
{
  return a.id == b.id;
}

//----------------------------------------------------------------------------
static void appendLinesIntoOnePolyLine(vtkPolyData* input, vtkPolyData* output)
{
  if (input->GetNumberOfPoints() == 0 || input->GetNumberOfCells() == 0)
  {
    output->ShallowCopy(input);
    return;
  }
  output->Initialize();
  output->Allocate();

  // copy points to the output
  output->GetPointData()->ShallowCopy(input->GetPointData());
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->ShallowCopy(input->GetPoints());
  output->SetPoints(points);
  vtkIdType numCells = input->GetNumberOfCells();
  // make sure the first cell is a line or polyline
  vtkCell* first = input->GetCell(0);
  vtkLine* firstLine = vtkLine::SafeDownCast(first);
  vtkPolyLine* firstPolyLine = vtkPolyLine::SafeDownCast(first);
  if (!firstLine && !firstPolyLine)
  {
    // if not it is an error
    return;
  }
  linePiece firstPiece = { 0, true };
  vtkIdType totalPointsInLine = first->GetNumberOfPoints();

  std::list<linePiece> pieceList;
  pieceList.push_back(firstPiece);
  bool foundANewPiece = true;
  while (foundANewPiece)
  {
    foundANewPiece = false;
    // look for pieces that end at the start of the first segment
    {
      vtkCell* frontCell = input->GetCell(pieceList.front().id);
      vtkIdType frontPointNumberInCell =
        pieceList.front().forward ? 0 : frontCell->GetNumberOfPoints() - 1;
      vtkIdType firstPoint = frontCell->GetPointId(frontPointNumberInCell);
      for (int i = 0; i < numCells; ++i)
      {
        vtkCell* c = input->GetCell(i);
        if (!vtkPolyLine::SafeDownCast(c) && !vtkLine::SafeDownCast(c))
        {
          continue;
        }
        if (c->GetPointId(0) == firstPoint ||
          c->GetPointId(c->GetNumberOfPoints() - 1) == firstPoint)
        {
          linePiece l = { i, c->GetPointId(0) != firstPoint };
          if (std::find(pieceList.begin(), pieceList.end(), l) == pieceList.end())
          {
            pieceList.push_front(l);
            foundANewPiece = true;
            totalPointsInLine += c->GetNumberOfPoints() - 1;
            break;
          }
        }
      }
    }
    // look for pieces that start at the end of the last segment
    {
      vtkCell* backCell = input->GetCell(pieceList.back().id);
      vtkIdType backPointNumberInCell =
        pieceList.back().forward ? backCell->GetNumberOfPoints() - 1 : 0;
      vtkIdType lastPoint = backCell->GetPointId(backPointNumberInCell);
      for (int i = 0; i < numCells; ++i)
      {
        vtkCell* c = input->GetCell(i);
        if (!vtkPolyLine::SafeDownCast(c) && !vtkLine::SafeDownCast(c))
        {
          continue;
        }
        if (c->GetPointId(0) == lastPoint || c->GetPointId(c->GetNumberOfPoints() - 1 == lastPoint))
        {
          linePiece l = { i, c->GetPointId(0) == lastPoint };
          if (std::find(pieceList.begin(), pieceList.end(), l) == pieceList.end())
          {
            pieceList.push_back(l);
            foundANewPiece = true;
            totalPointsInLine += c->GetNumberOfPoints() - 1;
            break;
          }
        }
      }
    }
  }
  vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
  idList->SetNumberOfIds(totalPointsInLine);
  vtkIdType current = 0;
  // Append the lines into a polyline
  for (std::list<linePiece>::iterator itr = pieceList.begin(); itr != pieceList.end(); ++itr)
  {
    vtkCell* cell = input->GetCell((*itr).id);
    vtkIdType start = (*itr).forward ? 0 : cell->GetNumberOfPoints() - 1;
    vtkIdType end = (*itr).forward ? cell->GetNumberOfPoints() : -1;
    vtkIdType increment = (*itr).forward ? 1 : -1;
    // except for the first time, we already have the first point,
    // it was the last point in the previous cell
    if (current != 0)
    {
      start += increment;
    }
    for (vtkIdType i = start; i != end; i += increment)
    {
      vtkIdType id = cell->GetPointId(i);
      assert(id < input->GetNumberOfPoints());
      idList->SetId(current, id);
      ++current;
    }
  }
  assert(current == totalPointsInLine);
  output->InsertNextCell(VTK_POLY_LINE, idList);
}

//----------------------------------------------------------------------------
void vtkSliceAlongPolyPlane::CleanPolyLine(vtkPolyData* input, vtkPolyData* output)
{
  output->ShallowCopy(input);

  // remove other cell types that we don't care about.
  output->SetVerts(nullptr);
  output->SetPolys(nullptr);
  output->SetVerts(nullptr);

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    // communicate this dataset among all processes. This will ensure that the
    // polyline is cloned and communicated to all ranks.
    vtkNew<vtkMPIMoveData> moveData;
    moveData->SetController(controller);
    moveData->SetServerToDataServer();
    moveData->SetMoveModeToClone();
    moveData->SetOutputDataType(VTK_POLY_DATA);
    moveData->SetInputData(output);
    moveData->Update();
    output->ShallowCopy(moveData->GetOutputDataObject(0));
  }

  // this method conditions the input polydata to extract a single polyline from
  // it. It also handles parallel use-cases.
  vtkNew<vtkCleanPolyData> cleanFilter;
  cleanFilter->SetInputData(output);
  cleanFilter->Update();

  appendLinesIntoOnePolyLine(cleanFilter->GetOutput(), output);
}

//----------------------------------------------------------------------------
int vtkSliceAlongPolyPlane::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{

  vtkNew<vtkPolyData> polyLinePD;
  this->CleanPolyLine(vtkPolyData::GetData(inputVector[1], 0), polyLinePD.GetPointer());

  if (vtkPolyLine::SafeDownCast(polyLinePD->GetCell(0)) == nullptr)
  {
    vtkErrorMacro(<< " First cell in input polydata is not a vtkPolyLine.");
    return 0;
  }

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  if (vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    vtkCompositeDataSet* output = vtkCompositeDataSet::GetData(outputVector, 0);
    assert(output);
    output->CopyStructure(cd);

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
      {
        vtkNew<vtkPolyData> pd;
        if (!this->Execute(ds, polyLinePD.GetPointer(), pd.GetPointer()))
        {
          return 0;
        }
        output->SetDataSet(iter, pd.GetPointer());
      }
    }
    return 1;
  }
  else if (vtkDataSet* ds = vtkDataSet::SafeDownCast(inputDO))
  {
    vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);
    assert(output);
    return this->Execute(ds, polyLinePD.GetPointer(), output) ? 1 : 0;
  }

  return 0;
}

//----------------------------------------------------------------------------
bool vtkSliceAlongPolyPlane::Execute(
  vtkDataSet* inputDataset, vtkPolyData* lineDataSet, vtkPolyData* output)
{
  assert(inputDataset && lineDataSet && output);

  vtkPolyLine* line = vtkPolyLine::SafeDownCast(lineDataSet->GetCell(0));
  assert(line);

  vtkNew<vtkPolyPlane> planes;
  planes->SetPolyLine(line);

  vtkNew<vtkCutter> cutter;
  cutter->SetCutFunction(planes.GetPointer());
  cutter->SetInputData(inputDataset);
  cutter->Update();

  vtkDataSet* cutterOutput = vtkDataSet::SafeDownCast(cutter->GetOutputDataObject(0));
  if (cutterOutput == nullptr || cutterOutput->GetNumberOfPoints() == 0)
  {
    // shortcut if the cut produces an empty dataset.
    return 1;
  }

  vtkNew<vtkTransformPolyDataFilter> transformOutputFilter;
  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->Scale(1, 1, 0);
  transformOutputFilter->SetTransform(transform);
  transformOutputFilter->SetInputData(cutterOutput);
  transformOutputFilter->Update();

  vtkNew<vtkAppendArcLength> arcLengthFilter;
  arcLengthFilter->SetInputData(lineDataSet);
  arcLengthFilter->Update();

  vtkNew<vtkTransformPolyDataFilter> transformLineFilter;
  transformLineFilter->SetTransform(transform);
  transformLineFilter->SetInputConnection(arcLengthFilter->GetOutputPort());
  transformLineFilter->Update();

  vtkNew<vtkProbeFilter> probe;
  probe->SetInputConnection(transformOutputFilter->GetOutputPort());
  probe->SetSourceConnection(transformLineFilter->GetOutputPort());
  probe->SetPassPointArrays(1);
  probe->SetPassCellArrays(1);
  probe->SetValidPointMaskArrayName("vtkValidPointMaskArrayName");
  probe->ComputeToleranceOff();
  probe->SetTolerance(this->Tolerance);
  probe->Update();

  vtkNew<vtkPolyData> combinationPolyData;
  combinationPolyData->ShallowCopy(cutter->GetOutput());
  combinationPolyData->GetPointData()->ShallowCopy(probe->GetOutput()->GetPointData());

  vtkNew<vtkThreshold> threshold;
  threshold->SetInputData(combinationPolyData.GetPointer());
  threshold->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "vtkValidPointMaskArrayName");
  threshold->ThresholdBetween(0.5, 1.5);
  threshold->Update();

  vtkNew<vtkDataSetSurfaceFilter> toPolyData;
  toPolyData->SetInputConnection(threshold->GetOutputPort());
  toPolyData->Update();

  output->ShallowCopy(toPolyData->GetOutput());
  output->GetPointData()->RemoveArray("functionValues");
  output->GetPointData()->RemoveArray("vtkValidPointMaskArrayName");

  return true;
}
