/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientServerRenderSyncManagerHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMClientServerRenderSyncManagerHelper.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVServerInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"

//----------------------------------------------------------------------------
vtkSMClientServerRenderSyncManagerHelper::vtkSMClientServerRenderSyncManagerHelper()
{
}

//----------------------------------------------------------------------------
vtkSMClientServerRenderSyncManagerHelper::~vtkSMClientServerRenderSyncManagerHelper()
{
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderSyncManagerHelper::CreateRenderWindow(vtkSMProxy* renWinProxy, 
  vtkClientServerID sharedServerRenderWindowID)
{
  if (!renWinProxy)
    {
    vtkGenericWarningMacro("RenderWindow proxy must be defined.");
    return;
    }

  if (renWinProxy->GetObjectsCreated())
    {
    vtkGenericWarningMacro("RenderWindow is already created.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  if (!sharedServerRenderWindowID.IsNull())
    {
    renWinProxy->SetServers(vtkProcessModule::CLIENT);
    renWinProxy->UpdateVTKObjects();

    stream  << vtkClientServerStream::Assign
            << renWinProxy->GetID()
            << sharedServerRenderWindowID 
            << vtkClientServerStream::End;
    pm->SendStream(renWinProxy->GetConnectionID(), 
      vtkProcessModule::RENDER_SERVER, stream);
    }

  renWinProxy->SetServers(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}


//----------------------------------------------------------------------------
void vtkSMClientServerRenderSyncManagerHelper::CreateRenderSyncManager(
  vtkSMProxy* rsmProxy,
  vtkClientServerID sharedServerRSMID,
  const char* rsmServerClassName)
{
  if (rsmProxy->GetObjectsCreated())
    {
    vtkGenericWarningMacro("RenderSyncManager already created.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  // XML Configuration defines the client side class for RenderSyncManager.
  rsmProxy->SetServers(vtkProcessModule::CLIENT);
  rsmProxy->UpdateVTKObjects();
  // This will create the client side  vtkPVDesktopDeliveryClient.
  
  if (!sharedServerRSMID.IsNull())
    {
    stream  << vtkClientServerStream::Assign
            << rsmProxy->GetID()
            << sharedServerRSMID 
            << vtkClientServerStream::End;
    }
  else
    {
    stream  << vtkClientServerStream::New
            << rsmServerClassName
            << rsmProxy->GetID()
            << vtkClientServerStream::End;
    }
  pm->SendStream(rsmProxy->GetConnectionID(), 
    vtkProcessModule::RENDER_SERVER_ROOT, stream);

  rsmProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER_ROOT);
}


//----------------------------------------------------------------------------
void vtkSMClientServerRenderSyncManagerHelper::InitializeRenderSyncManager(
  vtkSMProxy* rsmProxy, vtkSMProxy* renderWindowProxy)
{
  vtkSMIntVectorProperty* ivp;
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkIdType cid = rsmProxy->GetConnectionID();

  // Synchronize the environment among all the render server nodes.
  vtkPVServerInformation* serverInfo = pm->GetServerInformation(cid);
  unsigned int idx;
  unsigned int numMachines = serverInfo->GetNumberOfMachines();
  for (idx = 0; idx < numMachines; idx++)
    {
    if (serverInfo->GetEnvironment(idx))
      {
      stream  << vtkClientServerStream::Invoke 
              << pm->GetProcessModuleID() 
              << "SetProcessEnvironmentVariable" 
              << idx 
              << serverInfo->GetEnvironment(idx)
              << vtkClientServerStream::End;
      }
    }
  pm->SendStream(cid, vtkProcessModule::RENDER_SERVER, stream);

  // RenderSyncManager needs access to the socket controller between the client 
  // and the render server root. This is the socket connection that is used by
  // the vtkPVDesktopDeliveryClient and vtkPVDesktopDeliveryServer to
  // communicate. So, set that up.
  stream  << vtkClientServerStream::Invoke 
          << pm->GetProcessModuleID()
          << "GetActiveRemoteConnection"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke 
          << rsmProxy->GetID()
          << "Initialize" 
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(cid, rsmProxy->GetServers(), stream);

  // In case we are using MultiView support, the server side RenderWindow may
  // have more renderers than this view module has (since all view modules share
  // the server side render windows). In that case, we don't want to sync the
  // render window renderers.
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    rsmProxy->GetProperty("SyncRenderWindowRenderers"));
  if (!ivp)
    {
    vtkGenericWarningMacro("Falied to find property SyncRenderWindowRenderers");
    return;
    }
  ivp->SetElement(0, 0);
  rsmProxy->UpdateVTKObjects();

  // Setup RMI callbacks. 
  // FIXME: Make InitializeRMIs idempotent.
  // rsmProxy->InvokeCommand("InitializeRMIs");

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rsmProxy->GetProperty("RenderWindow"));
  pp->RemoveAllProxies();
  pp->AddProxy(renderWindowProxy);

  // Force compressor setting from client to server. This is needed when loading
  // from state.
  vtkSMStringVectorProperty* svp =
      dynamic_cast<vtkSMStringVectorProperty*>(rsmProxy->GetProperty("CompressorConfig"));
  vtkstd::string sv(svp->GetElement(0));
  svp->SetElement(0,"NULL");
  svp->SetElement(0,sv.c_str());

  ivp=dynamic_cast<vtkSMIntVectorProperty*>(rsmProxy->GetProperty("CompressionEnabled"));
  int iv=ivp->GetElement(0);
  ivp->SetElement(0,-1);
  ivp->SetElement(0,iv);

  // Update the server process so that the render window is set before
  // we initialize offscreen rendering.
  rsmProxy->UpdateVTKObjects();

  if (getenv("PV_DISABLE_COMPOSITE_INTERRUPTS"))
    {
    // Does anything support EnableAbort right now?
    rsmProxy->InvokeCommand("EnableAbort");
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    rsmProxy->GetProperty("UseCompositing"));
  if (ivp)
    {
    // So that the server window does not popup until needed.
    ivp->SetElement(0, 0); 
    }

  rsmProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderSyncManagerHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


