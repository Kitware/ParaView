/*=========================================================================

  Program:   ParaView
  Module:    TestPVArrayInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFloatArray.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkSmartPointer.h"

vtkSmartPointer<vtkFloatArray> GetPolyData()
{
  vtkIdType numPts = 101;
  vtkSmartPointer<vtkFloatArray> array = vtkSmartPointer<vtkFloatArray>::New();

  array->SetNumberOfTuples(numPts);
  for (vtkIdType cc = 0; cc < numPts; ++cc)
  {
    array->SetTypedComponent(cc, 0, static_cast<float>(cc));
  }
  return array;
}

int TestPVArrayInformation(int, char* [])
{

  vtkSmartPointer<vtkFloatArray> array = GetPolyData();

  vtkNew<vtkPVArrayInformation> info;
  info->CopyFromObject(array.Get());

  // Verify minimum and maximum
  double* range = info->GetComponentRange(0);
  if (!vtkMathUtilities::FuzzyCompare(range[0], 0.0))
  {
    cerr << "ERROR: failed to find range minimum: " << range[0] << " " << 0.0 << endl;
    return EXIT_FAILURE;
  }
  if (!vtkMathUtilities::FuzzyCompare(range[1], 100.0))
  {
    cerr << "ERROR: failed to find range maximum: " << range[1] << " " << 100.0 << endl;
    return EXIT_FAILURE;
  }
  range = info->GetComponentFiniteRange(0);
  if (!vtkMathUtilities::FuzzyCompare(range[0], 0.0))
  {
    cerr << "ERROR: failed to find finite range minimum: " << range[0] << " " << 0.0 << endl;
    return EXIT_FAILURE;
  }
  if (!vtkMathUtilities::FuzzyCompare(range[1], 100.0))
  {
    cerr << "ERROR: failed to find finite range maximum: " << range[1] << " " << 100.0 << endl;
    return EXIT_FAILURE;
  }

  double rangeArray[2];
  info->GetComponentRange(0, rangeArray);
  if (!vtkMathUtilities::FuzzyCompare(rangeArray[0], 0.0))
  {
    cerr << "ERROR: failed to find range minimum: " << rangeArray[0] << " " << 0.0 << endl;
    return EXIT_FAILURE;
  }
  if (!vtkMathUtilities::FuzzyCompare(rangeArray[1], 100.0))
  {
    cerr << "ERROR: failed to find range maximum: " << rangeArray[1] << " " << 100.0 << endl;
    return EXIT_FAILURE;
  }
  info->GetComponentFiniteRange(0, rangeArray);
  if (!vtkMathUtilities::FuzzyCompare(rangeArray[0], 0.0))
  {
    cerr << "ERROR: failed to find finite range minimum: " << rangeArray[0] << " " << 0.0 << endl;
    return EXIT_FAILURE;
  }
  if (!vtkMathUtilities::FuzzyCompare(rangeArray[1], 100.0))
  {
    cerr << "ERROR: failed to find finite range maximum: " << rangeArray[1] << " " << 100.0 << endl;
    return EXIT_FAILURE;
  }

  // Add infinity and verify minimum and maximum values.
  array->SetComponent(50, 0, vtkMath::Inf());
  array->Modified();
  info->CopyFromObject(array.Get());
  range = info->GetComponentRange(0);
  if (!vtkMathUtilities::FuzzyCompare(range[0], 0.0))
  {
    cerr << "ERROR: failed to find range minimum: " << range[0] << " " << 0.0 << endl;
    return EXIT_FAILURE;
  }
  if (range[1] != vtkMath::Inf())
  {
    cerr << "ERROR: failed to find range maximum: " << range[1] << " " << vtkMath::Inf() << endl;
    return EXIT_FAILURE;
  }
  range = info->GetComponentFiniteRange(0);
  if (!vtkMathUtilities::FuzzyCompare(range[0], 0.0))
  {
    cerr << "ERROR: failed to find finite range minimum: " << range[0] << " " << 0.0 << endl;
    return EXIT_FAILURE;
  }
  if (!vtkMathUtilities::FuzzyCompare(range[1], 100.0))
  {
    cerr << "ERROR: failed to find finite range maximum: " << range[1] << " " << 100.0 << endl;
    return EXIT_FAILURE;
  }

  info->GetComponentRange(0, rangeArray);
  if (!vtkMathUtilities::FuzzyCompare(rangeArray[0], 0.0))
  {
    cerr << "ERROR: failed to find range minimum: " << rangeArray[0] << " " << 0.0 << endl;
    return EXIT_FAILURE;
  }
  if (rangeArray[1] != vtkMath::Inf())
  {
    cerr << "ERROR: failed to find range maximum: " << rangeArray[1] << " " << vtkMath::Inf()
         << endl;
    return EXIT_FAILURE;
  }
  info->GetComponentFiniteRange(0, rangeArray);
  if (!vtkMathUtilities::FuzzyCompare(rangeArray[0], 0.0))
  {
    cerr << "ERROR: failed to find finite range minimum: " << rangeArray[0] << " " << 0.0 << endl;
    return EXIT_FAILURE;
  }
  if (!vtkMathUtilities::FuzzyCompare(rangeArray[1], 100.0))
  {
    cerr << "ERROR: failed to find finite range maximum: " << rangeArray[1] << " " << 100.0 << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
