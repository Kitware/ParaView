// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVLight.h"

#include "vtkInformation.h"
#include "vtkInformationStringKey.h"
#include "vtkObjectFactory.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayLightNode.h"
#endif

vtkInformationKeyMacro(vtkPVLight, LIGHT_NAME, String);

vtkStandardNewMacro(vtkPVLight);
//----------------------------------------------------------------------------
vtkPVLight::vtkPVLight() = default;

//----------------------------------------------------------------------------
vtkPVLight::~vtkPVLight() = default;

//----------------------------------------------------------------------------
void vtkPVLight::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVLight::SetRadius(double r)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayLightNode::SetRadius(r, this);
#else
  (void)r;
#endif
}

//----------------------------------------------------------------------------
void vtkPVLight::SetName(const std::string& name)
{
  GetInformation()->Set(vtkPVLight::LIGHT_NAME(), name);
}

//----------------------------------------------------------------------------
std::string vtkPVLight::GetName()
{
  if (GetInformation())
  {
    return GetInformation()->Get(vtkPVLight::LIGHT_NAME());
  }
  return "";
}

//----------------------------------------------------------------------------
void vtkPVLight::SetLightType(int t)
{
  if (t < VTK_LIGHT_TYPE_AMBIENT_LIGHT)
  {
    this->Superclass::SetLightType(t);
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
    vtkOSPRayLightNode::SetIsAmbient(0, this);
#endif
  }
  else
  {
    this->Superclass::SetLightType(VTK_LIGHT_TYPE_HEADLIGHT);
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
    vtkOSPRayLightNode::SetIsAmbient(1, this);
#endif
  }
}
