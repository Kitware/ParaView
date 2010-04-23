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
  vtkTypeMacro(vtkTransferFunctionEditorWidget, vtkAbstractWidget);
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
  void SetWholeScalarRange(double min, double max);
  void SetWholeScalarRange(double range[2])
    { this->SetWholeScalarRange(range[0], range[1]); }
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
  // Set/get the opacity transfer function.
  vtkGetObjectMacro(OpacityFunction, vtkPiecewiseFunction);
  void SetOpacityFunction(vtkPiecewiseFunction *function);

  // Description:
  // Set/get the color transfer function.
  vtkGetObjectMacro(ColorFunction, vtkColorTransferFunction);
  void SetColorFunction(vtkColorTransferFunction *function);

  // Description:
  // Toggle whether to allow interior nodes in the transfer function.
  vtkSetClampMacro(AllowInteriorElements, int, 0, 1);
  vtkGetMacro(AllowInteriorElements, int);
  vtkBooleanMacro(AllowInteriorElements, int);

  // Description:
  // Set/get the opacity of a particular element in the transfer function
  // editor.
  virtual void SetElementOpacity(unsigned int, double) {}
  virtual double GetElementOpacity(unsigned int) { return 0; }

  // Description:
  // Set/get the RGB color of a particular element in the transfer function
  // editor.
  virtual void SetElementRGBColor(unsigned int, double, double, double) {}
//BTX
  virtual int GetElementRGBColor(unsigned int, double[3]) { return 0; }
//ETX

  // Description:
  // Set the HSV color of a particular element in the transfer function editor.
  virtual void SetElementHSVColor(unsigned int, double, double, double) {}
//BTX
  virtual int GetElementHSVColor(unsigned int, double[3]) { return 0; }
//ETX

  // Description:
  // Set the scalar value associated with a particular element in the transfer
  // function editor.
  virtual void SetElementScalar(unsigned int, double) {}
  virtual double GetElementScalar(unsigned int) { return 0.0; }

  // Description:
  // Set/get the rectilinear grid containing the histogram.
  virtual void SetHistogram(vtkRectilinearGrid *histogram);
  vtkGetObjectMacro(Histogram, vtkRectilinearGrid);

  // Description:
  // Set the color space. (RGB = 0, HSV = 1, HSV with wrapping = 2,
  // CIELAB = 3, Diverging = 4)
  virtual void SetColorSpace(int) {}

  // Description:
  // Move to previous/next transfer function element.
  virtual void MoveToPreviousElement();
  virtual void MoveToNextElement();

  // Description:
  // Respond to keypress events.
  virtual void OnChar();

  // Description:
  // Get the modified time associated with the color / opacity transfer
  // functions.
  vtkGetMacro(ColorMTime, unsigned long);
  vtkGetMacro(OpacityMTime, unsigned long);

  // Description:
  // Update this widget based on changes to the transfer functions.
  virtual void UpdateFromTransferFunctions() {}

  // Description:
  // Create a default representation for this widget,
  // vtkTransferFunctionEditorRepresentationSimple1D in this case.
  virtual void CreateDefaultRepresentation();

  // Description:
  // Set the width (in pixels) of the border around the transfer function
  // editor.
  virtual void SetBorderWidth(int width);

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
  unsigned long ColorMTime;
  unsigned long OpacityMTime;
  int AllowInteriorElements;
  int BorderWidth;

  virtual void UpdateTransferFunctionMTime();

private:
  vtkTransferFunctionEditorWidget(const vtkTransferFunctionEditorWidget&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorWidget&); // Not implemented.
};

#endif
