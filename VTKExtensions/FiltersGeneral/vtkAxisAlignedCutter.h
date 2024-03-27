// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkAxisAlignedCutter
 * @brief Cut data with an Axis-Aligned cut function
 *
 * `vtkAxisAlignedCutter` is a filter producing Axis-Aligned "slices" of the input data.
 * Among slicing filters, this one allows to preserve the input type. In other words,
 * this filter reduces the dimention of the input by 1, following an implicit cutting function.
 * For example, slicing a 3D `vtkHyperTreeGrid` will produce a 2D `vtkHyperTreeGrid`.
 * For that matter, this filter is limited to Axis-Aligned functions.
 *
 * Except for `vtkOverlapingAMR`, this filter can produce multiple slices at once.
 * In such cases, the output slices are stored in a `vtkPartitionedDataSetCollection`.
 *
 * Currently supported input types are:
 * - `vtkHyperTreeGrid`, output is a `vtkPartitionedDataSetCollection` (of `vtkHyperTreeGrid`)
 * - `vtkOverlappingAMR`, output is a `vtkOverlappingAMR`
 *
 * Currently supported cutting function is Axis-Aligned `vtkPVPlane`.
 *
 * @remark This filter do not support composite inputs.
 *
 * @sa
 * vtkCutter vtkPVCutter vtkPlaneCutter vtkPVPlaneCutter vtkPVMetaSliceDataSet
 * vtkHyperTreeGridAxisCut vtkAMRSliceFilter
 */

#ifndef vtkAxisAlignedPlaneCutter_h
#define vtkAxisAlignedPlaneCutter_h

#include "vtkAMRSliceFilter.h" // for vtkAMRSliceFilter
#include "vtkContourValues.h"  // for vtkContourValues
#include "vtkDataObjectAlgorithm.h"
#include "vtkHyperTreeGridAxisCut.h"                // for vtkHyperTreeGridAxisCut
#include "vtkImplicitFunction.h"                    // fir vtkImplicitFunction
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" // for export macro
#include "vtkSmartPointer.h"                        // for vtkSmartPointer

class vtkHyperTreeGridAxisCut;
class vtkPVPlane;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkAxisAlignedCutter : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkAxisAlignedCutter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAxisAlignedCutter* New();

  ///@{
  /**
   * Specify the implicit function to perform the cutting.
   * For now, only `Axis-Aligned` planes are supported.
   */
  vtkSetSmartPointerMacro(CutFunction, vtkImplicitFunction);
  vtkGetSmartPointerMacro(CutFunction, vtkImplicitFunction);
  ///@}

  /**
   * Get the last modified time of this filter.
   * This time also depends on the the modified
   * time of the internal CutFunction instance.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/get a slice offset value at specified index i.
   * Negative indices are clamped to 0.
   * New space is allocated if necessary.
   */
  void SetOffsetValue(int i, double value);
  double GetOffsetValue(int i);
  ///@}

  ///@{
  /**
   * Set/get the number of slice offset values.
   * Allocate new space to store them if needed.
   */
  void SetNumberOfOffsetValues(int number);
  int GetNumberOfOffsetValues();
  ///@}

  ///@{
  /**
   * Sets the level of resolution.
   * Default is 0.
   *
   * Note: Only used for cutting overlapping AMR.
   */
  vtkSetMacro(LevelOfResolution, int);
  vtkGetMacro(LevelOfResolution, int);
  ///@}

protected:
  vtkAxisAlignedCutter();
  ~vtkAxisAlignedCutter() override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation* info) override;

private:
  vtkAxisAlignedCutter(const vtkAxisAlignedCutter&) = delete;
  void operator=(const vtkAxisAlignedCutter&) = delete;

  /**
   * Cut HTG with axis-aligned plane, applying additional plane offset if needed
   */
  void CutHTGWithAAPlane(
    vtkHyperTreeGrid* input, vtkHyperTreeGrid* output, vtkPVPlane* plane, double offset);

  /**
   * Cut AMR with axis-aligned plane
   */
  void CutAMRWithAAPlane(vtkOverlappingAMR* input, vtkOverlappingAMR* output, vtkPVPlane* plane);

  vtkSmartPointer<vtkImplicitFunction> CutFunction;
  vtkNew<vtkHyperTreeGridAxisCut> HTGCutter;
  vtkNew<vtkAMRSliceFilter> AMRCutter;
  int LevelOfResolution = 0;
  vtkNew<vtkContourValues> OffsetValues;
};

#endif
