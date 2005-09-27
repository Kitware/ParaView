/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyle.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInteractorStyle - interactive manipulation of the camera
// .SECTION Description
// vtkPVInteractorStyle allows the user to interactively
// manipulate the camera, the viewpoint of the scene.
// The left button is for rotation; shift + left button is for rolling;
// the right button is for panning; and shift + right button is for zooming.

#ifndef __vtkPVInteractorStyle_h
#define __vtkPVInteractorStyle_h

#include "vtkInteractorStyle.h"

class vtkPVCameraManipulator;
class vtkCollection;

class VTK_EXPORT vtkPVInteractorStyle : public vtkInteractorStyle
{
public:
  static vtkPVInteractorStyle *New();
  vtkTypeRevisionMacro(vtkPVInteractorStyle, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnMiddleButtonDown();
  virtual void OnMiddleButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();
  
  // Description:
  // Access to adding or removing manipulators.
  void AddManipulator(vtkPVCameraManipulator *m);

  //BTX
  // Description:
  // Accessor for the collection of camera manipulators.
  vtkGetObjectMacro(CameraManipulators, vtkCollection);
  //ETX

  // Description:
  // Propagates the center to the manipulators.
  void SetCenterOfRotation(float x, float y, float z);

  // Description:
  // Do not let the superclass do anything with a char event.
  virtual void OnChar() {};

protected:
  vtkPVInteractorStyle();
  ~vtkPVInteractorStyle();

  vtkPVCameraManipulator *Current;

  // The CameraInteractors also store there button and modifier.
  vtkCollection *CameraManipulators;

  void OnButtonDown(int button, int shift, int control);
  void OnButtonUp(int button);
  void ResetLights();

  vtkPVInteractorStyle(const vtkPVInteractorStyle&); // Not implemented
  void operator=(const vtkPVInteractorStyle&); // Not implemented
};

#endif
