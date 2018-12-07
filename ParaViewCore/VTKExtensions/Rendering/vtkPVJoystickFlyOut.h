/*=========================================================================

  Program:   ParaView
  Module:    vtkPVJoystickFlyOut.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVJoystickFlyOut : public vtkPVJoystickFly
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
