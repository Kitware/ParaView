/*=========================================================================

  Program:   ParaView
  Module:    vtkPVJoystickFlyIn.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVJoystickFlyIn.h"

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVJoystickFlyIn);

//-------------------------------------------------------------------------
vtkPVJoystickFlyIn::vtkPVJoystickFlyIn()
{
  this->In = 1;
}

//-------------------------------------------------------------------------
vtkPVJoystickFlyIn::~vtkPVJoystickFlyIn() = default;

//-------------------------------------------------------------------------
void vtkPVJoystickFlyIn::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
