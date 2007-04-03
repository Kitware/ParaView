/*=========================================================================

  Program:   ParaView
  Module:    vtkSMConsumerDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMConsumerDisplayProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxyProperty.h"

int vtkSMConsumerDisplayProxy::UseCache = 0;

vtkCxxRevisionMacro(vtkSMConsumerDisplayProxy, "1.3");

//-----------------------------------------------------------------------------
vtkSMConsumerDisplayProxy::vtkSMConsumerDisplayProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMConsumerDisplayProxy::~vtkSMConsumerDisplayProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMConsumerDisplayProxy::SetUseCache(int useCache)
{
  vtkSMConsumerDisplayProxy::UseCache = useCache;
}

//-----------------------------------------------------------------------------
int vtkSMConsumerDisplayProxy::GetUseCache()
{
  return vtkSMConsumerDisplayProxy::UseCache;
}

//-----------------------------------------------------------------------------
void vtkSMConsumerDisplayProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  this->Superclass::MarkModified(modifiedProxy);

  // Do not invalidate geometry if MarkModified() was called by self.
  // A lot of the changes to the display proxy do not require
  // invalidating geometry. Those that do should call InvalidateGeometry()
  // explicitly.
  if (modifiedProxy != this)
    {
    this->InvalidateGeometryInternal(this->UseCache);
    }
}

//-----------------------------------------------------------------------------
void vtkSMConsumerDisplayProxy::InvalidateGeometry()
{
  this->InvalidateGeometryInternal(0);
}


//-----------------------------------------------------------------------------
void vtkSMConsumerDisplayProxy::MarkUpstreamModified()
{
  vtkSMProxy* current = this;
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    current->GetProperty("Input"));
  while (current && pp && pp->GetNumberOfProxies() > 0)
    {
    current = pp->GetProxy(0);
    pp = vtkSMProxyProperty::SafeDownCast(current->GetProperty("Input"));
    }

  if (current)
    {
    current->MarkModified(current);
    }
}

//-----------------------------------------------------------------------------
void vtkSMConsumerDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
