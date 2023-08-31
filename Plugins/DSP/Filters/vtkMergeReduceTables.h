// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkMergeReduceTables
 *
 * This filter performs reduction operations such as the mean or the sum over
 * columns across vtkTables. Each table typically corresponds to a point/cell.
 * These tables can be obtained after applying a vtkTemporalMultiplexing or
 * vtkPExtractDataArraysOverTime filter.
 * Input arrays can also be copied to the output.
 *
 * @warning Make sure that ghost points are marked as such when using the filter
 * in distributed mode to prevent duplicates. This can be done by applying a
 * vtkGhostCellsGenerator on the geometry prior to transforming it into tables
 * in the previous steps.
 */

#ifndef vtkMergeReduceTables_h
#define vtkMergeReduceTables_h

#include "vtkDSPFiltersPluginModule.h"
#include "vtkDataArrayRange.h"
#include "vtkNew.h" // for vtkNew
#include "vtkTableAlgorithm.h"

class vtkDataArraySelection;

class VTKDSPFILTERSPLUGIN_EXPORT vtkMergeReduceTables : public vtkTableAlgorithm
{
public:
  static vtkMergeReduceTables* New();
  vtkTypeMacro(vtkMergeReduceTables, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using RangeType = typename vtk::detail::ValueRange<vtkDataArray, vtk::detail::DynamicTupleSize>;

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
  vtkMergeReduceTables();
  ~vtkMergeReduceTables() override = default;

  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMergeReduceTables(const vtkMergeReduceTables&) = delete;
  void operator=(const vtkMergeReduceTables&) = delete;

  /**
   * Compute the sum of the given arrays element by element.
   */
  void ComputeSum(RangeType srcRange, RangeType dstRange) const;

  /**
   * Compute the minimum of the given arrays element by element.
   */
  void ComputeMin(RangeType srcRange, RangeType dstRange) const;

  /**
   * Compute the maximum of the given arrays element by element.
   */
  void ComputeMax(RangeType srcRange, RangeType dstRange) const;

  vtkNew<vtkDataArraySelection> ColumnToReduceSelection;
  vtkNew<vtkDataArraySelection> ColumnToCopySelection;
  vtkNew<vtkDataArraySelection> OperationSelection;
};

#endif // vtkMergeReduceTables_h
