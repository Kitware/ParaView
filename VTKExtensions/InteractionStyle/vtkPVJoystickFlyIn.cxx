// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
