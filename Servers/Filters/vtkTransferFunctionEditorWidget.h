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

class vtkColorTransferFunction;
class vtkDataSet;
class vtkPiecewiseFunction;
class vtkRectilinearGrid;

class VTK_EXPORT vtkTransferFunctionEditorWidget : public vtkAbstractWidget
{
public:
  vtkTypeRevisionMacro(vtkTransferFunctionEditorWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the scalar range to show in the transfer function editor.
  virtual void SetVisibleScalarRange(double range[2])
    { this->SetVisibleScalarRange(range[0], range[1]); }
  virtual void SetVisibleScalarRange(double min, double max);
  vtkGetVector2Macro(VisibleScalarRange, double);

  // Description:
  // Set the whole range of possible scalar values to show in the transfer
  // function editor. This is used for showing the whole scalar range when
  // the histogram is not set.
  vtkSetVector2Macro(WholeScalarRange, double);
  vtkGetVector2Macro(WholeScalarRange, double);

  // Description:
  // Reset the widget so that it shows the whole scalar range of the input
  // data set.
  void ShowWholeScalarRange();

  // Description:
  // Reconfigure the widget based on the size of the renderer containing it.
  virtual void Configure(int size[2]);

  // Description:
  // Set the type of function to modify.
  vtkSetClampMacro(ModificationType, int, 0, 2);
  
//BTX
  enum ModificationTypes
  {
    COLOR = 0,
    OPACITY,
    COLOR_AND_OPACITY
  };
//ETX

  // Description:
  // Get the opacity transfer function.
  vtkGetObjectMacro(OpacityFunction, vtkPiecewiseFunction);

  // Description:
  // Get the color transfer function.
  vtkGetObjectMacro(ColorFunction, vtkColorTransferFunction);

  // Description:
  // Set the opacity of a particular element in the transfer function editor.
  virtual void SetElementOpacity(unsigned int, double) {}

  // Description:
  // Set the RGB color of a particular element in the transfer function editor.
  // Description:
  virtual void SetElementRGBColor(unsigned int, double, double, double) {}

  // Description:
  // Set the HSV color of a particular element in the transfer function editor.
  // Description:
  virtual void SetElementHSVColor(unsigned int, double, double, double) {}

  // Description:
  // Set/get the rectilinear grid containing the histogram.
  virtual void SetHistogram(vtkRectilinearGrid *histogram);
  vtkGetObjectMacro(Histogram, vtkRectilinearGrid);

  // Description:
  // Set the color space. (RGB = 0, HSV = 1, HSV with wrapping = 2)
  virtual void SetColorSpace(int) {}

protected:
  vtkTransferFunctionEditorWidget();
  ~vtkTransferFunctionEditorWidget();

  double VisibleScalarRange[2];
  double WholeScalarRange[2];
  int NumberOfScalarBins; // used for float and double input images
  int ModificationType;
  vtkPiecewiseFunction *OpacityFunction;
  vtkColorTransferFunction *ColorFunction;
  vtkRectilinearGrid *Histogram;

private:
  vtkTransferFunctionEditorWidget(const vtkTransferFunctionEditorWidget&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorWidget&); // Not implemented.
};

#endif
