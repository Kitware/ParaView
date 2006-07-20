/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDisplayProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMDisplayProxy);
vtkCxxRevisionMacro(vtkSMDisplayProxy, "1.11");

//-----------------------------------------------------------------------------
vtkSMDisplayProxy::vtkSMDisplayProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMDisplayProxy::~vtkSMDisplayProxy()
{
}
 
//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMDisplayProxy::GetInteractorProxy(vtkSMRenderModuleProxy* ren)
{
  return (ren?ren->GetInteractorProxy():0);
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMDisplayProxy::GetRendererProxy(vtkSMRenderModuleProxy* ren)
{
  return (ren?ren->GetRendererProxy():0);
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMDisplayProxy::GetRenderer2DProxy(vtkSMRenderModuleProxy* ren)
{
  return (ren?ren->GetRenderer2DProxy():0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::AddPropToRenderer(
  vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren)
{
  ren->AddPropToRenderer(proxy);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::AddPropToRenderer2D(
  vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren)
{
  ren->AddPropToRenderer2D(proxy);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::RemovePropFromRenderer(
  vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren)
{
  ren->RemovePropFromRenderer(proxy);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::RemovePropFromRenderer2D(
  vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren)
{
  ren->RemovePropFromRenderer2D(proxy);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  vtkSMProxy* p = this->GetSubProxy("Prop");
  if (p)
    {
    this->AddPropToRenderer(p, rm);
    }
  p = this->GetSubProxy("Prop2D");
  if (p)
    {
    this->AddPropToRenderer2D(p, rm);
    }
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::RemoveFromRenderModule(vtkSMRenderModuleProxy* rm)
{
  vtkSMProxy* p = this->GetSubProxy("Prop");
  if (p)
    {
    this->RemovePropFromRenderer(p, rm);
    }
  p = this->GetSubProxy("Prop2D");
  if (p)
    {
    this->RemovePropFromRenderer2D(p, rm);
    }
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
