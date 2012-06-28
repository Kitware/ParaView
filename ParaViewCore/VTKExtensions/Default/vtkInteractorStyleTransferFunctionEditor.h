/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTransferFunctionEditor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleTransferFunctionEditor - an interactor style for the vtkTransferFunctionViewer
// .SECTION Description
// vtkInteractorStyleTransferFunctionEditor is a subclass of vtkInteractorStyle
// that is designed for use with the vtkTransferFunctionViewer and a
// vtkTransferFunctionEditorWidget. Instead of modifying the camera or the
// position of actors in the scene, this interactor style specifies the
// scalar range to be displayed in the transfer function editor.

#ifndef __vtkInteractorStyleTransferFunctionEditor_h
#define __vtkInteractorStyleTransferFunctionEditor_h

#include "vtkInteractorStyle.h"

class vtkTransferFunctionEditorWidget;

class VTK_EXPORT vtkInteractorStyleTransferFunctionEditor : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleTransferFunctionEditor* New();
  vtkTypeMacro(vtkInteractorStyleTransferFunctionEditor, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove();
  virtual void OnMiddleButtonDown();
  virtual void OnMiddleButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();
  virtual void OnConfigure();
  virtual void OnChar();

  // Description:
  // Set the vtkTransferFunctionEditorWidget. The Pan and Zoom methods
  // determine the scalar range of the widget to be displayed.
  void SetWidget(vtkTransferFunctionEditorWidget *widget);

  // Description:
  // Overriding Pan and Zoom methods. These methods affect the scalar range
  // of the vtkTransferFunctionEditorWidget, not the vtkCamera.
  virtual void Pan();
  virtual void Zoom();

protected:
  vtkInteractorStyleTransferFunctionEditor();
  ~vtkInteractorStyleTransferFunctionEditor();

  vtkTransferFunctionEditorWidget *Widget;
  double MotionFactor;

private:
  vtkInteractorStyleTransferFunctionEditor(const vtkInteractorStyleTransferFunctionEditor&); // Not implemented.
  void operator=(const vtkInteractorStyleTransferFunctionEditor&); // Not implemented.
};

#endif
