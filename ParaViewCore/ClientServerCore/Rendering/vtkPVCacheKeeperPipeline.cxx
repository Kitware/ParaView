/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCacheKeeperPipeline.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCacheKeeperPipeline.h"

#include "vtkObjectFactory.h"
#include "vtkPVCacheKeeper.h"

vtkStandardNewMacro(vtkPVCacheKeeperPipeline);
//----------------------------------------------------------------------------
vtkPVCacheKeeperPipeline::vtkPVCacheKeeperPipeline()
{
}

//----------------------------------------------------------------------------
vtkPVCacheKeeperPipeline::~vtkPVCacheKeeperPipeline()
{
}

//----------------------------------------------------------------------------
int vtkPVCacheKeeperPipeline::ForwardUpstream(int i, int j, vtkInformation* request)
{
  vtkPVCacheKeeper* keeper = vtkPVCacheKeeper::SafeDownCast(this->Algorithm);
  if (keeper && keeper->GetCachingEnabled() && keeper->IsCached())
  {
    // shunt upstream updates when using cache.
    return 1;
  }

  return this->Superclass::ForwardUpstream(i, j, request);
}

//----------------------------------------------------------------------------
int vtkPVCacheKeeperPipeline::ForwardUpstream(vtkInformation* request)
{
  vtkPVCacheKeeper* keeper = vtkPVCacheKeeper::SafeDownCast(this->Algorithm);
  if (keeper && keeper->GetCachingEnabled() && keeper->IsCached())
  {
    // shunt upstream updates when using cache.
    return 1;
  }

  return this->Superclass::ForwardUpstream(request);
}

//----------------------------------------------------------------------------
void vtkPVCacheKeeperPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
