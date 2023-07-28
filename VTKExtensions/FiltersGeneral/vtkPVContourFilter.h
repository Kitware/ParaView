// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVContourFilter
 * @brief   generate isosurfaces/isolines from scalar values
 *
 * vtkPVContourFilter is an extension to vtkContourFilter. It adds the
 * ability to generate isosurfaces / isolines for AMR dataset.
 *
 * vtkPVContourFilter also handles cleaning up interpolation error in the output
 * scalars array when `ComputeScalars` is set to true. For each requested
 * contour value, the output scalar array may have an error introduced due to
 * interpolation along an edge. This filter fixes that error so that for all
 * points generated from a particular contour value, the scalar array will have
 * exactly the same value.
 *
 * @warning
 * Certain flags in vtkAMRDualContour are assumed to be ON.
 *
 * @sa
 * vtkContourFilter vtkAMRDualContour
 */

#ifndef vtkPVContourFilter_h
#define vtkPVContourFilter_h

#include "vtkContourFilter.h"
#include "vtkHyperTreeGridContour.h"                // for vtkHyperTreeGridContour
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" // needed for exports

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVContourFilter : public vtkContourFilter
{
public:
  vtkTypeMacro(vtkPVContourFilter, vtkContourFilter);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVContourFilter* New();

  ///@{
  /**
   * Get/set the contour strategy to apply in case of a HTG input.
   * By default, the strategy is vtkHyperTreeGridContour::USE_VOXELS.
   * This method is time-efficient but can lead to bad results in the 3D case, where generated dual
   * cells can be concave.
   * vtkHyperTreeGridContour::USE_DECOMPOSED_POLYHEDRA allows better results in such cases (3D HTGs
   * only). It takes advantage of the vtkPolyhedronUtilities::Decompose method to generate better
   * contours. The dowside is that this method is much slower than
   * vtkHyperTreeGridContour::USE_VOXELS.
   */
  vtkGetMacro(HTGStrategy3D, int);
  vtkSetClampMacro(HTGStrategy3D, int, vtkHyperTreeGridContour::USE_VOXELS,
    vtkHyperTreeGridContour::USE_DECOMPOSED_POLYHEDRA);
  ///@}

protected:
  vtkPVContourFilter();
  ~vtkPVContourFilter() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Class superclass request data. Also handles iterating over
   * vtkHierarchicalBoxDataSet.
   */
  int ContourUsingSuperclass(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  /**
   * When `ComputeScalars` is true, the filter computes scalars for the
   * contours. However, due to interpolation errors, there's a small difference
   * between the requested contour value and the interpolated scalar value that
   * ends up getting computed. That makes it complicated to identify contours
   * for a specific scalar value. This method cleans the output scalars array to
   * exactly match requested values.
   */
  void CleanOutputScalars(vtkDataArray* outScalars);

private:
  vtkPVContourFilter(const vtkPVContourFilter&) = delete;
  void operator=(const vtkPVContourFilter&) = delete;

  // Strategy used to represent HTG dual cells in 3D
  int HTGStrategy3D = vtkHyperTreeGridContour::USE_VOXELS;
};

#endif // vtkPVContourFilter_h
