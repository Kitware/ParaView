/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorRepresentationShapes1D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionEditorRepresentationShapes1D - a representation for a 3D widget for manipulating a transfer function
// .SECTION Description
// vtkTransferFunctionEditorRepresentationShapes1D is the representation
// associated with vtkTransferFunctionEditorWidgetShapes1D. It is used for
// displaying / manipulating a 1D transfer function using shapes.

#ifndef __vtkTransferFunctionEditorRepresentationShapes1D_h
#define __vtkTransferFunctionEditorRepresentationShapes1D_h

#include "vtkTransferFunctionEditorRepresentation1D.h"

class VTK_EXPORT vtkTransferFunctionEditorRepresentationShapes1D : public vtkTransferFunctionEditorRepresentation1D
{
public:
  static vtkTransferFunctionEditorRepresentationShapes1D* New();
  vtkTypeMacro(vtkTransferFunctionEditorRepresentationShapes1D, vtkTransferFunctionEditorRepresentation1D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Put together the parts necessary for displaying this 3D widget.
  virtual void BuildRepresentation();

protected:
  vtkTransferFunctionEditorRepresentationShapes1D() {}
  ~vtkTransferFunctionEditorRepresentationShapes1D() {}

private:
  vtkTransferFunctionEditorRepresentationShapes1D(const vtkTransferFunctionEditorRepresentationShapes1D&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorRepresentationShapes1D&); // Not implemented.
};

#endif
