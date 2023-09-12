// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAOSDataArrayTemplate.h"
#include "vtkDSPDataModelTestingUtilities.h"
#include "vtkDataArrayRange.h"
#include "vtkMultiDimensionalArray.h"
#include "vtkMultiDimensionalImplicitBackend.h"
#include "vtkNew.h"

#include <memory>
#include <numeric>

namespace
{
using DataContainerInt = typename vtkMultiDimensionalImplicitBackend<int>::DataContainerT;

/**
 * Generate a list of "nbOfArrays" std::vector<int>. Values are incremented along the
 * 3 dimensions: value at (arrayIdx, valueIdx) equals to
 * nbOfTuples * nbOfComp * arrayIdx + valueIdx.
 */
DataContainerInt generateIntArrayVector(int nbOfArrays, int nbOfTuples, int nbOfComp)
{
  DataContainerInt arrays(nbOfArrays);
  const int nbOfValues = nbOfTuples * nbOfComp;
  for (int arrayIdx = 0; arrayIdx < nbOfArrays; ++arrayIdx)
  {
    std::vector<int>& array = arrays[arrayIdx];
    array.resize(nbOfValues);
    std::iota(array.begin(), array.end(), arrayIdx * nbOfValues);
  }
  return arrays;
}
}

//-----------------------------------------------------------------------------
int TestMultiDimensionalArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  constexpr vtkIdType nbOfArrays = 3;
  constexpr vtkIdType nbOfTuples = 3;
  constexpr int nbOfComp = 3;

  // Construct multi-dimensional array
  vtkNew<vtkMultiDimensionalArray<int>> mdArray;
  mdArray->ConstructBackend(std::make_shared<::DataContainerInt>(
                              ::generateIntArrayVector(nbOfArrays, nbOfTuples, nbOfComp)),
    nbOfTuples, nbOfComp);

  // Check array sizes
  if (!vtkDSPDataModelTestingUtilities::testValue(
        mdArray->GetNumberOfComponents(), nbOfComp, "GetNumberOfComponents") ||
    !vtkDSPDataModelTestingUtilities::testValue(
      mdArray->GetNumberOfTuples(), nbOfTuples, "GetNumberOfTuples") ||
    !vtkDSPDataModelTestingUtilities::testValue(
      mdArray->GetNumberOfArrays(), nbOfArrays, "GetNumberOfArrays"))
  {
    return EXIT_FAILURE;
  }

  // Check array values
  int value = 0;
  for (vtkIdType arrayIdx = 0; arrayIdx < nbOfArrays; arrayIdx++)
  {
    mdArray->SetIndex(arrayIdx);
    for (vtkIdType tupleIdx = 0; tupleIdx < nbOfTuples; tupleIdx++)
    {
      int tuple[nbOfComp] = { 0 };
      mdArray->GetTypedTuple(tupleIdx, tuple);
      for (int compIdx = 0; compIdx < nbOfComp; compIdx++)
      {
        if (!vtkDSPDataModelTestingUtilities::testValue(
              mdArray->GetValue(nbOfComp * tupleIdx + compIdx), value, arrayIdx, tupleIdx, compIdx,
              "GetValue") ||
          !vtkDSPDataModelTestingUtilities::testValue(mdArray->GetTypedComponent(tupleIdx, compIdx),
            value, arrayIdx, tupleIdx, compIdx, "GetTypedComponent") ||
          !vtkDSPDataModelTestingUtilities::testValue(
            tuple[compIdx], value, arrayIdx, tupleIdx, compIdx, "GetTypedTuple"))
        {
          return EXIT_FAILURE;
        }
        value++;
      }
    }
  }

  // Check with range iterator over values
  value = 0;
  for (vtkIdType arrayIdx = 0; arrayIdx < nbOfArrays; arrayIdx++)
  {
    mdArray->SetIndex(arrayIdx);
    for (const auto val : vtk::DataArrayValueRange<3>(mdArray))
    {
      if (!vtkDSPDataModelTestingUtilities::testValue(
            val, value, arrayIdx, value / nbOfComp, value % nbOfComp, ""))
      {
        return EXIT_FAILURE;
      }
      value++;
    }
  }

  // Check with range iterator over tuples
  value = 0;
  for (vtkIdType arrayIdx = 0; arrayIdx < nbOfArrays; arrayIdx++)
  {
    mdArray->SetIndex(arrayIdx);
    for (const auto tuple : vtk::DataArrayTupleRange<3>(mdArray))
    {
      for (const auto comp : tuple)
      {
        if (!vtkDSPDataModelTestingUtilities::testValue(
              comp, value, arrayIdx, value / nbOfComp, value % nbOfComp, ""))
        {
          return EXIT_FAILURE;
        }
        value++;
      }
    }
  }

  // Check new instance
  vtkSmartPointer<vtkDataArray> mdNewInstance;
  mdNewInstance.TakeReference(mdArray->NewInstance());
  if (!vtkAOSDataArrayTemplate<int>::SafeDownCast(mdNewInstance))
  {
    std::cerr << "Multi-dimensional array failed to provide correct AOS array on NewInstance call."
              << std::endl;
    return EXIT_FAILURE;
  }

  // Check memory size
  vtkNew<vtkMultiDimensionalArray<int>> mdArrayLarge;
  mdArrayLarge->ConstructBackend(
    std::make_shared<::DataContainerInt>(::generateIntArrayVector(1024, 50, 4)), 50, 4);
  vtkDataArray* mdDataArray = mdArrayLarge;

  if (mdArrayLarge->GetActualMemorySize() != 50 * 4 * sizeof(int) ||
    mdDataArray->GetActualMemorySize() != mdArrayLarge->GetActualMemorySize())
  {
    std::cerr << "Large multi-dimensional array failed to return correct size measurement, "
              << mdArrayLarge->GetActualMemorySize() << "KiB instead of expected "
              << 50 * 4 * sizeof(int) << "KiB." << std::endl;
    return EXIT_FAILURE;
  }

  if (mdArray->GetActualMemorySize() != 1)
  {
    std::cerr << "Multi-dimensional array failed to return correct size measurement, "
              << mdArray->GetActualMemorySize() << "KiB instead of expected 1KiB." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
