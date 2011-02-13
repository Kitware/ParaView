/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionEditorRepresentation - a representation for a 3D widget for manipulating a transfer function
// .SECTION Description
// vtkTransferFunctionEditorRepresentation is a superclass for representations
// associated with subclasses of vtkTransferFunctionEditorWidget. In addition
// to displaying the nodes / shapes for specifying the transfer function,
// this class also allows you to display a histogram in the background.
//
// .SECTION See Also
// vtkTransferFunctionEditorRepresentationSimple1D
// vtkTransferFunctionEditorRepresentationShapes1D
// vtkTransferFunctionEditorRepresentationShapes2D


#ifndef __vtkTransferFunctionEditorRepresentation_h
#define __vtkTransferFunctionEditorRepresentation_h

#include "vtkWidgetRepresentation.h"

class vtkActor;
class vtkColorTransferFunction;
class vtkImageData;
class vtkIntArray;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkTexture;

class VTK_EXPORT vtkTransferFunctionEditorRepresentation : public vtkWidgetRepresentation
{
public:
  vtkTypeMacro(vtkTransferFunctionEditorRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the visibility of the histogram. Defaults to on.
  vtkSetMacro(HistogramVisibility, int);
  vtkGetMacro(HistogramVisibility, int);
  vtkBooleanMacro(HistogramVisibility, int);

  // Description:
  // Toggle whether to display the color function in the histogram.
  vtkSetMacro(ShowColorFunctionInHistogram, int);

  // Description:
  // Rendering methods
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Release any graphics resources that are being consumed by this widget
  // representation. The parameter window could be used to determine which
  // graphic resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *window);

  // Description:
  // Set/get the size of the display containing this representation.
  virtual void SetDisplaySize(int x, int y);
  virtual void SetDisplaySize(int size[2])
    { this->SetDisplaySize(size[0], size[1]); }
  vtkGetVector2Macro(DisplaySize, int);

  // Description:
  // Set the starting and ending index into the histogram to be used when
  // displaying the histogram behind the widget.
  vtkSetVector2Macro(ScalarBinRange, int);

  // Description:
  // Set/get the current handle Id.
  virtual void SetActiveHandle(unsigned int) {}
  virtual unsigned int GetActiveHandle() { return 0; }

  // Description:
  // Get the number of existing handles.
  virtual unsigned int GetNumberOfHandles() { return 0; }

  // Description:
  // Set the color of the histogram.
  vtkSetVector3Macro(HistogramColor, double);

  // Description:
  // Set the color transfer function being modified.
  virtual void SetColorFunction(vtkColorTransferFunction *color);

  // Description:
  // Toggle whether to display to color transfer function as a gradient in
  // the background of the editor.
  vtkSetMacro(ShowColorFunctionInBackground, int);
  vtkGetMacro(ShowColorFunctionInBackground, int);
  vtkBooleanMacro(ShowColorFunctionInBackground, int);

  // Description:
  // Tell the representation whether the lines should be a solid color or
  // whether they should display the color transfer function.
  virtual void SetColorLinesByScalar(int) {}

  // Description:
  // Specify the color to use for the lines if they are not displaying
  // the color transfer function.
  virtual void SetLinesColor(double, double, double) {}

  // Description:
  // Specify whether the node color should be determined by the color
  // transfer function.
  vtkSetMacro(ColorElementsByColorFunction, int);
  vtkGetMacro(ColorElementsByColorFunction, int);

  // Description:
  // Set the color to use for nodes if not coloring them by the color
  // transfer function.
  vtkSetVector3Macro(ElementsColor, double);

  // Description:
  // Set/get the visible scalar range.
  vtkSetVector2Macro(VisibleScalarRange, double);
  vtkGetVector2Macro(VisibleScalarRange, double);

  // Description:
  // Set the lighting parameters for the transfer function editor elements.
  virtual void SetElementLighting(double, double, double, double) {}

  // Description:
  // Set the border width (in pixels) so the histogram and background images
  // may be sized accordingly.
  virtual void SetBorderWidth(int width);

  // Description:
  // Set the int array containing the histogram (computed by the
  // associated vtkTransferFunctionEditorWidget1D).
  void SetHistogram(vtkIntArray* histogram);

  // Description:
  // Get the modified time associated with the histogram.
  vtkGetMacro(HistogramMTime, unsigned long);

protected:
  vtkTransferFunctionEditorRepresentation();
  ~vtkTransferFunctionEditorRepresentation();

  vtkImageData *HistogramImage;
  vtkTexture *HistogramTexture;
  vtkPolyData *HistogramGeometry;
  vtkPolyDataMapper *HistogramMapper;
  vtkActor *HistogramActor;
  int HistogramVisibility;
  int ColorElementsByColorFunction;
  double ElementsColor[3];
  int DisplaySize[2];
  int ScalarBinRange[2];
  double HistogramColor[3];
  vtkColorTransferFunction *ColorFunction;
  vtkPolyData *BackgroundImage;
  vtkPolyDataMapper *BackgroundMapper;
  vtkActor *BackgroundActor;
  int ShowColorFunctionInBackground;
  int ShowColorFunctionInHistogram;
  double VisibleScalarRange[2];
  int BorderWidth;
  unsigned long HistogramMTime;
  vtkIntArray *Histogram;

  void InitializeImage(vtkImageData *image);
  void UpdateActorSize(vtkActor *actor);
  void UpdateHistogramMTime();

private:
  vtkTransferFunctionEditorRepresentation(const vtkTransferFunctionEditorRepresentation&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorRepresentation&); // Not implemented.
};

#endif
