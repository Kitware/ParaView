// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMultiDimensionalArray_h
#define vtkMultiDimensionalArray_h

#include "vtkAOSDataArrayTemplate.h"            // For vtkAOSDataArrayTemplate
#include "vtkImplicitArray.h"                   // For vtkImplicitArray
#include "vtkMultiDimensionalImplicitBackend.h" // For the array backend
#include "vtkObjectFactory.h"                   // For VTK_STANDARD_NEW_BODY

/**
 * @class vtkMultiDimensionalArray
 * @brief Implicit array used to represent read-only multi-dimensional arrays.
 *
 * This class is meant to display 2D views (tuple/component, like any vtkDataArray)
 * on arrays of higher dimensions. For now, only 3D arrays are supported.
 *
 * The constructor takes a list of vtkAOSDataArrayTemplate of the same type (ValueType).
 * Each array should have the same number of tuples and components.
 *
 * Once constructed, use the SetIndex method to "move" the 2D view on the 3D array.
 * Subsequent read access on the 3D array will be done internally on the 2D array at
 * this index.
 *
 * Example of use of vtkMultiDimensionalArray:
 * @code{cpp}
 * // Construct arrays
 * std::vector<vtkSmartPointer<vtkAOSDataArrayTemplate<int>>> arrays;
 * vtkNew<vtkAOSDataArrayTemplate<int>> array1;
 * array1->SetNumberOfComponents(1);
 * array1->SetNumberOfTuples(2);
 * array1->SetValue(0, 0);
 * array1->SetValue(0, 1);
 * arrays->emplace_back(array1);
 * vtkNew<vtkAOSDataArrayTemplate<int>> array2;
 * array2->SetNumberOfComponents(1);
 * array2->SetNumberOfTuples(2);
 * array2->SetValue(0, 2);
 * array2->SetValue(1, 3);
 * arrays->emplace_back(array2);
 *
 * // Construct vtkMultiDimensionalArray
 * vtkNew<vtkMultiDimensionalArray<int>> mdArray;
 * mdArray->ConstructBackend(arrays);
 *
 * // Access to vtkMultiDimensionalArray
 * int val = mdArray->GetValue(1); // val == 1
 * mdArray->SetIndex(1);
 * val = mdArray->GetValue(1);     // val == 3
 * @endcode
 *
 * @sa vtkMultiDimensionalImplicitBackend vtkImplicitArray
 */

VTK_ABI_NAMESPACE_BEGIN
template <typename ValueType>
class vtkMultiDimensionalArray
  : public vtkImplicitArray<vtkMultiDimensionalImplicitBackend<ValueType>>
{
public:
  using SelfType = vtkMultiDimensionalArray<ValueType>;
  using BackendType = vtkMultiDimensionalImplicitBackend<ValueType>;
  using SuperType = vtkImplicitArray<BackendType>;

  static vtkMultiDimensionalArray* New()
  {
    VTK_STANDARD_NEW_BODY(vtkMultiDimensionalArray<ValueType>);
  }

  vtkImplicitArrayTypeMacro(SelfType, SuperType);

  /**
   * Setter for Backend.
   * Redefinition of vtkImplicitArray::SetBackend.
   */
  void SetBackend(std::shared_ptr<BackendType> newBackend)
  {
    this->SetNumberOfComponents(newBackend->GetNumberOfComponents());
    this->SetNumberOfTuples(newBackend->GetNumberOfTuples());
    this->Superclass::SetBackend(newBackend);
  }

  /**
   * Utility method for setting backend parameterization directly.
   * Redefinition of vtkImplicitArray::ConstructBackend.
   */
  template <typename... Params>
  void ConstructBackend(Params&&... params)
  {
    this->SetBackend(std::make_shared<BackendType>(std::forward<Params>(params)...));
  }

  /**
   * Set the index to fix the "first" dimension of the 3D array.
   */
  void SetIndex(vtkIdType idx) { this->Backend->SetIndex(idx); }

  /**
   * Get the number of arrays stored in the backend.
   */
  vtkIdType GetNumberOfArrays() { return this->Backend->GetNumberOfArrays(); }

  /**
   * Specific ShallowCopy
   *
   * We cannot call the method `ShallowCopy` since that conflicts with the virtual function of
   * the same name that cannot be templated.
   */
  template <typename OtherValue>
  void ImplicitShallowCopy(vtkMultiDimensionalArray<OtherValue>* other)
  {
    static_assert(std::is_same<OtherValue, ValueType>::value,
      "Cannot copy multidimensional array from another underlying type");
    this->SetName(other->GetName());
    auto backend = other->GetBackend();
    this->ConstructBackend(
      backend->GetData(), backend->GetNumberOfTuples(), backend->GetNumberOfComponents());
  }

protected:
  vtkMultiDimensionalArray() = default;
  ~vtkMultiDimensionalArray() override = default;

private:
  vtkMultiDimensionalArray(const vtkMultiDimensionalArray&) = delete;
  void operator=(const vtkMultiDimensionalArray&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif // vtkMultiDimensionalArray_h
