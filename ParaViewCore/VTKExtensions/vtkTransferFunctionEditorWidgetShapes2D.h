/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidgetShapes2D.h,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionEditorWidgetShapes2D - a 3D widget for manipulating a transfer function
// .SECTION Description
// vtkTransferFunctionEditorWidgetShapes1D is a 3D widget used for manipulating
// 2D transfer functions using shapes.

#ifndef __vtkTransferFunctionEditorWidgetShapes2D_h
#define __vtkTransferFunctionEditorWidgetShapes2D_h

#include "vtkTransferFunctionEditorWidget.h"

class VTK_EXPORT vtkTransferFunctionEditorWidgetShapes2D : public vtkTransferFunctionEditorWidget
{
public:
  static vtkTransferFunctionEditorWidgetShapes2D* New();
  vtkTypeMacro(vtkTransferFunctionEditorWidgetShapes2D, vtkTransferFunctionEditorWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a default widget representation,
  // vtkTransferFunctionEditorRepresentationShapes2D in this case.
  virtual void CreateDefaultRepresentation();

protected:
  vtkTransferFunctionEditorWidgetShapes2D() {}
  ~vtkTransferFunctionEditorWidgetShapes2D() {}

private:
  vtkTransferFunctionEditorWidgetShapes2D(const vtkTransferFunctionEditorWidgetShapes2D&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorWidgetShapes2D&); // Not implemented.
};

#endif
