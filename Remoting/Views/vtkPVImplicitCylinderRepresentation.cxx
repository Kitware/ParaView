/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImplicitCylinderRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVImplicitCylinderRepresentation.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkPVImplicitCylinderRepresentation);

//----------------------------------------------------------------------------
vtkPVImplicitCylinderRepresentation::vtkPVImplicitCylinderRepresentation()
{
  vtkMultiProcessController* ctrl = NULL;
  ctrl = vtkMultiProcessController::GetGlobalController();
  double opacity = 1;
  if (ctrl == NULL || ctrl->GetNumberOfProcesses() == 1)
  {
    opacity = 0.25;
  }

  this->GetCylinderProperty()->SetOpacity(opacity);
  this->GetSelectedCylinderProperty()->SetOpacity(opacity);
}

//----------------------------------------------------------------------------
vtkPVImplicitCylinderRepresentation::~vtkPVImplicitCylinderRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkPVImplicitCylinderRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
