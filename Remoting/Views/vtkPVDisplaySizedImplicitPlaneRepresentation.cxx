/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDisplaySizedImplicitPlaneRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
