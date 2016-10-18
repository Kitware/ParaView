/*=========================================================================

  Program:   ParaView
  Module:    vtkPVJoystickFlyOut.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVJoystickFlyOut.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVJoystickFlyOut);

//-------------------------------------------------------------------------
vtkPVJoystickFlyOut::vtkPVJoystickFlyOut()
{
  this->In = 0;
}

//-------------------------------------------------------------------------
vtkPVJoystickFlyOut::~vtkPVJoystickFlyOut()
{
}

//-------------------------------------------------------------------------
void vtkPVJoystickFlyOut::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
