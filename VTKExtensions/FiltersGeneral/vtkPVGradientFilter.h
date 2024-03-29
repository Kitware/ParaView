// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVGradientFilter
 * @brief Filter to unify gradient implementations for different types.
 *
 * This is a subclass of vtkGradientFilter that allows to apply gradient filters
 * to different structure types through a unique filter. In practice, vtkGradientFilter
 * can be applied to any vtkDataSet. For unstructured grids, the gradient is computed
 * using the cell derivatives, while for structured grids, central differencing is
 * used, except on the boundaries of the dataset where forward and backward differencing
 * is used for the boundary elements.
 * For vtkImageData, it is possible to use the vtkImageGradient implementation.
 * Note that vtkImageGradient always uses central differencing by duplicating the
 * boundary values, which has a smoothing effect. This can be done by setting
 * the boundary handling method to 'SMOOTHED'.
 */

#ifndef vtkPVGradientFilter_h
#define vtkPVGradientFilter_h

#include "vtkGradientFilter.h"
#include "vtkHyperTreeGridGradient.h"               // for the HTG::ComputeMode enum
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVGradientFilter : public vtkGradientFilter
{
public:
  static vtkPVGradientFilter* New();
  vtkTypeMacro(vtkPVGradientFilter, vtkGradientFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Possible values for gradient computation at boundaries for vtkImageData:
   * - SMOOTHED - Boundary values are duplicated to apply central differencing on the
   * boundary elements.
   * - NON_SMOOTHED - Forward or backward differencing is used on the boundary
   * elements by using their only neighbor.
   */
  enum BoundaryMethod
  {
    SMOOTHED = 0,
    NON_SMOOTHED,
  };

  ///@{
  /**
   * Get/Set the number of dimensions used to compute the gradient when using SMOOTHED.
   * In two dimensions, the X and Y dimensions are used. The default is set to 3.
   * Used in vtkImageGradient for the vtkImageData type.
   * The default is set to 3.
   */
  vtkSetClampMacro(Dimensionality, int, 2, 3);
  vtkGetMacro(Dimensionality, int);
  ///@}

  ///@{
  /**
   * Get/Set the method used to compute the gradient on the boundaries of a vtkImageData.
   * SMOOTHED corresponds to the vtkImageGradient implementation with HandleBoundaries
   * on, while NON_SMOOTHED corresponds to the vtkGradientFilter implementation.
   * The default is set to NON_SMOOTHED.
   */
  vtkSetClampMacro(BoundaryMethod, int, SMOOTHED, NON_SMOOTHED);
  vtkGetMacro(BoundaryMethod, int);
  ///@}

  ///@{
  /**
   * Get/Set the mode used to compute the gradient on a vtkHyperTreeGrid.
   * Reminder: in the context of HyperTreeGrids, possible values for gradient computation are:
   * - UNLIMITED - Neighborhood is virtually refined so coarser cells have limited
   * impact on finer ones.
   * - UNSTRUCTURED - Coarser cells are processed as is and may have a greater
   * weight than finer cells.
   * The default is set to UNLIMITED.
   */
  vtkSetClampMacro(
    HTGMode, int, vtkHyperTreeGridGradient::UNLIMITED, vtkHyperTreeGridGradient::UNSTRUCTURED);
  vtkGetMacro(HTGMode, int);
  ///@}

  ///@{
  /**
   * Get/Set the extensiveness mode used to compute the gradient on a vtkHyperTreeGrid.
   * This parameter only work in UNLIMITED mode.
   * HTGExtensiveComputation is meant to reduce the influence of an attribute
   * as the corresponding cell is virtually subdivided.
   * The default is set to false.
   */
  vtkSetMacro(HTGExtensiveComputation, bool);
  vtkGetMacro(HTGExtensiveComputation, bool);
  vtkBooleanMacro(HTGExtensiveComputation, bool);
  ///@}

protected:
  vtkPVGradientFilter() = default;
  ~vtkPVGradientFilter() override = default;

  int Dimensionality = 3;
  int BoundaryMethod = NON_SMOOTHED;

  int HTGMode = vtkHyperTreeGridGradient::UNLIMITED;

  bool HTGExtensiveComputation = false;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPVGradientFilter(const vtkPVGradientFilter&) = delete;
  void operator=(const vtkPVGradientFilter&) = delete;
};

#endif
