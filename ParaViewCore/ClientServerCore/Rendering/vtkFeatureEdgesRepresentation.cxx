/*=========================================================================

  Program:   ParaView
  Module:    vtkFeatureEdgesRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
vtkFeatureEdgesRepresentation::~vtkFeatureEdgesRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkFeatureEdgesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
