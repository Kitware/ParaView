/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTRenderModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIceTRenderModuleProxy.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkPVOptions.h"

vtkStandardNewMacro(vtkSMIceTRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMIceTRenderModuleProxy, "1.1.2.2");

//-----------------------------------------------------------------------------
vtkSMIceTRenderModuleProxy::vtkSMIceTRenderModuleProxy()
{
  this->SetDisplayXMLName("MultiDisplay");
  this->RemoteDisplay = 0;
  // don't send locally rendered images back to the client.
}

//-----------------------------------------------------------------------------
vtkSMIceTRenderModuleProxy::~vtkSMIceTRenderModuleProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::InitializeCompositingPipeline()
{

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  int *tileDims =  pm->GetOptions()->GetTileDimensions();
  this->TileDimensions[0] = tileDims[0];
  this->TileDimensions[1] = tileDims[1];


  if (!getenv("PV_ICET_WINDOW_BORDERS"))
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->RenderWindowProxy->GetProperty("FullScreen"));
    if (ivp)
      {
      ivp->SetElement(0, 1);
      }
    else
      {
      vtkErrorMacro("Failed to find property FullScreen on RenderWindowProxy.");
      }
    }
  // TODO: full screen must be sent only to render servers. I am sending that to
  // client as well, but it doesn;t seem to have any effect on the client.
  // verify that it is fine.
  this->Superclass::InitializeCompositingPipeline();

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CompositeManagerProxy->GetProperty("UseCompositing"));
  if (ivp)
    {
    // In multi display mode, the server windows must be shown immediately.
    ivp->SetElement(0, 1); 
    }

  this->CompositeManagerProxy->UpdateVTKObjects();
}


//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::BeginStillRender()
{
  this->Superclass::BeginStillRender();
  // HACK to make the client use LOD when compositing.
  if (!this->LocalRender)
    {
    vtkPVProcessModule::SetGlobalLODFlagInternal(1);
    }
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::EndStillRender()
{
  if (!this->LocalRender)
    {
    vtkPVProcessModule::SetGlobalLODFlagInternal(0);
    }
  this->Superclass::EndStillRender();
  
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::BeginInteractiveRender()
{
  this->Superclass::BeginInteractiveRender();
  // Force LOD on the client when Compositing (but not using LOD).
  if (!this->LocalRender && !this->GetUseLODDecision())
    {
    vtkPVProcessModule::SetGlobalLODFlagInternal(1);
    }
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::EndInteractiveRender()
{
  // Reset force LOD on the client when Compositing (but not using LOD).
  if (!this->LocalRender && !this->GetUseLODDecision())
    {
    vtkPVProcessModule::SetGlobalLODFlagInternal(0);
    }
  this->Superclass::EndInteractiveRender();
}

//-----------------------------------------------------------------------------
int vtkSMIceTRenderModuleProxy::GetLocalRenderDecision(unsigned long mem, 
  int stillRender)
{
  if (!stillRender && this->GetUseLODDecision())
    {
    return 1; 
    }
  return this->Superclass::GetLocalRenderDecision(mem, stillRender);
}

//-----------------------------------------------------------------------------
void vtkSMIceTRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
