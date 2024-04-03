// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPluginManager.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVPluginsInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMPluginLoaderProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <cassert>
#include <map>

class vtkSMPluginManager::vtkInternals
{
public:
  typedef std::map<vtkSMSession*, vtkSmartPointer<vtkPVPluginsInformation>> RemoteInfoMapType;
  RemoteInfoMapType RemoteInformations;
};

namespace
{
class vtkFlagStateUpdated
{
  bool Prev;
  bool& Flag;

public:
  vtkFlagStateUpdated(bool& flag, bool new_val = true)
    : Prev(flag)
    , Flag(flag)
  {
    this->Flag = new_val;
  }
  ~vtkFlagStateUpdated() { this->Flag = this->Prev; }

private:
  vtkFlagStateUpdated(const vtkFlagStateUpdated&) = delete;
  void operator=(const vtkFlagStateUpdated&) = delete;
};
}

vtkStandardNewMacro(vtkSMPluginManager);
//----------------------------------------------------------------------------
vtkSMPluginManager::vtkSMPluginManager()
{
  this->Internals = new vtkInternals();

  this->InLoadPlugin = false;

  // Setup and update local plugins information.
  this->LocalInformation = vtkPVPluginsInformation::New();
  this->LocalInformation->CopyFromObject(nullptr);

  // When a plugin is register with the local tracker (either at buildtime or
  // at runtime, RegisterEvent is fired. We ensure that the PluginLoadedEvent
  // get fired, especially for plugins brought in a buildtime.
  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();
  tracker->AddObserver(vtkCommand::RegisterEvent, this, &vtkSMPluginManager::OnPluginRegistered);
  tracker->AddObserver(
    vtkPVPluginTracker::RegisterAvailablePluginEvent, this, &vtkSMPluginManager::OnPluginAvailable);
}

//----------------------------------------------------------------------------
vtkSMPluginManager::~vtkSMPluginManager()
{
  delete this->Internals;
  this->Internals = nullptr;

  this->LocalInformation->Delete();
  this->LocalInformation = nullptr;
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::RegisterSession(vtkSMSession* session)
{
  assert(session != nullptr);

  if (this->Internals->RemoteInformations.find(session) !=
    this->Internals->RemoteInformations.end())
  {
    vtkWarningMacro("Session already registered!!!");
  }
  else
  {
    vtkPVPluginsInformation* remoteInfo = vtkPVPluginsInformation::New();
    this->Internals->RemoteInformations[session].TakeReference(remoteInfo);
    session->GatherInformation(vtkPVSession::DATA_SERVER_ROOT, remoteInfo, 0);
  }

  // Also update the local information. This is generally unnecessary, but for
  // statically linked in plugins, this provides a cleaner opportunity to bring
  // in what's linked in.

  // we use Update so that any auto-load state changes done in
  // this->LocalInformation are preserved.
  vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
  temp->CopyFromObject(nullptr);
  this->LocalInformation->Update(temp);
  temp->Delete();
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::UnRegisterSession(vtkSMSession* session)
{
  this->Internals->RemoteInformations.erase(session);
}

//----------------------------------------------------------------------------
vtkPVPluginsInformation* vtkSMPluginManager::GetRemoteInformation(vtkSMSession* session)
{
  return (session) ? this->Internals->RemoteInformations[session] : nullptr;
}

//----------------------------------------------------------------------------
bool vtkSMPluginManager::LoadLocalPlugin(const char* plugin)
{
  SM_SCOPED_TRACE(LoadPlugin).arg(plugin).arg("remote", false);

  bool status = false;
  if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_BATCH &&
    !vtkProcessModule::GetSymmetricMPIMode())
  {
    // In non-symmetric mode, local-plugin needs to be loaded on all satellites
    // too. That's best doing using the same code as "LoadRemotePlugin".
    if (auto session = vtkSMProxyManager::GetProxyManager()->GetActiveSession())
    {
      status = this->LoadRemotePlugin(plugin, session);
    }
    else
    {
      vtkLogF(WARNING, "no session found. Will simply load plugin on local process.");
    }
  }

  if (!status)
  {
    vtkFlagStateUpdated stateUpdater(this->InLoadPlugin);
    vtkNew<vtkPVPluginLoader> loader;
    if (vtksys::SystemTools::FileIsFullPath(plugin))
    {
      status = loader->LoadPlugin(plugin);
    }
    else
    {
      status = loader->LoadPluginByName(plugin);
    }
  }

  if (status)
  {
    // Update local-plugin information.
    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    temp->CopyFromObject(nullptr);
    // we use Update so that any auto-load state changes done in
    // this->LocalInformation are preserved.
    this->LocalInformation->Update(temp);
    temp->Delete();

    this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
    this->InvokeEvent(vtkSMPluginManager::LocalPluginLoadedEvent, (void*)plugin);
  }

  // We don't report the error here if status == false since vtkPVPluginLoader
  // already reports those errors using vtkErrorMacro.
  return status;
}

//----------------------------------------------------------------------------
bool vtkSMPluginManager::LoadRemotePlugin(const char* plugin, vtkSMSession* session)
{
  assert("Session cannot be nullptr" && session != nullptr);

  SM_SCOPED_TRACE(LoadPlugin).arg(plugin).arg("remote", true);

  vtkFlagStateUpdated stateUpdater(this->InLoadPlugin);

  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSMPluginLoaderProxy* proxy =
    vtkSMPluginLoaderProxy::SafeDownCast(pxm->NewProxy("misc", "PluginLoader"));
  proxy->UpdateVTKObjects();
  bool status;
  if (vtksys::SystemTools::FileIsFullPath(plugin))
  {
    status = proxy->LoadPlugin(plugin);
  }
  else
  {
    status = proxy->LoadPluginByName(plugin);
  }

  if (!status)
  {
    vtkErrorMacro(
      "Plugin load failed: " << vtkSMPropertyHelper(proxy, "ErrorString").GetAsString());
  }
  proxy->Delete();

  // Refresh definitions since those may have changed.
  pxm->GetProxyDefinitionManager()->SynchronizeDefinitions();

  if (status)
  {
    // Refresh the remote plugin information
    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    session->GatherInformation(vtkPVSession::DATA_SERVER_ROOT, temp, 0);
    this->Internals->RemoteInformations[session]->Update(temp);
    temp->Delete();
    this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
    this->InvokeEvent(vtkSMPluginManager::RemotePluginLoadedEvent, (void*)plugin);
  }
  return status;
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::LoadPluginConfigurationXML(
  const char* configurationFile, vtkSMSession* session, bool remote)
{
  vtkFlagStateUpdated stateUpdater(this->InLoadPlugin);
  if (remote)
  {
    assert("Session should already be set" && (session != nullptr));
    vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
    vtkSMPluginLoaderProxy* proxy =
      vtkSMPluginLoaderProxy::SafeDownCast(pxm->NewProxy("misc", "PluginLoader"));
    proxy->UpdateVTKObjects();
    proxy->LoadPluginConfigurationXML(configurationFile);
    proxy->Delete();

    // Refresh definitions since those may have changed.
    pxm->GetProxyDefinitionManager()->SynchronizeDefinitions();

    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    session->GatherInformation(vtkPVSession::DATA_SERVER_ROOT, temp, 0);
    this->Internals->RemoteInformations[session]->Update(temp);
    temp->Delete();
  }
  else
  {
    vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXML(configurationFile);

    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    temp->CopyFromObject(nullptr);
    this->LocalInformation->Update(temp);
    temp->Delete();
  }

  this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::LoadPluginConfigurationXMLFromString(
  const char* xmlcontents, vtkSMSession* session, bool remote)
{
  vtkFlagStateUpdated stateUpdater(this->InLoadPlugin);
  if (remote)
  {
    assert("Session should already be set" && (session != nullptr));
    vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
    vtkSMPluginLoaderProxy* proxy =
      vtkSMPluginLoaderProxy::SafeDownCast(pxm->NewProxy("misc", "PluginLoader"));
    proxy->UpdateVTKObjects();
    proxy->LoadPluginConfigurationXMLFromString(xmlcontents);
    proxy->Delete();

    // Refresh definitions since those may have changed.
    pxm->GetProxyDefinitionManager()->SynchronizeDefinitions();

    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    session->GatherInformation(vtkPVSession::DATA_SERVER_ROOT, temp, 0);
    this->Internals->RemoteInformations[session]->Update(temp);
    temp->Delete();
  }
  else
  {
    vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXMLFromString(xmlcontents);

    vtkPVPluginsInformation* temp = vtkPVPluginsInformation::New();
    temp->CopyFromObject(nullptr);
    this->LocalInformation->Update(temp);
    temp->Delete();
  }

  this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::OnPluginRegistered()
{
  if (this->InLoadPlugin)
  {
    return;
  }

  this->UpdateLocalPluginInformation();
  this->InvokeEvent(vtkSMPluginManager::PluginLoadedEvent);
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::OnPluginAvailable()
{
  this->UpdateLocalPluginInformation();
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::UpdateLocalPluginInformation()
{
  // Update local-plugin information.
  vtkNew<vtkPVPluginsInformation> temp;
  temp->CopyFromObject(nullptr);
  // we use Update so that any auto-load state changes done in
  // this->LocalInformation are preserved.
  this->LocalInformation->Update(temp);
}

//----------------------------------------------------------------------------
const char* vtkSMPluginManager::GetLocalPluginSearchPaths()
{
  return this->LocalInformation->GetSearchPaths();
}

//----------------------------------------------------------------------------
const char* vtkSMPluginManager::GetRemotePluginSearchPaths(vtkSMSession* session)
{
  return this->Internals->RemoteInformations[session]->GetSearchPaths();
}

//----------------------------------------------------------------------------
void vtkSMPluginManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkSMPluginManager::FulfillPluginRequirements(vtkSMSession* session, bool onlyCheck)
{
  vtkPVPluginsInformation* clientPlugins = this->GetLocalInformation();
  vtkPVPluginsInformation* serverPlugins = this->GetRemoteInformation(session);

  if (!clientPlugins || !serverPlugins)
  {
    return false;
  }

  std::map<std::string, unsigned int> clientMap;
  for (unsigned int i = 0; i < clientPlugins->GetNumberOfPlugins(); i++)
  {
    clientMap.emplace(clientPlugins->GetPluginName(i), i);
  }

  std::map<std::string, unsigned int> serverMap;
  for (unsigned int i = 0; i < serverPlugins->GetNumberOfPlugins(); i++)
  {
    serverMap.emplace(serverPlugins->GetPluginName(i), i);
  }
  // XXX here it would be nice tu actually check the "RequiredPlugins" but this may be complex
  // as a recursion would be needed somewhere.

  // client plugin can load server plugins, but not the other way around
  bool ret = true;
  ret = this->FulfillPluginClientServerRequirements(
    session, clientMap, clientPlugins, serverMap, serverPlugins, true, onlyCheck);
  ret &= this->FulfillPluginClientServerRequirements(
    session, serverMap, serverPlugins, clientMap, clientPlugins, false, true);
  return ret;
}

//----------------------------------------------------------------------------
bool vtkSMPluginManager::FulfillPluginClientServerRequirements(vtkSMSession* session,
  const std::map<std::string, unsigned int>& inputMap, vtkPVPluginsInformation* inputPluginInfo,
  const std::map<std::string, unsigned int>& outputMap, vtkPVPluginsInformation* outputPluginInfo,
  bool clientInput, bool onlyCheck)
{
  bool ret = true;
  std::string locationName = clientInput ? "server" : "client";
  for (auto const& iter : inputMap)
  {
    inputPluginInfo->SetPluginStatusMessage(iter.second, "");

    // Check plugin is loaded
    if (!inputPluginInfo->GetPluginLoaded(iter.second))
    {
      continue;
    }

    // Check if it is required on the other side
    if (!(clientInput ? inputPluginInfo->GetRequiredOnServer(iter.second)
                      : inputPluginInfo->GetRequiredOnClient(iter.second)))
    {
      continue;
    }

    // Required plugin, check is loaded/loadable on the other side
    std::map<std::string, unsigned int>::const_iterator iter2 = outputMap.find(iter.first);
    if (iter2 == outputMap.cend())
    {
      ret = false;
      inputPluginInfo->SetPluginStatusMessage(iter.second,
        std::string("Must be loaded on " + locationName + " as well but it is not present")
          .c_str());
    }
    else if (!outputPluginInfo->GetPluginLoaded(iter2->second))
    {
      if (onlyCheck || !clientInput)
      {
        ret = false;
        inputPluginInfo->SetPluginStatusMessage(iter.second,
          std::string("Must be loaded on " + locationName + " as well but it is not loaded")
            .c_str());
      }
      else
      {
        // Available required remote plugins are automatically loaded
        if (!this->LoadRemotePlugin(iter2->first.c_str(), session))
        {
          ret = false;
          inputPluginInfo->SetPluginStatusMessage(iter.second,
            std::string("Must be loaded on " + locationName + " as well but it does not load")
              .c_str());
        }
      }
    }
  }
  return ret;
}
