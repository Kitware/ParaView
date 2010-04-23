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
// .NAME vtkPVJoystickFlyOut - Rotates camera with xy mouse movement.
// .SECTION Description
// vtkPVJoystickFlyOut allows the user to interactively
// manipulate the camera, the viewpoint of the scene.

#ifndef __vtkPVJoystickFlyOut_h
#define __vtkPVJoystickFlyOut_h

#include "vtkPVJoystickFly.h"

class VTK_EXPORT vtkPVJoystickFlyOut : public vtkPVJoystickFly
{
public:
  static vtkPVJoystickFlyOut *New();
  vtkTypeMacro(vtkPVJoystickFlyOut, vtkPVJoystickFly);
  void PrintSelf(ostream& os, vtkIndent indent);
  
protected:
  vtkPVJoystickFlyOut();
  ~vtkPVJoystickFlyOut();

private:
  vtkPVJoystickFlyOut(const vtkPVJoystickFlyOut&); // Not implemented
  void operator=(const vtkPVJoystickFlyOut&); // Not implemented
};

#endif
