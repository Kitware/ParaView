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

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkPVContourFilter* New();

  virtual int ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

protected:
  vtkPVContourFilter();
  ~vtkPVContourFilter();

  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  virtual int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  /**
   * Class superclass request data. Also handles iterating over
   * vtkHierarchicalBoxDataSet.
   */
  int ContourUsingSuperclass(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

private:
  vtkPVContourFilter(const vtkPVContourFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVContourFilter&) VTK_DELETE_FUNCTION;
};

#endif // vtkPVContourFilter_h
