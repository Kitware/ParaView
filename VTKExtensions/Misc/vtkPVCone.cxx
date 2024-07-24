// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVCone.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVCone);

//------------------------------------------------------------------------------
vtkPVCone::vtkPVCone()
{
  this->IsDoubleCone = false;
}
