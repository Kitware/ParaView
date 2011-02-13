/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidget1D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionEditorWidget1D - a 3D widget for manipulating a transfer function
// .SECTION Description
// vtkTransferFunctionEditorWidget1D is a superclass for 3D widget used for
// manipulating 1D transfer functions.
//
// .SECTION See Also
// vtkTransferFunctionEditorWidgetSimple1D
// vtkTransferFunctionEditorWidgetShapes1D

#ifndef __vtkTransferFunctionEditorWidget1D_h
#define __vtkTransferFunctionEditorWidget1D_h

#include "vtkTransferFunctionEditorWidget.h"

class VTK_EXPORT vtkTransferFunctionEditorWidget1D : public vtkTransferFunctionEditorWidget
{
public:
  vtkTypeMacro(vtkTransferFunctionEditorWidget1D, vtkTransferFunctionEditorWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the histogram.
  virtual void SetHistogram(vtkRectilinearGrid *histogram);

protected:
  vtkTransferFunctionEditorWidget1D();
  ~vtkTransferFunctionEditorWidget1D();

  double ComputeScalar(double pos, int width);
  double ComputePositionFromScalar(double scalar, int width);

private:
  vtkTransferFunctionEditorWidget1D(const vtkTransferFunctionEditorWidget1D&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorWidget1D&); // Not implemented.
};

#endif
