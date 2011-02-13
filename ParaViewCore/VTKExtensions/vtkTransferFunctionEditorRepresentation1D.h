/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorRepresentation1D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionEditorRepresentation1D - a representation of a 3D widget for manipulating a transfer function
// .SECTION Description
// vtkTransferFunctionEditorRepresentation1D is the superclass of
// representations associated with 3D widgets for dislaying and manipulating
// 1D transfer functions. The histogram associated with this representation
// is a bar chart indicating the number of times a particular scalar value is
// used. (Actually the log of this number is displayed.)
//
// .SECTION See Also
// vtkTransferFunctionEditorRepresentationSimple1D
// vtkTransferFunctionEditorRepresentationShapes1D

#ifndef __vtkTransferFunctionEditorRepresentation1D_h
#define __vtkTransferFunctionEditorRepresentation1D_h

#include "vtkTransferFunctionEditorRepresentation.h"

class VTK_EXPORT vtkTransferFunctionEditorRepresentation1D : public vtkTransferFunctionEditorRepresentation
{
public:
  vtkTypeMacro(vtkTransferFunctionEditorRepresentation1D, vtkTransferFunctionEditorRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Put together the necessary parts to display this 3D widget
  virtual void BuildRepresentation();

  // Description:
  // Toggle whether to display the color function in the histogram.
  virtual void SetShowColorFunctionInHistogram(int color);

  // Description:
  // Set the color transfer function being modified.
  virtual void SetColorFunction(vtkColorTransferFunction *color);

protected:
  vtkTransferFunctionEditorRepresentation1D();
  ~vtkTransferFunctionEditorRepresentation1D();

  void UpdateHistogramImage();

private:
  vtkTransferFunctionEditorRepresentation1D(const vtkTransferFunctionEditorRepresentation1D&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorRepresentation1D&); // Not implemented.
};

#endif
