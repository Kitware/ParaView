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

#include "vtkSMProxyManager.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkWeakPointer.h"

//-----------------------------------------------------------------------------
class vtkSMPythonTraceObserverCommandHelper : public vtkCommand
{
public:
  static vtkSMPythonTraceObserverCommandHelper* New()
    { 
    return new vtkSMPythonTraceObserverCommandHelper; 
    }
  
  void SetTarget(vtkSMPythonTraceObserver* t)
    {
    this->Target = t;
    }

  virtual void Execute(vtkObject* caller, unsigned long eventid, void* data)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(caller, eventid, data);
      }
    }

private:
  vtkSMPythonTraceObserverCommandHelper()
    {
    this->Target = 0;
    }

  vtkSMPythonTraceObserver* Target;
};

//-----------------------------------------------------------------------------
class vtkSMPythonTraceObserver::vtkInternal {
public:
  vtkWeakPointer<vtkSMSessionProxyManager> SessionProxyManager;
  vtkSMProxyManager::RegisteredProxyInformation LastRegisterProxyInfo;
  vtkSMProxyManager::RegisteredProxyInformation LastUnRegisterProxyInfo;
  vtkSMProxyManager::ModifiedPropertyInformation LastModifiedPropertyInfo;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPythonTraceObserver);

//-----------------------------------------------------------------------------
vtkSMPythonTraceObserver::vtkSMPythonTraceObserver()
{
  this->Internal = new vtkInternal;

  this->Observer = vtkSMPythonTraceObserverCommandHelper::New();
  this->Observer->SetTarget(this);

  vtkSMSessionProxyManager* pxm =
      vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  if(!pxm)
    {
    vtkWarningMacro("No ProxyManager available. No Observation will be made");
    return;
    }

  // Add observer for certain proxy manager events.
  // Use a high priority for the RegisterEvent so that the trace
  // observer is notified before other observers.  Other observers
  // may modify properties on the newly registered proxy during their
  // RegisterEvent handlers.  We don't want the trace observer to see
  // the property modifications before it has seen the RegisterEvent.
  pxm->AddObserver(vtkCommand::RegisterEvent, this->Observer, 100);
  pxm->AddObserver(vtkCommand::UnRegisterEvent, this->Observer);
  pxm->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer);
  pxm->AddObserver(vtkCommand::UpdateInformationEvent, this->Observer);

  // Keep the session proxy manager around so we can unregiter observers
  // to the correct one.
  this->Internal->SessionProxyManager = pxm;
}

//-----------------------------------------------------------------------------
vtkSMPythonTraceObserver::~vtkSMPythonTraceObserver()
{
  vtkSMSessionProxyManager* pxm = this->Internal->SessionProxyManager.GetPointer();
  if (pxm)
    {
    pxm->RemoveObserver(this->Observer);
    }

  this->Observer->SetTarget(NULL);
  this->Observer->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkSMPythonTraceObserver::ExecuteEvent(vtkObject* vtkNotUsed(caller), 
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

//-----------------------------------------------------------------------------
void vtkSMPythonTraceObserver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  //os << indent << "Var: " << this->Var << endl;
}
