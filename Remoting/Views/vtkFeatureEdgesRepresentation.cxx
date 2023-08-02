// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFeatureEdgesRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkPVGeometryFilter.h"

vtkStandardNewMacro(vtkFeatureEdgesRepresentation);
//----------------------------------------------------------------------------
vtkFeatureEdgesRepresentation::vtkFeatureEdgesRepresentation()
{
  this->SetUseOutline(0);
  this->SetGenerateFeatureEdges(true);
  this->SetRepresentation(WIREFRAME);

  this->SetAmbient(1);
  this->SetDiffuse(0);
  this->SetSpecular(0);

  // you cannot select the outline!
  this->SetPickable(0);

  this->SetSuppressLOD(1);
}

//----------------------------------------------------------------------------
vtkFeatureEdgesRepresentation::~vtkFeatureEdgesRepresentation() = default;

//----------------------------------------------------------------------------
void vtkFeatureEdgesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
