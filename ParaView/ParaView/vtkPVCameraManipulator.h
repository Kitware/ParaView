/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraManipulator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCameraManipulator - Abstraction of style away from button.
// .SECTION Description
// vtkPVCameraManipulator is a superclass foractions inside an
// interactor style and associated with a single button. An example
// might be rubber-band bounding-box zoom. This abstraction allows a
// camera manipulator to be assigned to any button.  This super class
// might become a subclass of vtkInteractorObserver in the future.

#ifndef __vtkPVCameraManipulator_h
#define __vtkPVCameraManipulator_h

#include "vtkObject.h"

class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkPVApplication;

class VTK_EXPORT vtkPVCameraManipulator : public vtkObject
{
public:
  static vtkPVCameraManipulator *New();
  vtkTypeRevisionMacro(vtkPVCameraManipulator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void StartInteraction();
  virtual void EndInteraction();

  virtual void OnMouseMove(int x, int y, vtkRenderer *ren,
                           vtkRenderWindowInteractor *iren);
  virtual void OnButtonDown(int x, int y, vtkRenderer *ren,
                            vtkRenderWindowInteractor *iren);
  virtual void OnButtonUp(int x, int y, vtkRenderer *ren,
                          vtkRenderWindowInteractor *iren);
  
  // Description:
  // These settings determine which button and modifiers the
  // manipulator responds to. Button can be either 1 (left), 2
  // (middle), and 3 right.
  vtkSetMacro(Button, int);
  vtkGetMacro(Button, int);
  vtkSetMacro(Shift, int);
  vtkGetMacro(Shift, int);
  vtkBooleanMacro(Shift, int);
  vtkSetMacro(Control, int);
  vtkGetMacro(Control, int);
  vtkBooleanMacro(Control, int);

  // Description:
  // For setting the center of rotation.
  vtkSetVector3Macro(Center, float);
  vtkGetVector3Macro(Center, float);

  // Description:
  // In order to make calls on the application, we need a pointer to
  // it.
  void SetApplication(vtkPVApplication*);
  vtkGetObjectMacro(Application, vtkPVApplication);

  // Description:
  // Set and get the manipulator name.
  vtkSetStringMacro(ManipulatorName);
  vtkGetStringMacro(ManipulatorName);

protected:
  vtkPVCameraManipulator();
  ~vtkPVCameraManipulator();

  void ResetLights();

  char* ManipulatorName;

  int Button;
  int Shift;
  int Control;

  int LastX;
  int LastY;

  float Center[3];
  float DisplayCenter[2];
  void ComputeDisplayCenter(vtkRenderer *ren);

  vtkPVApplication *Application;

private:
  vtkPVCameraManipulator(const vtkPVCameraManipulator&); // Not implemented
  void operator=(const vtkPVCameraManipulator&); // Not implemented
};

#endif
