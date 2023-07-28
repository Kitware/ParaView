// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDSPDataModelTestingUtilities.h"
#include "vtkMultiDimensionalImplicitBackend.h"
#include "vtkNew.h"

namespace
{
/**
 * Generate a list of "nbOfArrays" vtkIntArrays with "nbOfTuples" tuples and "nbOfComponents"
 * components. Values are incremented along the 3 dimensions: value at (arrayIdx, tupleIdx,
 * componentIdx) equals to nbOfTuples * arrayIdx + nbOfComponents * tupleIdx + componentIdx.
 */
std::vector<vtkSmartPointer<vtkAOSDataArrayTemplate<int>>> generateIntArrayVector(
  int nbOfArrays, int nbOfTuples, int nbOfComp)
{
  std::vector<vtkSmartPointer<vtkAOSDataArrayTemplate<int>>> arrays;
  int value = 0;
  for (int arrayIdx = 0; arrayIdx < nbOfArrays; arrayIdx++)
  {
    vtkNew<vtkAOSDataArrayTemplate<int>> array;
    array->SetNumberOfComponents(nbOfComp);
    array->SetNumberOfTuples(nbOfTuples);
    for (int tupleIdx = 0; tupleIdx < nbOfTuples; tupleIdx++)
    {
      for (int compIdx = 0; compIdx < nbOfComp; compIdx++)
      {
        array->SetValue(nbOfComp * tupleIdx + compIdx, value++);
      }
    }
    arrays.emplace_back(array);
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

  // Construct vector of vtkIntArrays
  auto arrays = ::generateIntArrayVector(nbOfArrays, nbOfTuples, nbOfComp);

  // Construct backend
  vtkMultiDimensionalImplicitBackend<int> backend(arrays);

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
