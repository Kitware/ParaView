/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPluginManager.h"

#include "vtkObjectFactory.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVPluginsInformation.h"
#include "vtkPVPluginTracker.h"
#include "vtkSMPluginLoaderProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkWeakPointer.h"

class vtkSMPluginManager::vtkInternals
{
public:
  vtkWeakPointer<vtkSMSession> Session;
};

vtkStandardNewMacro(vtkSMPluginManager);
//----------------------------------------------------------------------------
vtkSMPluginManager::vtkSMPluginManager()
{
  this->Internals = new vtkInternals();
  this->LocalInformation = vtkPVPluginsInformation::New();
  this->RemoteInformation = vtkPVPluginsInformation::New();
}

//----------------------------------------------------------------------------
vtkSMPluginManager::~vtkSMPluginManager()
{
  delete this->Internals;
  this->Internals = NULL;

  this->LocalInformation->Delete();
  this->RemoteInformation->Delete();
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::SetSession(vtkSMSession* session)
{
  this->Internals->Session = session;
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::Initialize()
{
  if (!this->Internals->Session)
    {
    vtkErrorMacro("Session must be initialized.");
    return;
    }

  this->Internals->Session->GatherInformation(
    vtkPVSession::CLIENT, this->LocalInformation, 0);
  this->Internals->Session->GatherInformation(
    vtkPVSession::DATA_SERVER_ROOT, this->RemoteInformation, 0);
}

//----------------------------------------------------------------------------
bool vtkSMPluginManager::LoadLocalPlugin(const char* filename)
{
  vtkPVPluginLoader* loader = vtkPVPluginLoader::New();
  bool ret_val = loader->LoadPlugin(filename);
  loader->Delete();
  if (ret_val)
    {
    // Update local-plugin information.
    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    this->Internals->Session->GatherInformation(
      vtkPVSession::CLIENT, temp, 0);
    this->LocalInformation->Update(temp);
    temp->Delete();

    this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
    }

  // We don't report the error here if ret_val == false since vtkPVPluginLoader
  // already reports those errors using vtkErrorMacro.
  return ret_val;
}

//----------------------------------------------------------------------------
bool vtkSMPluginManager::LoadRemotePlugin(const char* filename)
{
  vtkSMPluginLoaderProxy* proxy = vtkSMPluginLoaderProxy::SafeDownCast(
    vtkSMProxyManager::GetProxyManager()->NewProxy("misc", "PluginLoader"));
  proxy->UpdateVTKObjects();
  bool status = proxy->LoadPlugin(filename);
  if (!status)
    {
    vtkErrorMacro("Plugin load failed: " <<
      vtkSMPropertyHelper(proxy, "ErrorString").GetAsString());
    }
  proxy->Delete();

  // Refresh definitions since those may have changed.
  vtkSMProxyManager::GetProxyManager()->GetProxyDefinitionManager()->
    SynchronizeDefinitions();

  if (status)
    {
    // Refresh the remote plugin information
    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    this->Internals->Session->GatherInformation(
      vtkPVSession::DATA_SERVER_ROOT, temp, 0);
    this->RemoteInformation->Update(temp);
    temp->Delete();
    this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
    }
  return status;
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::LoadPluginConfigurationXMLFromString(
  const char* xmlcontents, bool remote)
{
  if (remote)
    {
    vtkSMPluginLoaderProxy* proxy = vtkSMPluginLoaderProxy::SafeDownCast(
      vtkSMProxyManager::GetProxyManager()->NewProxy("misc", "PluginLoader"));
    proxy->UpdateVTKObjects();
    proxy->LoadPluginConfigurationXMLFromString(xmlcontents);
    proxy->Delete();

    // Refresh definitions since those may have changed.
    vtkSMProxyManager::GetProxyManager()->GetProxyDefinitionManager()->
      SynchronizeDefinitions();

    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    this->Internals->Session->GatherInformation(
      vtkPVSession::DATA_SERVER_ROOT, temp, 0);
    this->RemoteInformation->Update(temp);
    temp->Delete();
    }
  else
    {
    vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXMLFromString(
      xmlcontents);

    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    this->Internals->Session->GatherInformation(
      vtkPVSession::CLIENT, temp, 0);
    this->LocalInformation->Update(temp);
    temp->Delete();
    }

  this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
}

//----------------------------------------------------------------------------
const char* vtkSMPluginManager::GetPluginSearchPaths(bool remote)
{
  return remote? this->RemoteInformation->GetSearchPaths() :
    this->LocalInformation->GetSearchPaths();
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
