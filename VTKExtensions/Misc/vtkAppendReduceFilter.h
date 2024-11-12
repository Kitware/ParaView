// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkAppendReduceFilter
 * @brief Reduction Filter that uses vtkAppendDataSets as PostGatherHelper
 *
 * This filter specializes vtkReductionFilter to use vtkAppendDataSets
 * as PostGatherHelper and expose its MergePoints and Tolerance properties to Paraview
 */

#ifndef vtkAppendReduceFilter_h
#define vtkAppendReduceFilter_h

#include "vtkPVVTKExtensionsMiscModule.h" // needed for export macro
#include "vtkReductionFilter.h"

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkAppendReduceFilter : public vtkReductionFilter
{

public:
  static vtkAppendReduceFilter* New();
  vtkTypeMacro(vtkAppendReduceFilter, vtkReductionFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set if the filter should merge coincidental points
   * Defaults to Off
   *
   * @sa vtkAppendFilter::MergePoints
   */
  vtkSetMacro(MergePoints, bool);
  vtkGetMacro(MergePoints, bool);
  ///@}

  ///@{
  /**
   * Get/Set the tolerance to use to find coincident points when `MergePoints`
   * is `true`. Default is 0.0.
   *
   * This is simply passed on to the internal vtkLocator used to merge points.
   * @sa `vtkLocator::SetTolerance`.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

protected:
  vtkAppendReduceFilter();
  ~vtkAppendReduceFilter() override;

  void Reduce(vtkDataObject* input, vtkDataObject* output) override;

private:
  bool MergePoints = false;
  double Tolerance = 0.0;
};

#endif
