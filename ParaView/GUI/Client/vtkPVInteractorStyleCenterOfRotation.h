/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyleCenterOfRotation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInteractorStyleCenterOfRotation - interactively select center of rotation
// .SECTION Description
// vtkPVInteractorStyleCenterOfRotation is a helper interactor style used
// in ParaView to interactively pick the center of rotation for the
// vtkPVInteractorStyleRotateCamera.

#ifndef __vtkPVInteractorStyleCenterOfRotation_h
#define __vtkPVInteractorStyleCenterOfRotation_h

#include "vtkInteractorStyle.h"
class vtkSMRenderModuleProxy;
class vtkPVWindow;
class vtkPVWorldPointPicker;

class VTK_EXPORT vtkPVInteractorStyleCenterOfRotation :public vtkInteractorStyle
{
public:
  static vtkPVInteractorStyleCenterOfRotation *New();
  vtkTypeRevisionMacro(vtkPVInteractorStyleCenterOfRotation, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();

  // Description:
  // These methods are for the interactions for this interactor style
  virtual void Pick();

  // Description:
  // Access to the center of rotation values
  vtkGetVector3Macro(Center, float);

  // Description:
  // The render module is for picking.
  void SetRenderModuleProxy(vtkSMRenderModuleProxy* rm);

  void SetPVWindow(vtkPVWindow* );
  vtkPVWindow* GetPVWindow() { return this->PVWindow;}
protected:
  vtkPVInteractorStyleCenterOfRotation();
  ~vtkPVInteractorStyleCenterOfRotation();
  
  // Description:
  // Set the center of rotation.
  void SetCenter(float x, float y, float z);
  float Center[3];
  
  vtkPVWorldPointPicker *Picker;
  // For picking.  Use a proxy in the future.
  vtkSMRenderModuleProxy* RenderModuleProxy;
  vtkPVWindow*       PVWindow;
  
  vtkPVInteractorStyleCenterOfRotation(const vtkPVInteractorStyleCenterOfRotation&); // Not implemented
  void operator=(const vtkPVInteractorStyleCenterOfRotation&); // Not implemented
};

#endif
