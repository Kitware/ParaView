// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDSPDataModelTestingUtilities.h"
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
  int value = 0;
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
int TestMultiDimensionalImplicitBackend(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  constexpr vtkIdType nbOfArrays = 3;
  constexpr vtkIdType nbOfTuples = 3;
  constexpr int nbOfComp = 3;

  // Construct backend
  vtkMultiDimensionalImplicitBackend<int> backend(
    std::make_shared<::DataContainerInt>(
      ::generateIntArrayVector(nbOfArrays, nbOfTuples, nbOfComp)),
    nbOfTuples, nbOfComp);

  // Check backend
  int value = 0;
  for (vtkIdType arrayIdx = 0; arrayIdx < nbOfArrays; arrayIdx++)
  {
    backend.SetIndex(arrayIdx);
    for (vtkIdType tupleIdx = 0; tupleIdx < nbOfTuples; tupleIdx++)
    {
      int tuple[nbOfComp] = { 0 };
      backend.mapTuple(tupleIdx, tuple);
      for (int compIdx = 0; compIdx < nbOfComp; compIdx++)
      {
        if (!vtkDSPDataModelTestingUtilities::testValue(backend(nbOfComp * tupleIdx + compIdx),
              value, arrayIdx, tupleIdx, compIdx, "operator()") ||
          !vtkDSPDataModelTestingUtilities::testValue(backend.mapComponent(tupleIdx, compIdx),
            value, arrayIdx, tupleIdx, compIdx, "mapComponent") ||
          !vtkDSPDataModelTestingUtilities::testValue(
            tuple[compIdx], value, arrayIdx, tupleIdx, compIdx, "mapTuple"))
        {
          return EXIT_FAILURE;
        }
        value++;
      }
    }
  }

  return EXIT_SUCCESS;
};
