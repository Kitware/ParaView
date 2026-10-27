// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"
#include "vtkDummyController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPlotEdges.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace
{
vtkIdType SortedPointCount(const std::vector<std::pair<vtkIdType, vtkIdType>>& segments,
  vtkIdType numberOfPoints, int& numberOfBlocks)
{
  vtkNew<vtkPoints> points;
  for (vtkIdType i = 0; i < numberOfPoints; ++i)
  {
    points->InsertNextPoint(static_cast<double>(i), 0.0, 0.0);
  }
  vtkNew<vtkCellArray> lines;
  for (const auto& segment : segments)
  {
    const vtkIdType ids[2] = { segment.first, segment.second };
    lines->InsertNextCell(2, ids);
  }
  vtkNew<vtkPolyData> input;
  input->SetPoints(points);
  input->SetLines(lines);

  vtkNew<vtkPlotEdges> plotEdges;
  plotEdges->SetInputData(input);
  plotEdges->Update();

  vtkMultiBlockDataSet* output = plotEdges->GetOutput();
  numberOfBlocks = static_cast<int>(output->GetNumberOfBlocks());
  vtkIdType count = 0;
  for (int i = 0; i < numberOfBlocks; ++i)
  {
    if (auto* block = vtkPolyData::SafeDownCast(output->GetBlock(i)))
    {
      count += block->GetNumberOfPoints();
    }
  }
  return count;
}
}

int TestPlotEdgesUnorderedSegments(int, char*[])
{
  vtkNew<vtkDummyController> controller;
  vtkMultiProcessController::SetGlobalController(controller);

  // Segments of one open polyline 0-1-2-3-4, listed out of walk order.
  int blocks = 0;
  vtkIdType count = SortedPointCount({ { 2, 3 }, { 0, 1 }, { 3, 4 }, { 1, 2 } }, 5, blocks);
  if (blocks != 1 || count != 5)
  {
    std::cerr << "Unordered polyline: expected 1 block of 5 points, got " << blocks
              << " block(s) with " << count << " points." << std::endl;
    vtkMultiProcessController::SetGlobalController(nullptr);
    return EXIT_FAILURE;
  }

  // Two disjoint polylines 0-1-2-3 and 5-6-7-8, segments interleaved.
  count =
    SortedPointCount({ { 6, 7 }, { 1, 2 }, { 5, 6 }, { 2, 3 }, { 7, 8 }, { 0, 1 } }, 9, blocks);
  if (blocks != 2 || count != 8)
  {
    std::cerr << "Two polylines: expected 2 blocks with 8 points, got " << blocks
              << " block(s) with " << count << " points." << std::endl;
    vtkMultiProcessController::SetGlobalController(nullptr);
    return EXIT_FAILURE;
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  return EXIT_SUCCESS;
}
