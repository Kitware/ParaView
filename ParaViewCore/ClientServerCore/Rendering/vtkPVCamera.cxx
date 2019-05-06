/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCamera.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCamera.h"

#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayCameraNode.h"
#endif

vtkStandardNewMacro(vtkPVCamera);
//----------------------------------------------------------------------------
vtkPVCamera::vtkPVCamera()
{
}

//----------------------------------------------------------------------------
vtkPVCamera::~vtkPVCamera()
{
}

//----------------------------------------------------------------------------
void vtkPVCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVCamera::SetDepthOfField(int dof)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayCameraNode::SetDepthOfField(dof, this);
#else
  (void)dof;
#endif
}

//----------------------------------------------------------------------------
int vtkPVCamera::GetDepthOfField()
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  return vtkOSPRayCameraNode::GetDepthOfField(this);
#else
  return 0;
#endif
}
