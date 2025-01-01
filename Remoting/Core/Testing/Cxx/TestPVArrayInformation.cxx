// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkSmartPointer.h"

#include <cstdlib>

static vtkSmartPointer<vtkFloatArray> GetPolyData()
{
  vtkIdType numPts = 101;
  vtkSmartPointer<vtkFloatArray> array = vtkSmartPointer<vtkFloatArray>::New();
  array->SetName("array");

  array->SetNumberOfTuples(numPts);
  for (vtkIdType cc = 0; cc < numPts; ++cc)
  {
    array->SetTypedComponent(cc, 0, static_cast<float>(cc));
  }
  return array;
}

static bool AreArrayInformationsEqual(vtkPVArrayInformation* lhs, vtkPVArrayInformation* rhs)
{
  bool equal = true;
  equal &= lhs->GetIsPartial() == rhs->GetIsPartial();
  equal &= lhs->GetDataTypeAsString() == rhs->GetDataTypeAsString();
  equal &= lhs->GetNumberOfComponents() == rhs->GetNumberOfComponents();
  equal &= lhs->GetNumberOfTuples() == rhs->GetNumberOfTuples();
  equal &= strcmp(lhs->GetName(), rhs->GetName()) == 0;

  for (int compIdx = 0; compIdx < lhs->GetNumberOfComponents(); ++compIdx)
  {
    equal &= strcmp(lhs->GetComponentName(compIdx), rhs->GetComponentName(compIdx)) == 0;

    std::array<double, 2> lhRange, rhRange;
    lhs->GetComponentRange(compIdx, lhRange.data());
    rhs->GetComponentRange(compIdx, rhRange.data());
    equal &= lhRange[0] == rhRange[0];
    equal &= lhRange[1] == rhRange[1];

    std::array<double, 2> lhFiniteRange, rhFiniteRange;
    lhs->GetComponentFiniteRange(compIdx, lhFiniteRange.data());
    rhs->GetComponentFiniteRange(compIdx, rhFiniteRange.data());
    equal &= lhFiniteRange[0] == rhFiniteRange[0];
    equal &= lhFiniteRange[1] == rhFiniteRange[1];
  }

  return equal;
}

extern int TestPVArrayInformation(int, char*[])
{

  vtkSmartPointer<vtkFloatArray> array = GetPolyData();
  vtkNew<vtkFieldData> fd;
  int fdArrayIdx = fd->AddArray(array);

  vtkNew<vtkPVArrayInformation> info;
  info->CopyFromArray(fd, fdArrayIdx);

  // Verify minimum and maximum
  auto range = info->GetComponentRange(0);
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
  info->CopyFromArray(fd, fdArrayIdx);
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

  // Compare CopyFromArray overloads
  vtkNew<vtkPVArrayInformation> infoCopiedFromArray;
  infoCopiedFromArray->CopyFromArray(array);

  if (!AreArrayInformationsEqual(info, infoCopiedFromArray))
  {
    vtkLogF(ERROR,
      "Copying information from array as simple array or as field data did not yield the same "
      "result");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
