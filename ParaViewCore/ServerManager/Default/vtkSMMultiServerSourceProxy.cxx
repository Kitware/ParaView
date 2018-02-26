/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiServerSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMultiServerSourceProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVMultiServerDataSource.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"

#include <assert.h>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMMultiServerSourceProxy);
//---------------------------------------------------------------------------
vtkSMMultiServerSourceProxy::vtkSMMultiServerSourceProxy()
{
  this->RemoteProxySessionID = this->RemoteProxyID = 0;
}

//---------------------------------------------------------------------------
vtkSMMultiServerSourceProxy::~vtkSMMultiServerSourceProxy()
{
}
//---------------------------------------------------------------------------
void vtkSMMultiServerSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
void vtkSMMultiServerSourceProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  // Notify the VTK object as well
  vtkPVMultiServerDataSource* clientObj =
    vtkPVMultiServerDataSource::SafeDownCast(this->GetClientSideObject());
  clientObj->Modified();

  // Propagate the dirty flag as regular proxy
  this->Superclass::MarkDirty(modifiedProxy);
}
//---------------------------------------------------------------------------
void vtkSMMultiServerSourceProxy::SetExternalProxy(
  vtkSMSourceProxy* proxyFromAnotherServer, int port)
{
  vtkPVMultiServerDataSource* clientObj =
    vtkPVMultiServerDataSource::SafeDownCast(this->GetClientSideObject());

  clientObj->SetExternalProxy(proxyFromAnotherServer, port);

  // Remove previous proxy dependency
  vtkSMSourceProxy* previousRemoteProxy = this->GetExternalProxy();
  if (previousRemoteProxy != NULL)
  {
    previousRemoteProxy->RemoveConsumer(this->GetProperty("DependencyLink"), this);
  }

  // Store data information
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  this->RemoteProxySessionID = pm->GetSessionID(proxyFromAnotherServer->GetSession());
  this->RemoteProxyID = proxyFromAnotherServer->GetGlobalID();
  this->PortToExport = port;

  // Create dependency
  proxyFromAnotherServer->AddConsumer(this->GetProperty("DependencyLink"), this);

  // Push our new state to the session
  this->UpdateState();

  // Mark dirty
  this->MarkDirty(proxyFromAnotherServer);
}
//---------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMMultiServerSourceProxy::GetExternalProxy()
{
  vtkSMSourceProxy* bindedProxy = NULL;
  vtkSMSession* remoteSession = vtkSMSession::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetSession(this->RemoteProxySessionID));
  if (remoteSession)
  {
    bindedProxy = vtkSMSourceProxy::SafeDownCast(
      remoteSession->GetProxyLocator()->LocateProxy(this->RemoteProxyID));
  }
  return bindedProxy;
}

//---------------------------------------------------------------------------
void vtkSMMultiServerSourceProxy::UpdateState()
{
  // ensure that the state is created correctly.
  this->CreateVTKObjects();

  // Make sure we update our local state with our internal data
  // push current state.
  this->State->ClearExtension(ProxyState::user_data);

  ProxyState_UserData* user_data = this->State->AddExtension(ProxyState::user_data);
  user_data->set_key("RemoteProxyInfo");

  Variant* variant = user_data->add_variant();
  variant->set_type(Variant::IDTYPE); // type is just arbitrary here.
  variant->add_idtype(this->RemoteProxySessionID);
  variant->add_idtype(this->RemoteProxyID);
  variant->add_integer(this->PortToExport);

  // Debug
  // this->State->PrintDebugString();

  this->PushState(this->State);
}

//---------------------------------------------------------------------------
void vtkSMMultiServerSourceProxy::LoadState(const vtkSMMessage* message, vtkSMProxyLocator* locator)
{
  this->Superclass::LoadState(message, locator);

  // Extract
  if (message->ExtensionSize(ProxyState::user_data) != 1)
  {
    return;
  }

  const ProxyState_UserData& user_data = message->GetExtension(ProxyState::user_data, 0);
  if (user_data.key() != "RemoteProxyInfo")
  {
    // vtkWarningMacro("Unexpected user_data. Expecting RemoteProxyInfo.");
    return;
  }

  const Variant& data = user_data.variant(0);
  this->RemoteProxySessionID = data.idtype(0);
  this->RemoteProxyID = data.idtype(0);
  this->PortToExport = data.integer(0);

  // Update associated VTK object
  this->SetExternalProxy(this->GetExternalProxy(), this->PortToExport);
}
