// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVImplicitAnnulusRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkPVImplicitAnnulusRepresentation);

//----------------------------------------------------------------------------
vtkPVImplicitAnnulusRepresentation::vtkPVImplicitAnnulusRepresentation()
{
  double opacity = 0.25;
  this->GetAnnulusProperty()->SetOpacity(opacity);
  this->GetSelectedAnnulusProperty()->SetOpacity(opacity);
}
