/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPythonTraceObserver.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPythonTraceObserver.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkWeakPointer.h"

#include <vector>

//-----------------------------------------------------------------------------
class vtkSMPythonTraceObserver::vtkInternal {
public:
  vtkWeakPointer<vtkSMSessionProxyManager> SessionProxyManager;
  vtkWeakPointer<vtkSMPluginManager> PluginManager;

  vtkSMProxyManager::RegisteredProxyInformation LastRegisterProxyInfo;
  vtkSMProxyManager::RegisteredProxyInformation LastUnRegisterProxyInfo;
  vtkSMProxyManager::ModifiedPropertyInformation LastModifiedPropertyInfo;
  const char* LastLocalPluginLoaded;
  const char* LastRemotePluginLoaded;

  std::vector<unsigned long> PXMObserverIds;
  std::vector<unsigned long> PMObserverIds;

  ~vtkInternal()
    {
    for (size_t cc=0;
      (this->SessionProxyManager != NULL) && (cc < this->PXMObserverIds.size()); cc++)
      {
      this->SessionProxyManager->RemoveObserver(this->PXMObserverIds[cc]);
      }
    for (size_t cc=0;
      (this->PluginManager != NULL) && (cc < this->PMObserverIds.size()); cc++)
      {
      this->PluginManager->RemoveObserver(this->PMObserverIds[cc]);
      }
    }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPythonTraceObserver);

//-----------------------------------------------------------------------------
vtkSMPythonTraceObserver::vtkSMPythonTraceObserver()
{
  this->Internal = new vtkInternal;
  vtkSMSessionProxyManager* pxm =
      vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  vtkSMPluginManager *pm =
    vtkSMProxyManager::GetProxyManager()->GetPluginManager();

  if (!pxm)
    {
    vtkWarningMacro("No ProxyManager available. No Observation will be made");
    return;
    }

  if (!pm)
    {
    vtkWarningMacro("No PluginManager available. Something must be incorrect.");
    return;
    }

  // Add observers for certain proxy manager events.
  // Use a high priority for the RegisterEvent so that the trace
  // observer is notified before other observers.  Other observers
  // may modify properties on the newly registered proxy during their
  // RegisterEvent handlers.  We don't want the trace observer to see
  // the property modifications before it has seen the RegisterEvent.
  
  std::vector<unsigned long> &pxmObserverIds = this->Internal->PXMObserverIds;
  pxmObserverIds.push_back(pxm->AddObserver(vtkCommand::RegisterEvent,
      this, &vtkSMPythonTraceObserver::EventCallback, 100));
  pxmObserverIds.push_back(pxm->AddObserver(vtkCommand::UnRegisterEvent,
      this, &vtkSMPythonTraceObserver::EventCallback));
  pxmObserverIds.push_back(pxm->AddObserver(vtkCommand::PropertyModifiedEvent,
      this, &vtkSMPythonTraceObserver::EventCallback));
  pxmObserverIds.push_back(pxm->AddObserver(vtkCommand::UpdateInformationEvent,
      this, &vtkSMPythonTraceObserver::EventCallback));

  std::vector<unsigned long> &pmObserverIds = this->Internal->PMObserverIds;
  pmObserverIds.push_back(pm->AddObserver(vtkSMPluginManager::LocalPluginLoadedEvent,
      this, &vtkSMPythonTraceObserver::EventCallback));
  pmObserverIds.push_back(pm->AddObserver(vtkSMPluginManager::RemotePluginLoadedEvent,
      this, &vtkSMPythonTraceObserver::EventCallback));

  // Keep the session proxy manager around so we can unregiter observers
  // to the correct one.
  this->Internal->SessionProxyManager = pxm;
}

//-----------------------------------------------------------------------------
vtkSMPythonTraceObserver::~vtkSMPythonTraceObserver()
{
  delete this->Internal;
  this->Internal = NULL;
}

//-----------------------------------------------------------------------------
void vtkSMPythonTraceObserver::EventCallback(vtkObject* vtkNotUsed(caller), 
  unsigned long eventid, void* data)
{
  switch (eventid)
    {
    case vtkCommand::RegisterEvent:
      {
      vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
        vtkSMProxyManager::RegisteredProxyInformation*>(data));

      if (info.Type == vtkSMProxyManager::RegisteredProxyInformation::PROXY)
        {
        this->Internal->LastRegisterProxyInfo = info;
        this->InvokeEvent(vtkCommand::RegisterEvent);
        }
      }
    break;

    case vtkCommand::UnRegisterEvent:
      {
      vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
        vtkSMProxyManager::RegisteredProxyInformation*>(data));

      if (info.Type == vtkSMProxyManager::RegisteredProxyInformation::PROXY)
        {
        this->Internal->LastUnRegisterProxyInfo = info;
        this->InvokeEvent(vtkCommand::UnRegisterEvent);
        }
      }
    break;

    case vtkCommand::PropertyModifiedEvent:
      {
      vtkSMProxyManager::ModifiedPropertyInformation &info =*(reinterpret_cast<
        vtkSMProxyManager::ModifiedPropertyInformation*>(data));
      this->Internal->LastModifiedPropertyInfo = info;
      this->InvokeEvent(vtkCommand::PropertyModifiedEvent);
      }
    break;

    case vtkCommand::UpdateInformationEvent:
      {
      this->InvokeEvent(vtkCommand::UpdateInformationEvent);
      }
    break;

    case vtkSMPluginManager::LocalPluginLoadedEvent:
      {
      const char *filename = (reinterpret_cast<const char*>(data));
      this->Internal->LastLocalPluginLoaded = filename;
      this->InvokeEvent(vtkSMPluginManager::LocalPluginLoadedEvent);
      }
    break;

    case vtkSMPluginManager::RemotePluginLoadedEvent:
      {
      const char *filename = (reinterpret_cast<const char*>(data));
      this->Internal->LastRemotePluginLoaded = filename;
      this->InvokeEvent(vtkSMPluginManager::RemotePluginLoadedEvent);
      }
    break;
    }
}


//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPythonTraceObserver::GetLastPropertyModifiedProxy()
{
  return this->Internal->LastModifiedPropertyInfo.Proxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPythonTraceObserver::GetLastProxyRegistered()
{
  return this->Internal->LastRegisterProxyInfo.Proxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPythonTraceObserver::GetLastProxyUnRegistered()
{
  return this->Internal->LastUnRegisterProxyInfo.Proxy;
}

//-----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastPropertyModifiedName()
{
  return this->Internal->LastModifiedPropertyInfo.PropertyName;
}

//-----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastProxyRegisteredGroup()
{
  return this->Internal->LastRegisterProxyInfo.GroupName;
}

//-----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastProxyRegisteredName()
{
  return this->Internal->LastRegisterProxyInfo.ProxyName;
}

//-----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastProxyUnRegisteredGroup()
{
  return this->Internal->LastUnRegisterProxyInfo.GroupName;
}

//-----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastProxyUnRegisteredName()
{
  return this->Internal->LastUnRegisterProxyInfo.ProxyName;
}

// ----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastLocalPluginLoaded()
{
  return this->Internal->LastLocalPluginLoaded;
}

// ----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastRemotePluginLoaded()
{
  return this->Internal->LastRemotePluginLoaded;
}

//-----------------------------------------------------------------------------
void vtkSMPythonTraceObserver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  //os << indent << "Var: " << this->Var << endl;
}
