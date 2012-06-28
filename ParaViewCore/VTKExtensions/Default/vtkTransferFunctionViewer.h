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
// This viewer invokes the following events from vtkCommand:
// * InteractionEvent - Lets you know the visible scalar range of the data has
// changed because of panning, zooming, or resetting to the whole scalar range.
// * PickEvent - Lets you know a transfer function element has been left-
// clicked so you can display a color chooser to select a color for it.
// * WidgetValueChangedEvent - Lets you know that a transfer function
// node has been moved.
// * EndInteractionEvent - Lets you know the left mouse button has been
// released after moving a node.
// * WidgetModifiedEvent - Lets you know that the active node has changed.
// * PlacePointEvent - Lets you know a node has been added or deleted.
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
  vtkTypeMacro(vtkTransferFunctionViewer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the background color of the rendering window.
  void SetBackgroundColor(double r, double g, double b);
  void SetBackgroundColor(double color[3])
    { this->SetBackgroundColor(color[0], color[1], color[2]); }

  // Description:
  // Set the color of the histogram.
  // Set the transfer function editor type before setting the color of the
  // histogram.
  void SetHistogramColor(double r, double g, double b);
  void SetHistogramColor(double color[3])
    { this->SetHistogramColor(color[0], color[1], color[2]); }

  // Description:
  // Set/get the size of the rendering window.
  void SetSize(int x, int y);
  void SetSize(int size[2])
    { this->SetSize(size[0], size[1]); }
  int* GetSize();

  // Description:
  // Set the width (in pixels) of the border around the transfer function
  // editor.
  void SetBorderWidth(int width);
  
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
  // Determine whether a gradient image showing the color function
  // should be displayed in the background.
  // Set the transfer function editor type before setting this value.
  void SetShowColorFunctionInBackground(int visibility);

  // Description:
  // Determine whether a gradient image showing the color function
  // should be displayed in the histogram (if the histogram is visible).
  // Set the transfer function editor type before setting this value.
  void SetShowColorFunctionInHistogram(int visibility);

  // Description:
  // Determine whether the color gradient should be displayed on the lines
  // connecting the nodes of the transfer function editor.
  // Set the transfer function editor type before setting this value.
  void SetShowColorFunctionOnLines(int visibility);

  // Description:
  // Specify the color in which to display the lines connecting the nodes
  // if they are not displaying the color gradient.
  // Set the transfer function editor type before calling this method.
  void SetLinesColor(double r, double g, double b);

  // Description:
  // Specify whether the node color should be determined by the color
  // transfer function.
  // Set the transfer function editor type before setting this value.
  void SetColorElementsByColorFunction(int color);

  // Description:
  // Set the color to use for nodes if not coloring them by the color
  // transfer function.
  // Set the transfer function editor type before calling this method.
  void SetElementsColor(double r, double g, double b);

  // Description:
  // Specify the lighting parameters for the transfer function editor
  // elements. Set the transfer function editor type before calling this
  // method.
  void SetElementLighting(double ambient, double diffuse,
                          double specular, double specularPower);

  // Description:
  // Set/get the opacity function.
  // Set the type of transfer function editor before trying to set or get
  // the opacity function.
  void SetOpacityFunction(vtkPiecewiseFunction *function);
  vtkPiecewiseFunction* GetOpacityFunction();

  // Description:
  // Set/get the color function.
  // Set the type of transfer function editor before trying to set or get
  // the color function.
  void SetColorFunction(vtkColorTransferFunction *function);
  vtkColorTransferFunction* GetColorFunction();

  // Description:
  // Toggle whether to allow interior nodes in the transfer function.
  // Set the type of transfer function editor before trying to set this value.
  void SetAllowInteriorElements(int allow);

  // Description:
  // Set/get the opacity of a particular transfer function element.
  void SetElementOpacity(unsigned int idx, double opacity);
  double GetElementOpacity(unsigned int idx);

  // Description:
  // Set/get the RGB color of a particular transfer function element.
  void SetElementRGBColor(unsigned int idx, double r, double g, double b);
  int GetElementRGBColor(unsigned int idx, double color[3]);

  // Description:
  // Set/get the HSV color of a particular transfer function element.
  void SetElementHSVColor(unsigned int idx, double h, double s, double v);
  int GetElementHSVColor(unsigned int idx, double color[3]);

  // Description:
  // Set/get the scalar value of a particular transfer function element.
  void SetElementScalar(unsigned int idx, double scalar);
  double GetElementScalar(unsigned int idx);

  // Description:
  // Set/get the current element Id.
  void SetCurrentElementId(unsigned int idx);
  unsigned int GetCurrentElementId();

  // Description:
  // Move to the previous/next transfer function element.
  void MoveToPreviousElement();
  void MoveToNextElement();

  // Description:
  // Set the color space. (RGB = 0, HSV = 1, HSV with wrapping = 2,
  // CIELAB = 3, Diverging = 4)
  void SetColorSpace(int space);

  // Description:
  // Set whether the endpoint nodes may or may not change scalar values 
  // and/or be deleted. By default moving and deleting are allowed.
  // Set the transfer function editor type before calling this method.
  // It only has an effect on the simple 1D case.
  void SetLockEndPoints(int lock);

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
  vtkRectilinearGrid *Histogram;

private:
  vtkTransferFunctionViewer(const vtkTransferFunctionViewer&); // Not implemented.
  void operator=(const vtkTransferFunctionViewer&); // Not implemented.
};

#endif
