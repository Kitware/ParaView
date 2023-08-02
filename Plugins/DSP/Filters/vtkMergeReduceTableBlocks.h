// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkMergeReduceTableBlocks
 *
 * This filter performs reduction operations such as the mean or the sum over
 * columns across all blocks of a multiblock of vtkTables.
 * Input arrays can also be copied to the output.
 */

#ifndef vtkMergeReduceTableBlocks_h
#define vtkMergeReduceTableBlocks_h

#include "vtkDSPFiltersPluginModule.h"
#include "vtkNew.h" // for vtkNew
#include "vtkTableAlgorithm.h"

class vtkDataArraySelection;

class VTKDSPFILTERSPLUGIN_EXPORT vtkMergeReduceTableBlocks : public vtkTableAlgorithm
{
public:
  static vtkMergeReduceTableBlocks* New();
  vtkTypeMacro(vtkMergeReduceTableBlocks, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the current selection of columns to reduce.
   */
  vtkGetNewMacro(ColumnToReduceSelection, vtkDataArraySelection);

  /**
   * Get the current selection of columns to copy from input to the output.
   * Columns are copied from the first block.
   */
  vtkGetNewMacro(ColumnToCopySelection, vtkDataArraySelection);

  /**
   * Get the current selection of operations for reduction.
   * Available operations include mean, sum, min and max.
   */
  vtkGetNewMacro(OperationSelection, vtkDataArraySelection);

protected:
  vtkMergeReduceTableBlocks();
  ~vtkMergeReduceTableBlocks() override = default;

  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMergeReduceTableBlocks(const vtkMergeReduceTableBlocks&) = delete;
  void operator=(const vtkMergeReduceTableBlocks&) = delete;

  vtkNew<vtkDataArraySelection> ColumnToReduceSelection;
  vtkNew<vtkDataArraySelection> ColumnToCopySelection;
  vtkNew<vtkDataArraySelection> OperationSelection;
};

#endif // vtkMergeReduceTableBlocks_h
