// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class    vtkDSPIterator
 * @brief    Abstract class defining a generic iterator for spatio temporal datasets.
 *
 * Factory generating iterator instances able to iterate over vtkTables with
 * vtkMultiDimensionalArrays or over vtkMultiBlockDataSets.
 * Use the GetInstance method to instantiate the appropriate type of iterator depending
 * on the type of the input.
 *
 * @sa
 * vtkDSPMultiBlockIterator vtkDSPTableIterator
 */

#ifndef vtkDSPIterator_h
#define vtkDSPIterator_h

#include "vtkDataObject.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

class vtkTable;

class vtkDSPIterator : public vtkDataObject
{
public:
  vtkTypeMacro(vtkDSPIterator, vtkDataObject);

  /**
   * Create a new DSPIterator subclass instance.
   *
   * @param object vtkDataObject over which the iterator must iterate
   *
   * This method will create an instance of vtkDSPMultiBlockIterator or
   * vtkDSPTableIterator depending on the type of the object parameter.
   * The object parameter must be a vtkMultiBlockDataSet or a vtkTable
   * with vtkMultiDimensionalArray.
   */
  static vtkSmartPointer<vtkDSPIterator> GetInstance(vtkDataObject* object);

  /**
   * Move the iterator to the first item.
   *
   * Implemented by subclasses.
   */
  virtual void GoToFirstItem() = 0;

  /**
   * Move the iterator to the next item.
   *
   * Implemented by subclasses.
   */
  virtual void GoToNextItem() = 0;

  /**
   * Return true if the iterator reached the end.
   *
   * @return True if the iterator reached the end.
   *
   * Implemented by subclasses.
   */
  virtual bool IsDoneWithTraversal() = 0;

  /**
   * Get the current item as a table.
   *
   * @return The vtkTable associated to the current item.
   *
   * Implemented by subclasses.
   */
  virtual vtkTable* GetCurrentTable() = 0;

  /**
   * Get the total number of iterations.
   *
   * @return The number of iterations needed to traverse the underlying item.
   *
   * Implemented by subclasses.
   */
  virtual vtkIdType GetNumberOfIterations() = 0;

protected:
  vtkDSPIterator() = default;
  ~vtkDSPIterator() override = default;

private:
  vtkDSPIterator(const vtkDSPIterator&) = delete;
  void operator=(const vtkDSPIterator&) = delete;
};

#endif // vtkDSPIterator_h
