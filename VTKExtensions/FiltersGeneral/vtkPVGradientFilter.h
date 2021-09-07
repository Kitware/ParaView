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
 * can be applied to any vtkDataSet using central differencing except on the boundaries of the
 * dataset. However, for vtkImageData, the class vtkImageGradient is used by default as a faster
 * implementation for that type. Note that vtkImageGradient always uses central differencing. By
 * setting the boundary handling method to forward/backward differencing, the vtkGradientFilter
 * class can also be used on vtkImageData.
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
   * - CENTRAL_DIFFERENCING - Boundary values are duplicated to apply central differencing on the
   * boundary elements.
   * - FORWARD_BACKWARD_DIFFERENCING - Forward or backward differencing is used on the boundary
   * elements by using their only neighbor.
   */
  enum BoundaryMethod
  {
    CENTRAL_DIFFERENCING = 0,
    FORWARD_BACKWARD_DIFFERENCING,
  };

  ///@{
  /**
   * Get/Set the number of dimensions used to compute the gradient when using CENTRAL_DIFFERENCING.
   * In two dimensions, the X and Y dimensions are used.
   * Used in vtkImageGradient for the vtkImageData type.
   */
  vtkSetClampMacro(Dimensionality, int, 2, 3);
  vtkGetMacro(Dimensionality, int);
  ///@}

  ///@{
  /**
   * Get/Set the method used to compute the gradient on the boundaries of a vtkImageData.
   * CENTRAL_DIFFERENCING corresponds to the vtkImageGradient implementation with HandleBoundaries
   * on, while FORWARD_BACKWARD_DIFFERENCING corresponds to the vtkGradientFilter implementation.
   */
  vtkSetClampMacro(BoundaryMethod, int, CENTRAL_DIFFERENCING, FORWARD_BACKWARD_DIFFERENCING);
  vtkGetMacro(BoundaryMethod, int);
  ///@}

protected:
  vtkPVGradientFilter() = default;
  ~vtkPVGradientFilter() override = default;

  int Dimensionality = 3;
  int BoundaryMethod = CENTRAL_DIFFERENCING;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPVGradientFilter(const vtkPVGradientFilter&) = delete;
  void operator=(const vtkPVGradientFilter&) = delete;
};

#endif
