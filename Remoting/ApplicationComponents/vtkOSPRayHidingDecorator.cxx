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
void vtkOSPRayHidingDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkOSPRayHidingDecorator::CanShow([[maybe_unused]] bool showAdvanced) const
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  bool enableVisRTX = vtkOSPRayPass::IsBackendAvailable("optix pathtracer") &&
    vtksys::SystemTools::GetEnv("VTK_DISABLE_VISRTX") == nullptr;
  bool enableOSPRay = vtkOSPRayPass::IsBackendAvailable("OSPRay pathtracer") &&
    vtksys::SystemTools::GetEnv("VTK_DISABLE_OSPRAY") == nullptr;
  if (enableOSPRay || enableVisRTX)
  {
    return this->Superclass::CanShow(showAdvanced);
  }
  else
  {
    return false;
  }
#else
  return false;
#endif
}
