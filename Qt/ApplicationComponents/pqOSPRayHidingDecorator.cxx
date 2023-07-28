// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqOSPRayHidingDecorator.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayPass.h"
#endif

//-----------------------------------------------------------------------------
pqOSPRayHidingDecorator::pqOSPRayHidingDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
}

//-----------------------------------------------------------------------------
pqOSPRayHidingDecorator::~pqOSPRayHidingDecorator() = default;

//-----------------------------------------------------------------------------
bool pqOSPRayHidingDecorator::canShowWidget(bool show_advanced) const
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  bool enableVisRTX = vtkOSPRayPass::IsBackendAvailable("optix pathtracer") &&
    vtksys::SystemTools::GetEnv("VTK_DISABLE_VISRTX") == nullptr;
  bool enableOSPRay = vtkOSPRayPass::IsBackendAvailable("OSPRay pathtracer") &&
    vtksys::SystemTools::GetEnv("VTK_DISABLE_OSPRAY") == nullptr;
  if (enableOSPRay || enableVisRTX)
  {
    return this->Superclass::canShowWidget(show_advanced);
  }
  else
#endif
  {
    (void)show_advanced;
    return false;
  }
}
