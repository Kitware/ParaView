/*=========================================================================

  Program:   ParaView
  Module:    vtkPVJoystickFly.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVJoystickFly - Fly camera towards or away from the object
// .SECTION Description
// vtkPVJoystickFly allows the user to interactively manipulate the
// camera, the viewpoint of the scene.

#ifndef __vtkPVJoystickFly_h
#define __vtkPVJoystickFly_h

#include "vtkPVCameraManipulator.h"

class vtkRenderer;

class VTK_EXPORT vtkPVJoystickFly : public vtkPVCameraManipulator
{
public:
  vtkTypeRevisionMacro(vtkPVJoystickFly, vtkPVCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove(int x, int y, vtkRenderer *ren,
                           vtkRenderWindowInteractor *rwi);
  virtual void OnButtonDown(int x, int y, vtkRenderer *ren,
                            vtkRenderWindowInteractor *rwi);
  virtual void OnButtonUp(int x, int y, vtkRenderer *ren,
                          vtkRenderWindowInteractor *rwi);

  // Description:
  // Set and get the speed of flying.
  //virtual void SetFlySpeed(double);
  vtkSetClampMacro(FlySpeed, double, 1, 30);  
  vtkGetMacro(FlySpeed, double);  

protected:
  vtkPVJoystickFly();
  ~vtkPVJoystickFly();

  int In;
  int FlyFlag;

  double FlySpeed;
  double Scale;
  double LastRenderTime;
  double CameraXAxis[3];
  double CameraYAxis[3];
  double CameraZAxis[3];

  void Fly(vtkRenderer* ren, vtkRenderWindowInteractor *rwi, 
           float scale, float speed);
  void ComputeCameraAxes(vtkRenderer*);

  vtkPVJoystickFly(const vtkPVJoystickFly&); // Not implemented
  void operator=(const vtkPVJoystickFly&); // Not implemented
};

#endif
