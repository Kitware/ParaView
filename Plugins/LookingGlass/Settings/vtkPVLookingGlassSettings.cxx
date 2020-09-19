/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLookingGlassSettings.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLookingGlassSettings.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVLookingGlassSettings);

vtkPVLookingGlassSettings::vtkPVLookingGlassSettings()
{
}

vtkPVLookingGlassSettings::~vtkPVLookingGlassSettings()
{
  this->SetView(nullptr);
}

void vtkPVLookingGlassSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FocalPlaneMovementFactor: " << this->FocalPlaneMovementFactor << "\n";
  os << indent << "DeviceIndex: " << this->DeviceIndex << "\n";
  os << indent << "RenderRate: " << this->RenderRate << "\n";
  os << indent << "NearClippingLimit: " << this->NearClippingLimit << "\n";
  os << indent << "FarClippingLimit: " << this->FarClippingLimit << "\n";
}

vtkCamera* vtkPVLookingGlassSettings::GetActiveCamera()
{
  if (this->View)
  {
    return this->View->GetActiveCamera();
  }

  return nullptr;
}
