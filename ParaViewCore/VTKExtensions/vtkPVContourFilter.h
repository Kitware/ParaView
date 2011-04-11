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
// .NAME vtkPVContourFilter - generate isosurfaces/isolines from scalar values
// .SECTION Description
// vtkPVContourFilter is an extension to vtkContourFilter. It adds the
// ability to generate isosurfaces / isolines for AMR dataset.
//
// .SECTION Caveats
// Certain flags in vtkAMRDualContour are assumed to be ON.
//
// .SECTION See Also
// vtkContourFilter vtkAMRDualContour

#ifndef __vtkPVContourFilter_h
#define __vtkPVContourFilter_h

#include "vtkContourFilter.h"

class VTK_EXPORT vtkPVContourFilter : public vtkContourFilter
{
public:
  vtkTypeMacro(vtkPVContourFilter, vtkContourFilter);

  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPVContourFilter* New();

  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);


protected:

  vtkPVContourFilter();
 ~vtkPVContourFilter();

 virtual int RequestData(vtkInformation* request,
                         vtkInformationVector** inputVector,
                         vtkInformationVector* outputVector);

 virtual int RequestDataObject(vtkInformation* request,
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector);

 virtual int FillInputPortInformation(int port, vtkInformation* info);
 virtual int FillOutputPortInformation(int port, vtkInformation* info);

 // Description:
 // Class superclass request data. Also handles iterating over
 // vtkHierarchicalBoxDataSet.
 int ContourUsingSuperclass(
   vtkInformation* request, vtkInformationVector** inputVector,
   vtkInformationVector* outputVector);

private:
 vtkPVContourFilter(const vtkPVContourFilter&); // Not implemented.
 void operator=(const vtkPVContourFilter&);     // Not implemented.
};

#endif // __vtkPVContourFilter_h
