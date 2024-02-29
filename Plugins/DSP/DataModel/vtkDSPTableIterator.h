// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class    vtkDSPTableIterator
 * @brief    Iterator for vtkTable containing vtkMultiDimensionalArrays.
 *
 * Iterate over implicit/hidden indices of vtkMultiDimensionalArrays
 * contained in a vtkTable.
 *
 * @sa
 * vtkDSPIterator vtkDSPMultiBlockIterator
 */

#ifndef vtkDSPTableIterator_h
#define vtkDSPTableIterator_h

#include "vtkDSPIterator.h"

#include <memory> // for std::unique_ptr

class vtkTable;

class vtkDSPTableIterator : public vtkDSPIterator
{
public:
  vtkTypeMacro(vtkDSPTableIterator, vtkDSPIterator);
  static vtkDSPTableIterator* New(vtkTable* table);

  /**
   * Move the iterator to the first item.
   */
  void GoToFirstItem() override;

  /**
   * Move the iterator to the next item.
   */
  void GoToNextItem() override;

  /**
   * Return true if the iterator reached the end.
   */
  bool IsDoneWithTraversal() override;

  /**
   * Get the current item as a table.
   */
  vtkTable* GetCurrentTable() override;

  /**
   * Get the number of iterations needed to traverse the current item.
   * This is equivalent to the number of hidden indices for the multidimensional arrays.
   */
  vtkIdType GetNumberOfIterations() override;

protected:
  vtkDSPTableIterator();
  ~vtkDSPTableIterator() override = default;

private:
  vtkDSPTableIterator(const vtkDSPTableIterator&) = delete;
  void operator=(const vtkDSPTableIterator&) = delete;

  static vtkDSPTableIterator* New();

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif // vtkDSPTableIterator_h
