// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMultiDimensionBrowser_h
#define vtkMultiDimensionBrowser_h

#include "vtkDSPFiltersPluginModule.h" // for export macro
#include "vtkTableAlgorithm.h"

class vtkTable;

/**
 * @class   vtkMultiDimensionBrowser
 *
 * @brief This filter fixes the first dimension of the vtkMultiDimensionalArrays.
 *
 * The first dimension of a multidimensional implicit array is hidden to the generic vtkDataArray
 * API. This filter intends to give a way to manipulate it anyway.
 *
 * vtkMultiDimensionBrowser loops over all input arrays to set Index on vtkMultiDimensionalArray.
 * To avoid side effect on upstream pipeline, it creates a new instance of the multidimensional
 * array and copies its backend. In terms of data, this is a zero-copy filter. Other types of arrays
 * are simply shallow copied to output.
 *
 * vtkMultiDimensionalArrays are typically created by the vtkTemporalMultiplexing filter.
 * In that case, the first dimension correspond to point index of the multiplexing input.
 * So changing Index means browsing points of this dataset.
 */
class VTKDSPFILTERSPLUGIN_EXPORT vtkMultiDimensionBrowser : public vtkTableAlgorithm
{
public:
  static vtkMultiDimensionBrowser* New();
  vtkTypeMacro(vtkMultiDimensionBrowser, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the choosen index for the selected vtkMultiDimensionnalArray
   */
  vtkSetMacro(Index, int);
  vtkGetMacro(Index, int);
  ///@}

  /**
   * Return the index valid range.
   */
  vtkGetVector2Macro(IndexRange, int);

protected:
  vtkMultiDimensionBrowser() = default;
  ~vtkMultiDimensionBrowser() override = default;

  /**
   * Shallow copy input into output and update Index of multidimensional array.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Update IndexRange, as it depends on input data.
   */
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Create output array based on sourceArray.
   *
   * If sourceArray is a multidimensional array, ShallowCopy it
   * and set Index on it. Copy is needed to avoid Index propagation upstream.
   *
   * Simply ShallowCopy other arrays.
   */
  bool CreateOutputArray(vtkDataArray* sourceArray, vtkTable* output);

  /**
   * Index range check.
   */
  ///@{
  /**
   * Get the maximum value available for index.
   * Parse all input arrays to get their maximum. Return the minimal value for safety.
   */
  int ComputeIndexMax();
  /**
   * Compute the IndexRange.
   * This is [0, ComputeIndexMax()].
   */
  void UpdateIndexRange();
  /**
   * Return true if Index is in IndexRange. Be sure range is up to date before calling this.
   * @sa UpdateIndexRange, ComputeIndexMax
   */
  bool IsIndexInRange();
  ///@}

private:
  vtkMultiDimensionBrowser(const vtkMultiDimensionBrowser&) = delete;
  void operator=(const vtkMultiDimensionBrowser&) = delete;

  int Index = 0;
  int IndexRange[2] = { 0, 0 };
};

#endif
