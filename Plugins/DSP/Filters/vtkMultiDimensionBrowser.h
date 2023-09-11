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
 *
 * In a distributed context, it can use a `GlobalIds` array, which is supposed to be continuous
 * between 0 and NumberOfElements.
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
   * Default is 0.
   */
  vtkSetMacro(Index, int);
  vtkGetMacro(Index, int);
  ///@}

  ///@{
  /**
   * Set/Get the use of GlobalIds to select index in hidden dimension.
   * Default is off.
   */
  vtkSetMacro(UseGlobalIds, bool);
  vtkGetMacro(UseGlobalIds, bool);
  vtkBooleanMacro(UseGlobalIds, bool);
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

private:
  vtkMultiDimensionBrowser(const vtkMultiDimensionBrowser&) = delete;
  void operator=(const vtkMultiDimensionBrowser&) = delete;

  /**
   * Get the maximum value available for index.
   */
  ///@{
  /// Compute the maximum significant value for Index.
  vtkIdType ComputeIndexMax();
  /// Compute size of hidden dimension. Use each input array and keep the min for robusteness.
  vtkIdType ComputeLocalSize();
  /// Find max value of the GlobalId array.
  vtkIdType ComputeLocalGlobalIdMax();
  ///@}

  /**
   * Index range check.
   */
  ///@{
  /**
   * Compute the IndexRange.
   * This is [0, ComputeIndexMax()].
   */
  void UpdateGlobalIndexRange();
  /**
   * Return true if Index is in IndexRange. Be sure range is up to date before calling this.
   * @sa UpdateIndexRange, ComputeIndexMax
   */
  bool IsIndexInRange();
  ///@}

  /**
   * Update LocalIndex.
   * Return true if LocalIndex is valid.
   */
  ///@{
  /// Update LocalIndex from Index.
  bool UpdateLocalIndex();
  /// Maps Index to a local index, using an offset per rank. Useful in distributed environment.
  bool MapToLocalIndex();
  /// Maps Index to its position in a GlobalIds local array.
  bool MapToLocalGlobalId();
  ///@}

  int Index = 0;
  int LocalIndex = 0;
  int IndexRange[2] = { 0, 0 };
  bool UseGlobalIds = false;
};

#endif
