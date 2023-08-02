// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVLookingGlassSettings.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVLookingGlassSettings);

vtkPVLookingGlassSettings::vtkPVLookingGlassSettings() = default;

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
  os << indent << "ClippingLimits: " << this->ClippingLimits[0] << ", " << this->ClippingLimits[1]
     << "\n";
}

vtkCamera* vtkPVLookingGlassSettings::GetActiveCamera()
{
  if (this->View)
  {
    return this->View->GetActiveCamera();
  }

  return nullptr;
}
