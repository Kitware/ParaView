// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSortedTableStreamer
 * @brief   return a sorted subset of the original table
 *
 *
 * This filter is used quickly get a sorted subset of a given vtkTable.
 * By sorted we mean a subset build from a global sort even if some optimisation
 * allow us to skip a global table sorting.
 */

#ifndef vtkSortedTableStreamer_h
#define vtkSortedTableStreamer_h

#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro
#include "vtkSmartPointer.h"                          // for vtkSmartPointer
#include "vtkTableAlgorithm.h"
#include <utility> // for std::pair

class vtkDataArray;
class vtkIdTypeArray;
class vtkMultiProcessController;
class vtkPartitionedDataSet;
class vtkStringArray;
class vtkTable;
class vtkUnsignedIntArray;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkSortedTableStreamer : public vtkTableAlgorithm
{
private:
  class InternalsBase;
  template <class T>
  class Internals;
  InternalsBase* Internal;

public:
  static void PrintInfo(vtkTable* input);
  ///@{
  /**
   * Test the internal structure and make sure that they behave as expected.
   * Return true if everything is OK, false otherwise.
   */
  static bool TestInternalClasses();
  static vtkSortedTableStreamer* New();
  vtkTypeMacro(vtkSortedTableStreamer, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Only one input which is the table to sort
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  ///@{
  /**
   * Block index used to select a data range
   */
  vtkGetMacro(Block, vtkIdType);
  vtkSetMacro(Block, vtkIdType);
  ///@}

  ///@{
  /**
   * Set the block size. Default value is 1024
   */
  vtkGetMacro(BlockSize, vtkIdType);
  vtkSetMacro(BlockSize, vtkIdType);
  ///@}

  ///@{
  /**
   * Choose on which column the sort operation should occur
   */
  vtkGetMacro(SelectedComponent, int);
  vtkSetMacro(SelectedComponent, int);
  ///@}

  ///@{
  /**
   * Allow user to enable/disable field data visibility.
   * Default to false.
   */
  vtkSetMacro(ShowFieldData, bool);
  vtkGetMacro(ShowFieldData, bool);
  vtkBooleanMacro(ShowFieldData, bool);
  ///@}

  ///@{
  /**
   * Get/Set the MPI controller used for gathering.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  /**
   * Choose on which column the sort operation should occur
   */
  const char* GetColumnNameToSort();

  // Update column to sort has well as invalidating the pre-processing
  void SetColumnNameToSort(const char* columnName);

  // Choose if the sorting order should be inverted or not
  void SetInvertOrder(int newValue);
  vtkGetMacro(InvertOrder, int);

protected:
  vtkSortedTableStreamer();
  ~vtkSortedTableStreamer() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void CreateInternalIfNeeded(vtkTable* input, vtkDataArray* data);
  vtkDataArray* GetDataArrayToProcess(vtkTable* input);

  ///@{
  /**
   * Choose on which column the sort operation should occur
   */
  vtkGetStringMacro(ColumnToSort);
  vtkSetStringMacro(ColumnToSort);
  ///@}

  vtkIdType Block;
  vtkIdType BlockSize;
  vtkMultiProcessController* Controller;

  char* ColumnToSort;
  bool ShowFieldData = false;
  int SelectedComponent;
  int InvertOrder;

private:
  vtkSortedTableStreamer(const vtkSortedTableStreamer&) = delete;
  void operator=(const vtkSortedTableStreamer&) = delete;

  vtkSmartPointer<vtkTable> MergeBlocks(vtkPartitionedDataSet* cd);

  /**
   * Add field data columns defined by block to the output table.
   */
  void PopulateFieldDataArrays(vtkPartitionedDataSet* ptd, vtkSmartPointer<vtkTable> outTable);

  vtkSmartPointer<vtkUnsignedIntArray> GenerateCompositeIndexArray(
    vtkPartitionedDataSet* cd, vtkIdType maxSize);
  vtkSmartPointer<vtkStringArray> GenerateBlockNameArray(vtkPartitionedDataSet* cd);
  vtkSmartPointer<vtkIdTypeArray> GenerateBlockIndicesArray(
    vtkPartitionedDataSet* cd, vtkStringArray* blockNames, vtkIdType maxSize);
};

#endif
