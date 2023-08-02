// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVJoystickFlyOut.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVJoystickFlyOut);

//-------------------------------------------------------------------------
vtkPVJoystickFlyOut::vtkPVJoystickFlyOut()
{
  this->In = 0;
}

//-------------------------------------------------------------------------
vtkPVJoystickFlyOut::~vtkPVJoystickFlyOut() = default;

//-------------------------------------------------------------------------
void vtkPVJoystickFlyOut::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
