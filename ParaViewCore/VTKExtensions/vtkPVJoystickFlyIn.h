/*=========================================================================

  Program:   ParaView
  Module:    vtkPVJoystickFlyIn.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVJoystickFlyIn - Rotates camera with xy mouse movement.
// .SECTION Description
// vtkPVJoystickFlyIn allows the user to interactively
// manipulate the camera, the viewpoint of the scene.

#ifndef __vtkPVJoystickFlyIn_h
#define __vtkPVJoystickFlyIn_h

#include "vtkPVJoystickFly.h"

class VTK_EXPORT vtkPVJoystickFlyIn : public vtkPVJoystickFly
{
public:
  static vtkPVJoystickFlyIn *New();
  vtkTypeMacro(vtkPVJoystickFlyIn, vtkPVJoystickFly);
  void PrintSelf(ostream& os, vtkIndent indent);
  
protected:
  vtkPVJoystickFlyIn();
  ~vtkPVJoystickFlyIn();

private:
  vtkPVJoystickFlyIn(const vtkPVJoystickFlyIn&); // Not implemented
  void operator=(const vtkPVJoystickFlyIn&); // Not implemented
};

#endif
