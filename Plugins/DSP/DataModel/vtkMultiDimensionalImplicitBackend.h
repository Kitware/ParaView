// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMultiDimensionalImplicitBackend_h
#define vtkMultiDimensionalImplicitBackend_h

#include "vtkAOSDataArrayTemplate.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include <memory> // For std::shared_ptr
#include <vector> // For std::vector

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
  using DataContainerT = std::vector<std::vector<ValueType>>;

  /**
   * Constructor for vtkMultiDimensionalImplicitBackend.
   * It takes a std::shared_ptr of a list of std::vector<ValueType> as parameter.
   * Each array should have the same number of values, so be equal to
   * nbOfTuples * nbOfComponents. They need to be passed as the arrays are
   * already flatten.
   */
  vtkMultiDimensionalImplicitBackend(
    std::shared_ptr<DataContainerT> arrays, vtkIdType nbOfTuples, int nbOfComponents)
  {
    if (arrays->empty())
    {
      return;
    }

    const std::size_t nbOfValues = static_cast<std::size_t>(nbOfTuples * nbOfComponents);
    for (auto array : *arrays)
    {
      if (array.size() != nbOfValues)
      {
        vtkErrorWithObjectMacro(nullptr, "Number of values of all the arrays are not equal");
        return;
      }
    }

    this->Arrays = arrays;
    this->CurrentArray = &(*this->Arrays)[0];
    this->NumberOfComponents = nbOfComponents;
    this->NumberOfTuples = nbOfTuples;
    this->NumberOfArrays = static_cast<vtkIdType>(this->Arrays->size());
  }

  /**
   * Set the index to fix the "first" dimension of the 3D array.
   * @warning No index checking is performed.
   */
  void SetIndex(vtkIdType idx) { this->CurrentArray = &(*this->Arrays)[idx]; }

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
   * @warning No index checking is performed.
   */
  ValueType operator()(vtkIdType idx) const { return (*this->CurrentArray)[idx]; }

  /**
   * Used to implement GetTypedTuple on vtkMultiDimensionalArray.
   */
  void mapTuple(vtkIdType tupleIdx, ValueType* tuple) const
  {
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
    std::copy(this->CurrentArray->cbegin() + valueIdx,
      this->CurrentArray->cbegin() + valueIdx + this->NumberOfComponents, tuple);
  }

  /**
   * Used to implement GetTypedComponent on vtkMultiDimensionalArray.
   * @warning No index checking is performed.
   */
  ValueType mapComponent(vtkIdType tupleIdx, int compIdx) const
  {
    const vtkIdType idx = tupleIdx * this->NumberOfComponents + compIdx;
    return (*this->CurrentArray)[idx];
  }

  /**
   * Used to implement GetActualMemorySize on vtkMultiDimensionalArray.
   * The function makes the assumption that all arrays have the same number of components and
   * tuples.
   */
  unsigned long getMemorySize()
  {
    unsigned long bytes = static_cast<unsigned long>(sizeof(ValueType)) *
      this->GetNumberOfArrays() * this->GetNumberOfTuples() * this->GetNumberOfComponents();
    return std::ceil(bytes / 1024.0);
  }

  /**
   * Get the shared_ptr of the data.
   * This allows multiple backend to share the same data without copy.
   */
  std::shared_ptr<DataContainerT> GetData() { return this->Arrays; }

private:
  std::shared_ptr<DataContainerT> Arrays;
  std::vector<ValueType>* CurrentArray = nullptr;
  int NumberOfComponents = 0;
  vtkIdType NumberOfTuples = 0;
  vtkIdType NumberOfArrays = 0;
};
VTK_ABI_NAMESPACE_END

#endif // vtkMultiDimensionalImplicitBackend_h
