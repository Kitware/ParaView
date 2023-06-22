/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultiDimensionalArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAOSDataArrayTemplate.h"
#include "vtkDSPDataModelTestingUtilities.h"
#include "vtkDataArrayRange.h"
#include "vtkMultiDimensionalArray.h"
#include "vtkMultiDimensionalImplicitBackend.h"
#include "vtkNew.h"

#include <memory>

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
int TestMultiDimensionalArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  constexpr vtkIdType nbOfArrays = 3;
  constexpr vtkIdType nbOfTuples = 3;
  constexpr int nbOfComp = 3;

  // Construct vector of vtkIntArrays
  auto arrays = ::generateIntArrayVector(nbOfArrays, nbOfTuples, nbOfComp);

  // Construct multi-dimensional array
  vtkNew<vtkMultiDimensionalArray<int>> mdArray;
  mdArray->ConstructBackend(arrays);

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

  return EXIT_SUCCESS;
};
