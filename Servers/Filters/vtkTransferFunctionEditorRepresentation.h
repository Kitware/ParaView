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

class vtkActor2D;
class vtkColorTransferFunction;
class vtkImageMapper;

class VTK_EXPORT vtkTransferFunctionEditorRepresentation : public vtkWidgetRepresentation
{
public:
  vtkTypeRevisionMacro(vtkTransferFunctionEditorRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the visibility of the histogram. Defaults to on.
  vtkSetMacro(HistogramVisibility, int);
  vtkGetMacro(HistogramVisibility, int);
  vtkBooleanMacro(HistogramVisibility, int);

  // Description:
  // Rendering methods
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
  virtual int RenderOverlay(vtkViewport *viewport);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
  // Does this prop have some translucent polygonal geometry?
  // This method is called during the rendering process to know if there is
  // some translucent polygonal geometry. A simple prop that has some
  // translucent polygonal geometry will return true. A composite prop (like
  // vtkAssembly) that has at least one sub-prop that has some translucent
  // polygonal geometry will return true.
  // Default implementation return false.
  virtual int HasTranslucentPolygonalGeometry(); 

  // Description:
  // Set/get the size of the display containing this representation.
  vtkSetVector2Macro(DisplaySize, int);
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

protected:
  vtkTransferFunctionEditorRepresentation();
  ~vtkTransferFunctionEditorRepresentation();

  vtkImageMapper *HistogramMapper;
  vtkActor2D *HistogramActor;
  int HistogramVisibility;
  int DisplaySize[2];
  int ScalarBinRange[2];
  double HistogramColor[3];
  vtkColorTransferFunction *ColorFunction;

private:
  vtkTransferFunctionEditorRepresentation(const vtkTransferFunctionEditorRepresentation&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorRepresentation&); // Not implemented.
};

#endif
