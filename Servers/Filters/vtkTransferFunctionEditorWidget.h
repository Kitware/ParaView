/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionEditorWidget - a 3D widget for manipulating a transfer function
// .SECTION Description
// vtkTransferFunctionEditorWidget is a superclass for 3D widgets that
// manipulate a transfer function. It allows you to interactively change the
// transfer function used for determining the colors used in rendering an actor
// or volume.
//
// .SECTION See Also
// vtkTransferFunctionEditorWidgetSimple1D
// vtkTransferFunctionEditorWidgetShapes1D
// vtkTransferFunctionEditorWidgetShapes2D

#ifndef __vtkTransferFunctionEditorWidget_h
#define __vtkTransferFunctionEditorWidget_h

#include "vtkAbstractWidget.h"

class vtkDataSet;

class VTK_EXPORT vtkTransferFunctionEditorWidget : public vtkAbstractWidget
{
public:
  vtkTypeRevisionMacro(vtkTransferFunctionEditorWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input data set containing the array used for computing the
  // histogram and the scalar range.
  void SetInput(vtkDataSet *input);

  // Description:
  // Set the name of the array in the input data set to be processed.
  void SetArrayName(const char *name);

  // Description:
  // Set the field association (points = 0, cells = 1) of the array to process.
  void SetFieldAssociation(int assoc);

  // Description:
  // Set the scalar range to show in the transfer function editor.
  virtual void SetScalarRange(double range[2])
    { this->SetScalarRange(range[0], range[1]); }
  virtual void SetScalarRange(double min, double max);
  vtkGetVector2Macro(ScalarRange, double);

  // Description:
  // Reset the widget so that it shows the whole scalar range of the input
  // data set.
  void ShowWholeScalarRange();

  // Description:
  // Reconfigure the widget based on the size of the renderer containing it.
  virtual void Configure(int size[2]);

protected:
  vtkTransferFunctionEditorWidget();
  ~vtkTransferFunctionEditorWidget();

  virtual void ComputeHistogram() = 0;

  vtkDataSet *Input;
  char *ArrayName;
  int FieldAssociation;
  double ScalarRange[2];
  int NumberOfScalarBins; // used for float and double input images

private:
  vtkTransferFunctionEditorWidget(const vtkTransferFunctionEditorWidget&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorWidget&); // Not implemented.
};

#endif
