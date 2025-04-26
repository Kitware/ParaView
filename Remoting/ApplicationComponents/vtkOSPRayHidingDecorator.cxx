// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOSPRayHidingDecorator.h"
#include "vtkObjectFactory.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayPass.h"
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayHidingDecorator);

//-----------------------------------------------------------------------------
vtkOSPRayHidingDecorator::vtkOSPRayHidingDecorator() = default;

//-----------------------------------------------------------------------------
vtkOSPRayHidingDecorator::~vtkOSPRayHidingDecorator() = default;

//-----------------------------------------------------------------------------
void vtkOSPRayHidingDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkOSPRayHidingDecorator::CanShow(bool show_advanced) const
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  const bool enableVisRTX = vtkOSPRayPass::IsBackendAvailable("optix pathtracer") &&
    vtksys::SystemTools::GetEnv("VTK_DISABLE_VISRTX") == nullptr;
  const bool enableOSPRay = vtkOSPRayPass::IsBackendAvailable("OSPRay pathtracer") &&
    vtksys::SystemTools::GetEnv("VTK_DISABLE_OSPRAY") == nullptr;
  if (enableOSPRay || enableVisRTX)
  {
    return this->Superclass::CanShow(show_advanced);
  }
  else
#endif
  {
    (void)show_advanced;
    return false;
  }
}
