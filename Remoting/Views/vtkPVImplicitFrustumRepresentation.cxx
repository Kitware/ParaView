// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVImplicitFrustumRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkPVImplicitFrustumRepresentation);

//----------------------------------------------------------------------------
vtkPVImplicitFrustumRepresentation::vtkPVImplicitFrustumRepresentation()
{
  this->GetFrustumProperty()->SetOpacity(0.25);
}
