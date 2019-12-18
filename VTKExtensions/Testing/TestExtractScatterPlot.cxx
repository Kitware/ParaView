/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractScatterPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkExtractScatterPlot.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedLongArray.h"

/// Test the output of the vtkExtractHistogram filter in a simple serial case
int TestExtractScatterPlot(int, char* [])
{
  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();

  vtkSmartPointer<vtkExtractScatterPlot> extraction = vtkSmartPointer<vtkExtractScatterPlot>::New();

  const int bin_count = 3;

  extraction->SetInputConnection(sphere->GetOutputPort());
  extraction->SetInputArrayToProcess(
    0, 0, 0, vtkDataSet::FIELD_ASSOCIATION_POINTS_THEN_CELLS, "Normals");
  extraction->SetInputArrayToProcess(
    1, 0, 0, vtkDataSet::FIELD_ASSOCIATION_POINTS_THEN_CELLS, "Normals");
  extraction->SetXComponent(0);
  extraction->SetYComponent(1);
  extraction->SetXBinCount(bin_count);
  extraction->SetYBinCount(bin_count);
  extraction->Update();

  vtkPolyData* const scatter_plot = extraction->GetOutput();

  vtkDoubleArray* const x_bin_extents =
    vtkDoubleArray::SafeDownCast(scatter_plot->GetCellData()->GetArray("x_bin_extents"));
  if (!x_bin_extents)
  {
    return 1;
  }
  if (x_bin_extents->GetNumberOfComponents() != 1)
  {
    return 1;
  }
  if (x_bin_extents->GetNumberOfTuples() != bin_count + 1)
  {
    return 1;
  }

  vtkDoubleArray* const y_bin_extents =
    vtkDoubleArray::SafeDownCast(scatter_plot->GetCellData()->GetArray("y_bin_extents"));
  if (!y_bin_extents)
  {
    return 1;
  }
  if (y_bin_extents->GetNumberOfComponents() != 1)
  {
    return 1;
  }
  if (y_bin_extents->GetNumberOfTuples() != bin_count + 1)
  {
    return 1;
  }

  vtkUnsignedLongArray* const bin_values =
    vtkUnsignedLongArray::SafeDownCast(scatter_plot->GetCellData()->GetArray("bin_values"));
  if (!bin_values)
  {
    return 1;
  }
  if (bin_values->GetNumberOfComponents() != bin_count)
  {
    return 1;
  }
  if (bin_values->GetNumberOfTuples() != bin_count)
  {
    return 1;
  }

  int count = 0;
  for (int i = 0; i != bin_count; ++i)
  {
    for (int j = 0; j != bin_count; ++j)
    {
      count += static_cast<int>(bin_values->GetComponent(i, j));
    }
  }

  if (count != 50)
    return 1;

  return 0;
}
