/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTDesktopRenderModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIceTDesktopRenderModuleProxy.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkPVOptions.h"
#include "vtkRenderWindow.h"

vtkStandardNewMacro(vtkSMIceTDesktopRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMIceTDesktopRenderModuleProxy, "1.2");

//-----------------------------------------------------------------------------
vtkSMIceTDesktopRenderModuleProxy::vtkSMIceTDesktopRenderModuleProxy()
{
  this->TileDimensions[0] = this->TileDimensions[1] = 1;
  this->RemoteDisplay = 1;
}

//-----------------------------------------------------------------------------
vtkSMIceTDesktopRenderModuleProxy::~vtkSMIceTDesktopRenderModuleProxy()
{

}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->RendererProxy = this->GetSubProxy("Renderer");
  this->DisplayManagerProxy = this->GetSubProxy("DisplayManager");

  if (!this->RendererProxy)
    {
    vtkErrorMacro("Renderer subproxy must be defined.");
    return;
    }

  if (!this->DisplayManagerProxy)
    {
    vtkErrorMacro("DisplayManager subproxy must be defined.");
    return;
    }


  this->DisplayManagerProxy->SetServers(vtkProcessModule::RENDER_SERVER);
  this->DisplayManagerProxy->UpdateVTKObjects();

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());

  // We must create an ICE-T Renderer on the server and a regular renderer on the
  // client.
  this->RendererProxy->SetServers(vtkProcessModule::CLIENT);
  this->RendererProxy->UpdateVTKObjects(); // this will create the regular renderer on the client.

  vtkClientServerStream stream;
  stream << vtkClientServerStream::New << "vtkIceTRenderer" << this->RendererProxy->GetID(0)
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);

  this->RendererProxy->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  // Now we can use the RendererProxy as one!

  
  this->Superclass::CreateVTKObjects(numObjects);

  // Anti-aliasing screws up the compositing.  Turn it off.
  if (this->GetRenderWindow()->IsA("vtkOpenGLRenderWindow") &&
    (pm->GetNumberOfPartitions() > 1))
    {
    stream << vtkClientServerStream::Invoke
      << this->RenderWindowProxy->GetID(0) << "SetMultiSamples" << 0
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::CreateCompositeManager()
{
  this->CompositeManagerProxy = this->GetSubProxy("CompositeManager");
  if (!this->CompositeManagerProxy)
    {
    vtkErrorMacro("CompositeManager subproxy must be defined.");
    return;
    }

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  vtkClientServerStream stream;
  // Similar to the renderer, we create our peculiar CompositeManager.
  this->CompositeManagerProxy->SetServers(vtkProcessModule::CLIENT);
  // The XML must define only the manager to be created on the client.

  this->CompositeManagerProxy->UpdateVTKObjects();
  // Create the vtkDesktopDeliveryServer.
  stream << vtkClientServerStream::New << "vtkDesktopDeliveryServer"
    << this->CompositeManagerProxy->GetID(0)
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT, stream);

  this->CompositeManagerProxy->SetServers(vtkProcessModule::CLIENT | 
    vtkProcessModule::RENDER_SERVER_ROOT);
}

//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::InitializeCompositingPipeline()
{
  vtkSMIntVectorProperty* ivp;
  vtkSMProxyProperty* pp;

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->DisplayManagerProxy->GetProperty("TileDimensions"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property TileDimensions on DisplayManagerProxy.");
    return;
    }
  ivp->SetElements(this->TileDimensions);
  this->DisplayManagerProxy->UpdateVTKObjects(); 
  // Tile Dimensions must be set before the RenderWindow is set,
  // hence the call to DisplayManagerProxy->UpdateVTKObjects()  is required.
  // I know, calling UpdateVTKObjects so many times is waaaay expensive,
  // but hey! This is called only during creating.
  
  pp = vtkSMProxyProperty::SafeDownCast(
    this->DisplayManagerProxy->GetProperty("RenderWindow"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property RenderWindow on DisplayManagerProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->RenderWindowProxy);
  this->DisplayManagerProxy->UpdateVTKObjects(); 

  vtkClientServerStream stream;
  unsigned int i;

  for (i=0; i < this->DisplayManagerProxy->GetNumberOfIDs(); i++)
    {
    stream << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID()
      << "GetController"
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << this->DisplayManagerProxy->GetID(i) << "SetController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << this->DisplayManagerProxy->GetID(i) 
      << "InitializeRMIs"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
    }
  //************************************************************
  // Clean up this mess !!!!!!!!!!!!!
  // Even a cast to vtkPVClientServerModule would be better than this.
  // How can we syncronize the process modules and render modules?
  for (i=0; i < this->CompositeManagerProxy->GetNumberOfIDs(); i++)
    {
    stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
      << "GetRenderServerSocketController"
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
      << this->CompositeManagerProxy->GetID(i)
      << "SetController" << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  pm->SendStream(
    vtkProcessModule::RENDER_SERVER_ROOT | vtkProcessModule::CLIENT, stream);
  
  this->Superclass::InitializeCompositingPipeline();
  
  
  for (i=0; i < this->CompositeManagerProxy->GetNumberOfIDs(); i++)
    {
    // The client server manager needs to set parameters on the IceT manager.
    stream << vtkClientServerStream::Invoke << this->CompositeManagerProxy->GetID(i)
      << "SetParallelRenderManager" << this->DisplayManagerProxy->GetID(i)
      << vtkClientServerStream::End;

    stream << vtkClientServerStream::Invoke << this->CompositeManagerProxy->GetID(i)
      << "SetRemoteDisplay" << this->RemoteDisplay << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::RENDER_SERVER_ROOT, stream);

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkSMIceTDesktopRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TileDimensions: " << this->TileDimensions[0] 
    << ", " << this->TileDimensions[1] << endl;
  os << indent << "RemoteDisplay: " << this->RemoteDisplay 
    << endl;
  os << indent << "DisplayManagerProxy: " << this->DisplayManagerProxy
    <<endl;
}

