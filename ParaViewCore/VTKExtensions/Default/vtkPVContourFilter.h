/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * For certain inputs, this filter delegates operation to vtkContour3DLinearGrid.
 * The input must meet the following conditions for this filter to be used:
 *
 * - all cells in the input are linear (i.e., not higher order)
 * - the contour array is one of the types supported by the vtkContour3DLinearGrid
 * - the ComputeScalars option is off
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
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVContourFilter : public vtkContourFilter
{
public:
  vtkTypeMacro(vtkPVContourFilter, vtkContourFilter);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVContourFilter* New();

  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkPVContourFilter();
  ~vtkPVContourFilter() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

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
};

#endif // vtkPVContourFilter_h
