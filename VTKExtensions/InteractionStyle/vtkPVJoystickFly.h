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
/**
 * @class   vtkPVJoystickFly
 * @brief   Fly camera towards or away from the object
 *
 * vtkPVJoystickFly allows the user to interactively manipulate the
 * camera, the viewpoint of the scene.
*/

#ifndef vtkPVJoystickFly_h
#define vtkPVJoystickFly_h

#include "vtkCameraManipulator.h"
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro

class vtkRenderer;

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVJoystickFly : public vtkCameraManipulator
{
public:
  vtkTypeMacro(vtkPVJoystickFly, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  //@}

  //@{
  /**
   * Set and get the speed of flying.
   */
  vtkSetClampMacro(FlySpeed, double, 1, 30);
  vtkGetMacro(FlySpeed, double);
  //@}

protected:
  vtkPVJoystickFly();
  ~vtkPVJoystickFly() override;

  int In;
  int FlyFlag;

  double FlySpeed;
  double Scale;
  double LastRenderTime;
  double CameraXAxis[3];
  double CameraYAxis[3];
  double CameraZAxis[3];

  void Fly(vtkRenderer* ren, vtkRenderWindowInteractor* rwi, double scale, double speed);
  void ComputeCameraAxes(vtkRenderer*);

  vtkPVJoystickFly(const vtkPVJoystickFly&) = delete;
  void operator=(const vtkPVJoystickFly&) = delete;
};

#endif
