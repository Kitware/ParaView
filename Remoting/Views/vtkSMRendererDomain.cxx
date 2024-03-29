// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSMRendererDomain.h"
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayPass.h"
#endif
#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMRendererDomain);

//---------------------------------------------------------------------------
void vtkSMRendererDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
int vtkSMRendererDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  int retVal = this->Superclass::ReadXMLAttributes(prop, element);
  if (!retVal)
  {
    return 0;
  }
  // throw away whatever XML said our strings are and call update instead
  this->Update(nullptr);
#else
  (void)prop;
  (void)element;
#endif
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMRendererDomain::Update(vtkSMProperty* vtkNotUsed(prop))
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing

  // populate my list
  std::vector<std::string> sa;
  if (vtkOSPRayPass::IsBackendAvailable("OSPRay raycaster"))
  {
    sa.push_back(std::string("OSPRay raycaster"));
  }
  if (vtkOSPRayPass::IsBackendAvailable("OSPRay pathtracer"))
  {
    sa.push_back(std::string("OSPRay pathtracer"));
  }
  if (vtkOSPRayPass::IsBackendAvailable("OptiX pathtracer"))
  {
    sa.push_back(std::string("OptiX pathtracer"));
  }

  this->SetStrings(sa);
#endif
}
