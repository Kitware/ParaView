/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkExtractHistogram.h"
#include "vtkIntArray.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTable.h"

/// Test the output of the vtkExtractHistogram filter in a simple serial case
int TestExtractHistogram(int, char* [])
{
  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkExtractHistogram> extraction = vtkSmartPointer<vtkExtractHistogram>::New();

  const int bin_count = 3;

  extraction->SetInputConnection(sphere->GetOutputPort());
  extraction->SetInputArrayToProcess(
    0, 0, 0, vtkDataSet::FIELD_ASSOCIATION_POINTS_THEN_CELLS, "Normals");
  extraction->SetComponent(0);
  extraction->SetBinCount(bin_count);
  extraction->Update();

  vtkTable* const histogram = extraction->GetOutput();

  vtkDoubleArray* const bin_extents =
    vtkDoubleArray::SafeDownCast(histogram->GetRowData()->GetArray("bin_extents"));

  if (!bin_extents)
  {
    vtkGenericWarningMacro("No bin extents found.");
    return 1;
  }

  if (bin_extents->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("XCoordinates must be a  1 component array.");
    return 1;
  }

  if (bin_extents->GetNumberOfTuples() != bin_count)
  {
    vtkGenericWarningMacro("Incorrect number of  bins.");
    return 1;
  }

  vtkIntArray* const bin_values =
    vtkIntArray::SafeDownCast(histogram->GetRowData()->GetArray((int)1));
  if (!bin_values)
  {
    vtkGenericWarningMacro("cell data missing.");
    return 1;
  }

  if (bin_values->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("bin_values must be a 1 component array.") return 1;
  }
  if (bin_values->GetNumberOfTuples() != bin_count)
  {
    vtkGenericWarningMacro("bin_count don;t have enough values.");
    return 1;
  }

  if (bin_values->GetComponent(0, 0) != 14)
  {
    vtkGenericWarningMacro("incorrect bin value.");
    return 1;
  }
  if (bin_values->GetComponent(1, 0) != 22)
  {
    vtkGenericWarningMacro("incorrect bin value.");
    return 1;
  }
  if (bin_values->GetComponent(2, 0) != 14)
  {
    vtkGenericWarningMacro("incorrect bin value.");
    return 1;
  }
  return 0;
}
