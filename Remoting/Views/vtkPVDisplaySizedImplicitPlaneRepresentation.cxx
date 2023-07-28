// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVDisplaySizedImplicitPlaneRepresentation.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkPVDisplaySizedImplicitPlaneRepresentation);

//----------------------------------------------------------------------------
vtkPVDisplaySizedImplicitPlaneRepresentation::vtkPVDisplaySizedImplicitPlaneRepresentation()
{
  vtkMultiProcessController* ctrl = nullptr;
  ctrl = vtkMultiProcessController::GetGlobalController();
  double opacity = 1;
  if (ctrl == nullptr || ctrl->GetNumberOfProcesses() == 1)
  {
    opacity = 0.25;
  }

  this->GetPlaneProperty()->SetOpacity(opacity);
  this->GetSelectedPlaneProperty()->SetOpacity(opacity);
}

//----------------------------------------------------------------------------
vtkPVDisplaySizedImplicitPlaneRepresentation::~vtkPVDisplaySizedImplicitPlaneRepresentation() =
  default;

//----------------------------------------------------------------------------
void vtkPVDisplaySizedImplicitPlaneRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
