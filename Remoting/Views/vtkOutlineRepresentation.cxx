// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOutlineRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkPVGeometryFilter.h"

vtkStandardNewMacro(vtkOutlineRepresentation);
//----------------------------------------------------------------------------
vtkOutlineRepresentation::vtkOutlineRepresentation()
{
  this->SetUseOutline(1);
  this->SetRepresentation(WIREFRAME);

  this->SetAmbient(1);
  this->SetDiffuse(0);
  this->SetSpecular(0);

  // you cannot select the outline!
  this->SetPickable(0);

  this->SetSuppressLOD(1);
}

//----------------------------------------------------------------------------
vtkOutlineRepresentation::~vtkOutlineRepresentation() = default;

//----------------------------------------------------------------------------
void vtkOutlineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
