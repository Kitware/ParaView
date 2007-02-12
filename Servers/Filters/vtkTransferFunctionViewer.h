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

class vtkDataSet;
class vtkEventForwarderCommand;
class vtkInteractorStyleTransferFunctionEditor;
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
  // Set the input data set containing the scalar array used in the
  // transfer function. If the input is not set, the histogram will not be
  // visible, regardless of how HistogramVisibility is set.
  void SetInput(vtkDataSet *input);

  // Description:
  // Set the name of the data array in the input data set to process.
  void SetArrayName(const char* name);

  // Description:
  // Set whether the array to process is point-centered or cell-centered.
  void SetFieldAssociation(int assoc);
  void SetFieldAssociationToPoints();
  void SetFieldAssociationToCells();

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
  vtkDataSet *Input;
  char *ArrayName;
  int FieldAssociation;
  vtkEventForwarderCommand *EventForwarder;
  unsigned long InputMTime;

private:
  vtkTransferFunctionViewer(const vtkTransferFunctionViewer&); // Not implemented.
  void operator=(const vtkTransferFunctionViewer&); // Not implemented.
};

#endif
