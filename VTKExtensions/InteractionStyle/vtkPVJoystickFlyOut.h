// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVJoystickFlyOut
 * @brief   Rotates camera with xy mouse movement.
 *
 * vtkPVJoystickFlyOut allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 */

#ifndef vtkPVJoystickFlyOut_h
#define vtkPVJoystickFlyOut_h

#include "vtkPVJoystickFly.h"
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVJoystickFlyOut : public vtkPVJoystickFly
{
public:
  static vtkPVJoystickFlyOut* New();
  vtkTypeMacro(vtkPVJoystickFlyOut, vtkPVJoystickFly);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVJoystickFlyOut();
  ~vtkPVJoystickFlyOut() override;

private:
  vtkPVJoystickFlyOut(const vtkPVJoystickFlyOut&) = delete;
  void operator=(const vtkPVJoystickFlyOut&) = delete;
};

#endif
