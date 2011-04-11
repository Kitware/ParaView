/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidgetShapes1D.h,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionEditorWidgetShapes1D - a 3D widget for manipulating a transfer function
// .SECTION Description
// vtkTransferFunctionEditorWidgetShapes1D is a 3D widget used for manipulating
// 1D transfer functions using shapes.

#ifndef __vtkTransferFunctionEditorWidgetShapes1D_h
#define __vtkTransferFunctionEditorWidgetShapes1D_h

#include "vtkTransferFunctionEditorWidget1D.h"

class VTK_EXPORT vtkTransferFunctionEditorWidgetShapes1D : public vtkTransferFunctionEditorWidget1D
{
public:
  static vtkTransferFunctionEditorWidgetShapes1D* New();
  vtkTypeMacro(vtkTransferFunctionEditorWidgetShapes1D, vtkTransferFunctionEditorWidget1D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a default widget representation,
  // vtkTransferFunctionEditorRepresentationShapes1D in this case.
  virtual void CreateDefaultRepresentation();

protected:
  vtkTransferFunctionEditorWidgetShapes1D() {}
  ~vtkTransferFunctionEditorWidgetShapes1D() {}

private:
  vtkTransferFunctionEditorWidgetShapes1D(const vtkTransferFunctionEditorWidgetShapes1D&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorWidgetShapes1D&); // Not implemented.
};

#endif
