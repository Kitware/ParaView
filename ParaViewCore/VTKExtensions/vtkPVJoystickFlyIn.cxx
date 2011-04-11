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

#include "vtkMath.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

vtkStandardNewMacro(vtkPVJoystickFlyIn);

//-------------------------------------------------------------------------
vtkPVJoystickFlyIn::vtkPVJoystickFlyIn()
{
  this->In = 1;
}

//-------------------------------------------------------------------------
vtkPVJoystickFlyIn::~vtkPVJoystickFlyIn()
{
}

//-------------------------------------------------------------------------
void vtkPVJoystickFlyIn::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}






