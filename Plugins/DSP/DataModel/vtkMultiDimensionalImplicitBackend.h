// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMultiDimensionalImplicitBackend_h
#define vtkMultiDimensionalImplicitBackend_h

#include "vtkAOSDataArrayTemplate.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include <algorithm> // For std::swap
#include <vector>    // For std::vector

/**
 * @class vtkMultiDimensionalImplicitBackend
 * @brief Backend for multi-dimensional implicit arrays.
 *
 * vtkMultiDimensionalImplicitBackend is a utility class serving as a backend for
 * vtkMultiDimensionalArray. Please refer to this class for more informations.
 *
 * @sa vtkMultiDimensionalArray vtkImplicitArray
 */

VTK_ABI_NAMESPACE_BEGIN
template <typename ValueType>
class vtkMultiDimensionalImplicitBackend final
{
public:
  /**
   * Constructor for vtkMultiDimensionalImplicitBackend.
   * It takes a list of vtkAOSDataArrayTemplate<ValueType> as parameter.
   * Each array should have the same number of tuples and components.
   * @warning The input vector of arrays will not be preserved upon construction.
   */
  vtkMultiDimensionalImplicitBackend(
    std::vector<vtkSmartPointer<vtkAOSDataArrayTemplate<ValueType>>> arrays)
  {
    if (arrays.empty())
    {
      return;
    }

    int nbOfComp = arrays[0]->GetNumberOfComponents();
    vtkIdType nbOfTuples = arrays[0]->GetNumberOfTuples();

    for (auto array : arrays)
    {
      if (array->GetNumberOfComponents() != nbOfComp)
      {
        vtkErrorWithObjectMacro(nullptr, "Number of components of all the arrays are not equal");
        return;
      }
      if (array->GetNumberOfTuples() != nbOfTuples)
      {
        vtkErrorWithObjectMacro(nullptr, "Number of tuples of all the arrays are not equal");
        return;
      }
    }

    std::swap(this->Arrays, arrays);

    this->CurrentArray = this->Arrays[0];
    this->NumberOfComponents = nbOfComp;
    this->NumberOfTuples = nbOfTuples;
    this->NumberOfArrays = this->Arrays.size();
  }

  /**
   * Set the index to fix the "first" dimension of the 3D array.
   */
  void SetIndex(vtkIdType idx) { this->CurrentArray = this->Arrays[idx]; }

  /**
   * Get the number of components of stored arrays (equal for all arrays).
   */
  int GetNumberOfComponents() { return this->NumberOfComponents; }

  /**
   * Get the number of tuples of stored arrays (equal for all arrays).
   */
  vtkIdType GetNumberOfTuples() { return this->NumberOfTuples; }

  /**
   * Get the number of stored arrays.
   */
  vtkIdType GetNumberOfArrays() { return this->NumberOfArrays; }

  /**
   * The main call method for the backend.
   */
  ValueType operator()(int idx) const { return this->CurrentArray->GetValue(idx); }

  /**
   * Used to implement GetTypedTuple on vtkMultiDimensionalArray.
   */
  void mapTuple(int tupleidx, ValueType* tuple) const
  {
    return this->CurrentArray->GetTypedTuple(tupleidx, tuple);
  }

  /**
   * Used to implement GetTypedComponent on vtkMultiDimensionalArray.
   */
  ValueType mapComponent(vtkIdType tupleIdx, int compIdx) const
  {
    return this->CurrentArray->GetTypedComponent(tupleIdx, compIdx);
  }

private:
  std::vector<vtkSmartPointer<vtkAOSDataArrayTemplate<ValueType>>> Arrays;
  vtkAOSDataArrayTemplate<ValueType>* CurrentArray = nullptr;
  int NumberOfComponents = 1;
  vtkIdType NumberOfTuples = 0;
  vtkIdType NumberOfArrays = 0;
};
VTK_ABI_NAMESPACE_END

#endif // vtkMultiDimensionalImplicitBackend_h
