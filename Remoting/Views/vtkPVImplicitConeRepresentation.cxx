// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVImplicitConeRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkPVImplicitConeRepresentation);

//----------------------------------------------------------------------------
vtkPVImplicitConeRepresentation::vtkPVImplicitConeRepresentation()
{
  double opacity = 0.25;
  this->GetConeProperty()->SetOpacity(opacity);
  this->GetSelectedConeProperty()->SetOpacity(opacity);
}
