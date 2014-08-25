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

#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkPolyPlane.h"
#include "vtkLine.h"
#include "vtkPolyLine.h"
#include "vtkCutter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkRectilinearGrid.h"
#include "vtkImageData.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkCleanPolyData.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkProbeFilter.h"
#include "vtkThreshold.h"
#include "vtkAppendArcLength.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataSetSurfaceFilter.h"

#include <list>
#include <algorithm>
#include <iostream>

vtkStandardNewMacro(vtkSliceAlongPolyPlane)

void vtkSliceAlongPolyPlane::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  // TODO
}

vtkSliceAlongPolyPlane::vtkSliceAlongPolyPlane()
{
  this->SetNumberOfInputPorts(2);
  this->Tolerance = 10;
}

vtkSliceAlongPolyPlane::~vtkSliceAlongPolyPlane()
{
}

int vtkSliceAlongPolyPlane::FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == 0)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
    }
  return 0;
}

// A struct that holds the id and direction of a polyline to be appended
typedef struct {
  vtkIdType id;
  bool forward;
} linePiece;

static bool operator==(const linePiece &a, const linePiece &b)
{
  return a.id == b.id;
}

static void appendLinesIntoOnePolyLine(vtkPolyData* input, vtkPolyData *output)
{
  // copy points to the output
  output->GetPointData()->ShallowCopy(input->GetPointData());
  vtkSmartPointer< vtkPoints > points =
      vtkSmartPointer< vtkPoints >::New();
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
  linePiece firstPiece = {0,true};
  vtkIdType totalPointsInLine = first->GetNumberOfPoints();

  std::list< linePiece > pieceList;
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
    for (int i = 0 ; i < numCells; ++i)
      {
      vtkCell* c = input->GetCell(i);
      if (!vtkPolyLine::SafeDownCast(c) && !vtkLine::SafeDownCast(c))
        {
        continue;
        }
      if (c->GetPointId(0) == firstPoint || c->GetPointId(c->GetNumberOfPoints() - 1) == firstPoint)
        {
        linePiece l = {i, c->GetPointId(0) != firstPoint};
        if (std::find(pieceList.begin(),pieceList.end(),l) == pieceList.end())
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
        linePiece l = {i, c->GetPointId(0) == lastPoint};
        if (std::find(pieceList.begin(),pieceList.end(),l) == pieceList.end())
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
  vtkSmartPointer< vtkIdList > idList = vtkSmartPointer< vtkIdList >::New();
  idList->SetNumberOfIds(totalPointsInLine);
  vtkIdType current = 0;
  // Append the lines into a polyline
  for (std::list<linePiece>::iterator itr = pieceList.begin(); itr != pieceList.end();++itr)
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
      idList->SetId(current,id);
      ++current;
      }
    }
  assert(current == totalPointsInLine);
  output->InsertNextCell(VTK_POLY_LINE,idList);
}

int vtkSliceAlongPolyPlane::RequestData(vtkInformation* vtkNotUsed(request),
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector)
{
  vtkDataSet* inputDataset = vtkDataSet::GetData(inputVector[0],0);
  vtkPolyData* inputPolyLine = vtkPolyData::GetData(inputVector[1],0);
  vtkPolyData* output = vtkPolyData::GetData(outputVector,0);

  vtkNew< vtkCleanPolyData > cleanFilter;
  cleanFilter->SetInputData(inputPolyLine);
  cleanFilter->Update();

  vtkNew< vtkPolyData > appendedLines;
  appendedLines->Allocate();
  appendLinesIntoOnePolyLine(cleanFilter->GetOutput(), appendedLines.GetPointer());
  vtkPolyLine* line = vtkPolyLine::SafeDownCast(appendedLines->GetCell(0));
  if (!line)
    {
    vtkErrorMacro(<< " First cell in input polydata is not a vtkPolyLine.");
    return 0;
    }
  vtkNew< vtkPolyPlane > planes;
  planes->SetPolyLine(line);

  vtkNew< vtkCutter > cutter;
  cutter->SetCutFunction(planes.GetPointer());
  cutter->SetInputData(inputDataset);

  vtkNew< vtkTransformPolyDataFilter > transformOutputFilter;
  vtkSmartPointer< vtkTransform > transform = vtkSmartPointer< vtkTransform >::New();
  transform->Identity();
  transform->Scale(1,1,0);
  transformOutputFilter->SetTransform(transform);
  transformOutputFilter->SetInputConnection(cutter->GetOutputPort());
  transformOutputFilter->Update();

  vtkNew< vtkAppendArcLength > arcLengthFilter;
  arcLengthFilter->SetInputData(appendedLines.GetPointer());
  arcLengthFilter->Update();

  vtkNew< vtkTransformPolyDataFilter > transformLineFilter;
  transformLineFilter->SetTransform(transform);
  transformLineFilter->SetInputConnection(arcLengthFilter->GetOutputPort());
  transformLineFilter->Update();

  vtkNew< vtkProbeFilter > probe;
  probe->SetInputConnection(transformOutputFilter->GetOutputPort());
  probe->SetSourceConnection(transformLineFilter->GetOutputPort());
  probe->SetPassPointArrays(1);
  probe->SetPassCellArrays(1);
  probe->SetValidPointMaskArrayName("vtkValidPointMaskArrayName");
  probe->ComputeToleranceOff();
  probe->SetTolerance(this->Tolerance);
  probe->Update();

  vtkNew< vtkPolyData > combinationPolyData;
  combinationPolyData->ShallowCopy(cutter->GetOutput());
  combinationPolyData->GetPointData()->ShallowCopy(
        probe->GetOutput()->GetPointData());

  vtkNew< vtkThreshold > threshold;
  threshold->SetInputData(combinationPolyData.GetPointer());
  threshold->SetInputArrayToProcess(
        0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,"vtkValidPointMaskArrayName");
  threshold->ThresholdBetween(0.5,1.5);
  threshold->Update();

  vtkNew< vtkDataSetSurfaceFilter > toPolyData;
  toPolyData->SetInputConnection(threshold->GetOutputPort());
  toPolyData->Update();

  output->ShallowCopy(toPolyData->GetOutput());
  output->GetPointData()->RemoveArray("functionValues");
  output->GetPointData()->RemoveArray("vtkValidPointMaskArrayName");

  return 1;
}
