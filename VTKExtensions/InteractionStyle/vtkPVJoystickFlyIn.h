// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVJoystickFlyIn
 * @brief   Rotates camera with xy mouse movement.
 *
 * vtkPVJoystickFlyIn allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 */

#ifndef vtkPVJoystickFlyIn_h
#define vtkPVJoystickFlyIn_h

#include "vtkPVJoystickFly.h"
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVJoystickFlyIn : public vtkPVJoystickFly
{
public:
  static vtkPVJoystickFlyIn* New();
  vtkTypeMacro(vtkPVJoystickFlyIn, vtkPVJoystickFly);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVJoystickFlyIn();
  ~vtkPVJoystickFlyIn() override;

private:
  vtkPVJoystickFlyIn(const vtkPVJoystickFlyIn&) = delete;
  void operator=(const vtkPVJoystickFlyIn&) = delete;
};

#endif
