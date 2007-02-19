/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionViewer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionViewer - Display a transfer function for editing.
// .SECTION Description
// vtkTransferFunctionViewer bundles together several classes (including a
// vtkRenderer and vtkRenderWindow) for displaying a transfer function
// editor. It invokes an InteractionEvent when the scalar range shown in
// the transfer function editor changes.
//
// .SECTION See Also
// vtkTransferFunctionEditorWidget vtkTransferFunctionEditorRepresentation
// vtkInteractorStyleTransferFunctionEditor

#ifndef __vtkTransferFunctionViewer_h
#define __vtkTransferFunctionViewer_h

#include "vtkObject.h"

class vtkColorTransferFunction;
class vtkDataSet;
class vtkEventForwarderCommand;
class vtkInteractorStyleTransferFunctionEditor;
class vtkPiecewiseFunction;
class vtkRectilinearGrid;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkTransferFunctionEditorWidget;

class VTK_EXPORT vtkTransferFunctionViewer : public vtkObject
{
public:
  static vtkTransferFunctionViewer* New();
  vtkTypeRevisionMacro(vtkTransferFunctionViewer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the background color of the rendering window.
  void SetBackgroundColor(double r, double g, double b);
  void SetBackgroundColor(double color[3])
    { this->SetBackgroundColor(color[0], color[1], color[2]); }

  // Description:
  // Set the size of the rendering window.
  void SetSize(int x, int y);
  void SetSize(int size[2])
    { this->SetSize(size[0], size[1]); }

  // Description:
  // Render the transfer function editor.
  void Render();

  // Description:
  // Specify the type of transfer function editor to dipslay in this viewer.
  void SetTransferFunctionEditorType(int type);
  void SetTransferFunctionEditorTypeToSimple1D()
    { this->SetTransferFunctionEditorType(SIMPLE_1D); }
  void SetTransferFunctionEditorTypeToShapes1D()
    { this->SetTransferFunctionEditorType(SHAPES_1D); }
  void SetTransferFunctionEditorTypeToShapes2D()
    { this->SetTransferFunctionEditorType(SHAPES_2D); }

//BTX
  enum EditorTypes
  {
    SIMPLE_1D = 0,
    SHAPES_1D,
    SHAPES_2D
  };
//ETX

  // Description:
  // Set the type of function to modify (color, opacity, or both).
  // Set the editor type before setting the modification type.
  void SetModificationType(int type);
  void SetModificationTypeToColor();
  void SetModificationTypeToOpacity();
  void SetModificationTypeToColorAndOpacity();

  // Description:
  // Set the histogram to display behind the transfer function editor.
  // The X coordinates of this vtkRectilinearGrid give the scalar range for
  // each bin of the histogram.
  // There is a cell array containing the number of scalar values per bin.
  // Set the editor type before setting the histogram.
  void SetHistogram(vtkRectilinearGrid *histogram);

  // Description:
  // Set the range of scalar values that will be shown in the rendering
  // window. Set the type of transfer function editor to use before setting
  // or getting the scalar range.
  void SetVisibleScalarRange(double range[2]) 
    { this->SetVisibleScalarRange(range[0], range[1]); }
  void SetVisibleScalarRange(double min, double max);
  void GetVisibleScalarRange(double range[2]);
  double* GetVisibleScalarRange();

  // Description:
  // Set the whole range of possible scalar values. This will be used for
  // resetting the viewer to show the whole scalar range when the input is
  // not set. Set the type of transfer function editor to use before setting
  // or getting the scalar range.
  void SetWholeScalarRange(double range[2])
    { this->SetWholeScalarRange(range[0], range[1]); }
  void SetWholeScalarRange(double min, double max);
  void GetWholeScalarRange(double range[2]);
  double* GetWholeScalarRange();
  
  // Description:
  // Provide access to some of the underlying VTK objects.
  vtkGetObjectMacro(Renderer, vtkRenderer);
  vtkGetObjectMacro(EditorWidget, vtkTransferFunctionEditorWidget);
  vtkGetObjectMacro(Interactor, vtkRenderWindowInteractor);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

  // Description:
  // Set the render window to be used in this viewer. One will be created
  // automatically if you do not specify one.
  void SetRenderWindow(vtkRenderWindow *win);

  // Description:
  // Set the render window to be used in this viewer. One will be created
  // automatically if you do not specify one.
  void SetInteractor(vtkRenderWindowInteractor *win);

  // Description:
  // Determine whether a histogram will be displayed behind the transfer
  // function editor.
  void SetHistogramVisibility(int visibility);

  // Description:
  // Get the opacity function.
  vtkPiecewiseFunction* GetOpacityFunction();

  // Description:
  // Get the color function.
  vtkColorTransferFunction* GetColorFunction();

  // Description:
  // Set the opacity of a particular transfer function element.
  void SetElementOpacity(unsigned int idx, double opacity);

  // Description:
  // Set the RGB color of a particular transfer function element.
  void SetElementRGBColor(unsigned int idx, double r, double g, double b);

  // Description:
  // Set the HSV color of a particular transfer function element.
  void SetElementHSVColor(unsigned int idx, double h, double s, double v);

  // Description:
  // Return the current element Id.
  unsigned int GetCurrentElementId();

  // Description:
  // Set the color space. (RGB = 0, HSV = 1, HSV with wrapping = 2)
  void SetColorSpace(int space);

protected:
  vtkTransferFunctionViewer();
  ~vtkTransferFunctionViewer();

  virtual void InstallPipeline();
  virtual void UnInstallPipeline();

  vtkRenderWindow *RenderWindow;
  vtkRenderer *Renderer;
  vtkRenderWindowInteractor *Interactor;
  vtkInteractorStyleTransferFunctionEditor *InteractorStyle;
  vtkTransferFunctionEditorWidget *EditorWidget;
  vtkEventForwarderCommand *EventForwarder;
  unsigned long HistogramMTime;
  vtkRectilinearGrid *Histogram;

private:
  vtkTransferFunctionViewer(const vtkTransferFunctionViewer&); // Not implemented.
  void operator=(const vtkTransferFunctionViewer&); // Not implemented.
};

#endif
