// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVImplicitCylinderRepresentation.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkPVImplicitCylinderRepresentation);

//----------------------------------------------------------------------------
vtkPVImplicitCylinderRepresentation::vtkPVImplicitCylinderRepresentation()
{
  vtkMultiProcessController* ctrl = nullptr;
  ctrl = vtkMultiProcessController::GetGlobalController();
  double opacity = 1;
  if (ctrl == nullptr || ctrl->GetNumberOfProcesses() == 1)
  {
    opacity = 0.25;
  }

  this->GetCylinderProperty()->SetOpacity(opacity);
  this->GetSelectedCylinderProperty()->SetOpacity(opacity);
}

//----------------------------------------------------------------------------
vtkPVImplicitCylinderRepresentation::~vtkPVImplicitCylinderRepresentation() = default;

//----------------------------------------------------------------------------
void vtkPVImplicitCylinderRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
