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
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedLongArray.h"

/// Test the output of the vtkExtractHistogram filter in a simple serial case
int main(int, char*[])
{
  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
  
  vtkSmartPointer<vtkExtractHistogram> extraction = vtkSmartPointer<vtkExtractHistogram>::New();
  
  const unsigned long bin_count = 3;
  
  extraction->SetInputConnection(sphere->GetOutputPort());
  extraction->SetInputArrayToProcess(0, 0, 0, vtkDataSet::FIELD_ASSOCIATION_POINTS_THEN_CELLS, "Normals");
  extraction->SetComponent(0);
  extraction->SetBinCount(bin_count);
  
  vtkPolyData* const histogram = extraction->GetOutput();
  histogram->Update();

  vtkDoubleArray* const bin_extents = vtkDoubleArray::SafeDownCast(histogram->GetCellData()->GetArray("bin_extents"));
  if(!bin_extents)
    {
    return 1;
    }

  if(bin_extents->GetNumberOfComponents() != 1)
    {
    return 1;
    }
  
  if(bin_extents->GetNumberOfTuples() != bin_count + 1)
    {
    return 1;
    }

  vtkUnsignedLongArray* const bin_values = vtkUnsignedLongArray::SafeDownCast(histogram->GetCellData()->GetArray("bin_values"));
  if(!bin_values)
    {
    return 1;
    }

  if(bin_values->GetNumberOfComponents() != 1)
    {
    return 1;
    }
  if(bin_values->GetNumberOfTuples() != bin_count)
    {
    return 1;
    }

  if(bin_values->GetComponent(0, 0) != 14)
    {
    return 1;
    }
  if(bin_values->GetComponent(1, 0) != 22)
    {
    return 1;
    }
  if(bin_values->GetComponent(2, 0) != 14)
    {
    return 1;
    }
  
  return 0;
}
