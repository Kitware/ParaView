/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTextWidgetDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTextWidgetDisplayProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMProxyProperty.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMTextWidgetDisplayProxy);
vtkCxxRevisionMacro(vtkSMTextWidgetDisplayProxy, "1.1");

//----------------------------------------------------------------------------
vtkSMTextWidgetDisplayProxy::vtkSMTextWidgetDisplayProxy()
{
  this->TextActorProxy = 0;
  this->TextPropertyProxy = 0;
  this->RenderModuleProxy = 0;
  this->Visibility = 0;
}

//----------------------------------------------------------------------------
vtkSMTextWidgetDisplayProxy::~vtkSMTextWidgetDisplayProxy()
{
  this->TextActorProxy = 0;
  this->TextPropertyProxy = 0;
  this->RenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMTextWidgetDisplayProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  this->Superclass::AddToRenderModule(rm); 
  
  this->RenderModuleProxy = rm;
}

//----------------------------------------------------------------------------
void vtkSMTextWidgetDisplayProxy::RemoveFromRenderModule(vtkSMRenderModuleProxy* rm)
{
  this->Superclass::RemoveFromRenderModule(rm);
  
  this->RenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMTextWidgetDisplayProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->TextActorProxy = this->GetSubProxy("Prop2DActor");
  if (!this->TextActorProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2DActor.");
    return;
    }
  this->TextPropertyProxy = this->GetSubProxy("Prop2DProperty");
  if (!this->TextPropertyProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2DProperty.");
    return;
    }

  this->TextActorProxy->SetServers(
    vtkProcessModule::RENDER_SERVER | vtkProcessModule::CLIENT);
  this->TextPropertyProxy->SetServers(
    vtkProcessModule::RENDER_SERVER | vtkProcessModule::CLIENT);

  this->Superclass::CreateVTKObjects(numObjects);

  if (!this->RepresentationProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2D.");
    return;
    }


  vtkSMProxyProperty* tppp = vtkSMProxyProperty::SafeDownCast(
    this->TextActorProxy->GetProperty("TextProperty"));
  if (!tppp)
    {
    vtkErrorMacro("Failed to find property TextProperty on TextActorProxy.");
    return;
    }
  if(!tppp->AddProxy(this->TextPropertyProxy))
    {
    return;
    }

  vtkSMProxyProperty* tapp = vtkSMProxyProperty::SafeDownCast(
    this->RepresentationProxy->GetProperty("TextActor"));
  if (!tapp)
    {
    vtkErrorMacro("Failed to find property TextActor on TextRepresentationProxy.");
    return;
    }
  if(!tapp->AddProxy(this->TextActorProxy))
    {
    return;
    }
}

//----------------------------------------------------------------------------
void vtkSMTextWidgetDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;

}
