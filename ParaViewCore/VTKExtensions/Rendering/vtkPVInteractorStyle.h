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
// This class fires vtkCommand::StartInteractionEvent and 
// vtkCommand::EndInteractionEvent to signal start and end of interaction.

#ifndef __vtkPVInteractorStyle_h
#define __vtkPVInteractorStyle_h

#include "vtkInteractorStyleTrackballCamera.h"

class vtkCameraManipulator;
class vtkCollection;

class VTK_EXPORT vtkPVInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkPVInteractorStyle *New();
  vtkTypeMacro(vtkPVInteractorStyle, vtkInteractorStyleTrackballCamera);
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
  void AddManipulator(vtkCameraManipulator *m);

  // Description:
  // Removes all manipulators.
  void RemoveAllManipulators();

  //BTX
  // Description:
  // Accessor for the collection of camera manipulators.
  vtkGetObjectMacro(CameraManipulators, vtkCollection);
  //ETX

  // Description:
  // Propagates the center to the manipulators.
  // This simply sets an interal ivar.
  // It is propagated to a manipulator before the event
  // is sent to it.
  // Also changing the CenterOfRotation during interaction
  // i.e. after a button press but before a button up
  // has no effect until the next button press.
  vtkSetVector3Macro(CenterOfRotation, double);
  vtkGetVector3Macro(CenterOfRotation, double);

  // Description:
  // Do not let the superclass do anything with a char event.
  virtual void OnChar() {};

protected:
  vtkPVInteractorStyle();
  ~vtkPVInteractorStyle();

  vtkCameraManipulator *Current;
  double CenterOfRotation[3];

  // The CameraInteractors also store there button and modifier.
  vtkCollection *CameraManipulators;

  void OnButtonDown(int button, int shift, int control);
  void OnButtonUp(int button);
  void ResetLights();

  vtkPVInteractorStyle(const vtkPVInteractorStyle&); // Not implemented
  void operator=(const vtkPVInteractorStyle&); // Not implemented
};

#endif
