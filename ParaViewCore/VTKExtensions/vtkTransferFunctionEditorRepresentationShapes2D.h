/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorRepresentationShapes2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionEditorRepresentationShapes2D - a representation of a 3D widget for manipulating a transfer function
// .SECTION Description
// vtkTransferFunctionEditorRepresentationShapes2D is the representation
// associated with vtkTransferFunctionEditorWidgetShapes2D. It is used for
// displaying / manipulating a 2D transfer function using shapes.

#ifndef __vtkTransferFunctionEditorRepresentationShapes2D_h
#define __vtkTransferFunctionEditorRepresentationShapes2D_h

#include "vtkTransferFunctionEditorRepresentation.h"

class VTK_EXPORT vtkTransferFunctionEditorRepresentationShapes2D : public vtkTransferFunctionEditorRepresentation
{
public:
  static vtkTransferFunctionEditorRepresentationShapes2D* New();
  vtkTypeMacro(vtkTransferFunctionEditorRepresentationShapes2D, vtkTransferFunctionEditorRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Put together the parts necessary for displaying this 3D widget.
  virtual void BuildRepresentation();

protected:
  vtkTransferFunctionEditorRepresentationShapes2D() {}
  ~vtkTransferFunctionEditorRepresentationShapes2D() {}

private:
  vtkTransferFunctionEditorRepresentationShapes2D(const vtkTransferFunctionEditorRepresentationShapes2D&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorRepresentationShapes2D&); // Not implemented.
};

#endif
