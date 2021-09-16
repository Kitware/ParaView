/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGradientFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

protected:
  vtkPVGradientFilter() = default;
  ~vtkPVGradientFilter() override = default;

  int Dimensionality = 3;
  int BoundaryMethod = NON_SMOOTHED;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPVGradientFilter(const vtkPVGradientFilter&) = delete;
  void operator=(const vtkPVGradientFilter&) = delete;
};

#endif
