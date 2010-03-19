/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoStackBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUndoStackBuilder.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSMGlobalPropertiesLinkUndoElement.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyRegisterUndoElement.h"
#include "vtkSMProxyStateChangedUndoElement.h"
#include "vtkSMProxyUnRegisterUndoElement.h"
#include "vtkSMUndoStack.h"
#include "vtkSMUpdateInformationUndoElement.h"
#include "vtkUndoElement.h"
#include "vtkUndoSet.h"
#include "vtkUndoStackInternal.h"

#include <vtksys/RegularExpression.hxx>

class vtkSMUndoStackBuilderObserver : public vtkCommand
{
public:
  static vtkSMUndoStackBuilderObserver* New()
    { 
    return new vtkSMUndoStackBuilderObserver; 
    }
  
  void SetTarget(vtkSMUndoStackBuilder* t)
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
  vtkSMUndoStackBuilderObserver()
    {
    this->Target = 0;
    }

  vtkSMUndoStackBuilder* Target;
};

vtkStandardNewMacro(vtkSMUndoStackBuilder);
vtkCxxRevisionMacro(vtkSMUndoStackBuilder, "1.4");
vtkCxxSetObjectMacro(vtkSMUndoStackBuilder, UndoStack, vtkSMUndoStack);
//-----------------------------------------------------------------------------
vtkSMUndoStackBuilder::vtkSMUndoStackBuilder()
{
  this->Observer = vtkSMUndoStackBuilderObserver::New();
  this->Observer->SetTarget(this);

  this->UndoStack = 0;
  this->UndoSet = vtkUndoSet::New();
  this->ConnectionID = vtkProcessModuleConnectionManager::GetNullConnectionID();
  this->Label = NULL;
  this->EnableMonitoring = 0;
  this->IgnoreAllChanges = false;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (!pxm)
    {
    vtkErrorMacro("vtkSMUndoStackBuilder must be created only"
       << " after the ProxyManager has been created.");
    }
  else
    {
    // It is essential that the Undo/Redo system notices these events
    // before anyone else, hence we put these observers on a high priority level.
    pxm->AddObserver(vtkCommand::RegisterEvent, this->Observer, 100);
    pxm->AddObserver(vtkCommand::UnRegisterEvent, this->Observer, 100);
    pxm->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer, 100);
    pxm->AddObserver(vtkCommand::StateChangedEvent, this->Observer, 100);
    pxm->AddObserver(vtkCommand::UpdateInformationEvent, this->Observer, 100);

    // Add existing global properties managers.
    for (unsigned int cc=0; cc < pxm->GetNumberOfGlobalPropertiesManagers(); cc++)
      {
      this->OnRegisterGlobalPropertiesManager(
        pxm->GetGlobalPropertiesManager(cc));
      }
    }
}

//-----------------------------------------------------------------------------
vtkSMUndoStackBuilder::~vtkSMUndoStackBuilder()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (pxm)
    {
    pxm->RemoveObserver(this->Observer);
    }

  this->Observer->SetTarget(NULL);
  this->Observer->Delete();

  if (this->UndoSet)
    {
    this->UndoSet->Delete();
    this->UndoSet = NULL;
    }
  this->SetLabel(NULL);
  this->SetUndoStack(0);
}
//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::Begin(const char* label)
{
  if (!this->Label)
    {
    this->SetLabel(label);
    }

  ++this->EnableMonitoring;
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::End()
{
  if (this->EnableMonitoring == 0)
    {
    vtkWarningMacro("Unmatched End().");
    return;
    }
  this->EnableMonitoring--;

}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::PushToStack()
{
  if (this->UndoSet->GetNumberOfElements() > 0 && this->UndoStack)
    {
    this->UndoStack->Push(this->ConnectionID,
      (this->Label? this->Label : "Changes"), 
      this->UndoSet);
    }
  this->InitializeUndoSet();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::Clear()
{
  this->InitializeUndoSet();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::InitializeUndoSet()
{
  this->SetLabel(NULL);
  this->UndoSet->RemoveAllElements();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::Add(vtkUndoElement* element)
{
  if (!element)
    {
    return;
    }

  this->UndoSet->AddElement(element);
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::ExecuteEvent(vtkObject* caller, 
  unsigned long eventid, void* data)
{
  // These must be handled irrespective of whether IgnoreAllChanges or
  // HandleChangeEvents is set.
  if (eventid == vtkCommand::RegisterEvent)
    {
    vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
      vtkSMProxyManager::RegisteredProxyInformation*>(data));
    if (info.Type == 
      vtkSMProxyManager::RegisteredProxyInformation::GLOBAL_PROPERTIES_MANAGER)
      {
        this->OnRegisterGlobalPropertiesManager(
          vtkSMGlobalPropertiesManager::SafeDownCast(info.Proxy));
      }
    }
  else if (eventid == vtkCommand::UnRegisterEvent)
    {
    vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
      vtkSMProxyManager::RegisteredProxyInformation*>(data));
    if (info.Type == 
      vtkSMProxyManager::RegisteredProxyInformation::GLOBAL_PROPERTIES_MANAGER)
      {
        this->OnUnRegisterGlobalPropertiesManager(
          vtkSMGlobalPropertiesManager::SafeDownCast(info.Proxy));
      }
    }

  if (this->IgnoreAllChanges || !this->HandleChangeEvents())
    {
    return;
    }

  switch (eventid)
    {
  case vtkCommand::RegisterEvent:
      {
      vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
        vtkSMProxyManager::RegisteredProxyInformation*>(data));

      switch (info.Type)
        {
      case vtkSMProxyManager::RegisteredProxyInformation::COMPOUND_PROXY_DEFINITION:
        // Compound proxy definition registered.
        break;

      case vtkSMProxyManager::RegisteredProxyInformation::LINK:
        // link registered.
        this->OnRegisterLink(info.ProxyName);
        break;

      case vtkSMProxyManager::RegisteredProxyInformation::PROXY:
        this->OnRegisterProxy(info.GroupName, info.ProxyName, info.Proxy);
        break;
        }
      }
    break;

  case vtkCommand::UnRegisterEvent:
      {
      vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
        vtkSMProxyManager::RegisteredProxyInformation*>(data));

      switch (info.Type)
        {
      case vtkSMProxyManager::RegisteredProxyInformation::COMPOUND_PROXY_DEFINITION:
        // Compound proxy definition registered.
        break;

      case vtkSMProxyManager::RegisteredProxyInformation::LINK:
        // link unregistered.
        this->OnUnRegisterLink(info.ProxyName);
        break;

      case vtkSMProxyManager::RegisteredProxyInformation::PROXY:
        this->OnUnRegisterProxy(info.GroupName, info.ProxyName, info.Proxy);
        break;
        }
      }
    break;

  case vtkCommand::PropertyModifiedEvent:
      {
      vtkSMProxyManager::ModifiedPropertyInformation &info =*(reinterpret_cast<
        vtkSMProxyManager::ModifiedPropertyInformation*>(data)); 
      this->OnPropertyModified(info.Proxy, info.PropertyName);
      }
    break;

  case vtkCommand::StateChangedEvent:
      {
      vtkSMProxyManager::StateChangedInformation &info = *(reinterpret_cast<
        vtkSMProxyManager::StateChangedInformation*>(data));
      this->OnProxyStateChanged(info.Proxy, info.StateChangeElement);
      }
    break;

  case vtkCommand::UpdateInformationEvent:
      {
      this->OnUpdateInformation(reinterpret_cast<vtkSMProxy*>(data));
      }
    break;

  case vtkCommand::ModifiedEvent:
      {
      vtkSMGlobalPropertiesManager* mgr =
        vtkSMGlobalPropertiesManager::SafeDownCast(caller);
      vtkSMGlobalPropertiesManager::ModifiedInfo *info =
        reinterpret_cast<vtkSMGlobalPropertiesManager::ModifiedInfo*>(data);
      if (mgr && info)
        {
        if (info->AddLink)
          {
          this->GlobalPropertiesLinkAdded(
            this->GetProxyManager()->GetGlobalPropertiesManagerName(mgr),
            info->GlobalPropertyName, info->Proxy, info->PropertyName);
          }
        else
          {
          this->GlobalPropertiesLinkRemoved(
            this->GetProxyManager()->GetGlobalPropertiesManagerName(mgr),
            info->GlobalPropertyName, info->Proxy, info->PropertyName);
          }
        }
      }
    break;

    }
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnRegisterProxy(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  // proxies registered as prototypes don't participate in
  // undo/redo.
  vtksys::RegularExpression prototypesRe("_prototypes$");

  if (!group || prototypesRe.find(group) != 0)
    {
    return;
    }

  vtkSMProxyRegisterUndoElement* elem = vtkSMProxyRegisterUndoElement::New();
  elem->SetConnectionID(this->ConnectionID);
  elem->ProxyToRegister(group, name, proxy);
  this->UndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnUnRegisterProxy(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  // proxies registered as prototypes don't participate in
  // undo/redo.
  vtksys::RegularExpression prototypesRe("_prototypes$");

  if (!proxy || (group && prototypesRe.find(group) != 0))
    {
    return;
    }

  vtkSMProxyUnRegisterUndoElement* elem = 
    vtkSMProxyUnRegisterUndoElement::New();
  elem->SetConnectionID(this->ConnectionID);
  elem->ProxyToUnRegister(group, name, proxy);
  this->UndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnPropertyModified(vtkSMProxy* proxy, 
  const char* pname)
{
  // TODO: We need to determine if the property is being changed on a proxy 
  // that is registered only as a prototype. If so, we should not worry
  // about recording its property changes. When we update the SM data structure
  // to separately manage prototypes, this will be take care of automatically.
  // Hence, we defer it for now.
 
  vtkSMProperty* prop = proxy->GetProperty(pname);
  if (prop && !prop->GetInformationOnly() && !prop->GetIsInternal())
    {
    vtkSMPropertyModificationUndoElement* elem = 
      vtkSMPropertyModificationUndoElement::New();
    elem->ModifiedProperty(proxy, pname);
    this->UndoSet->AddElement(elem);
    elem->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnProxyStateChanged(vtkSMProxy* proxy,
  vtkPVXMLElement* stateChange)
{
  if (proxy && stateChange)
    {
    vtkSMProxyStateChangedUndoElement* elem =
      vtkSMProxyStateChangedUndoElement::New();
    elem->StateChanged(proxy, stateChange);
    this->UndoSet->AddElement(elem);
    elem->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnUpdateInformation(vtkSMProxy* proxy)
{
  vtkSMUpdateInformationUndoElement* elem = 
    vtkSMUpdateInformationUndoElement::New();
  elem->Updated(proxy);
  this->UndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnRegisterLink(const char* vtkNotUsed(name))
{
  // TODO: This will be implemented in future.
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnUnRegisterLink(const char* vtkNotUsed(name))
{
  // TODO: This will be implemented in future.
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnRegisterGlobalPropertiesManager(
  vtkSMGlobalPropertiesManager* mgr)
{
  mgr->AddObserver(vtkCommand::ModifiedEvent, this->Observer, 100);
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnUnRegisterGlobalPropertiesManager(
  vtkSMGlobalPropertiesManager* mgr)
{
  mgr->RemoveObserver(this->Observer);
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::GlobalPropertiesLinkAdded(
  const char* mgrname,
  const char* globalname, vtkSMProxy* proxy, const char* propname)
{
  vtkSMGlobalPropertiesLinkUndoElement* elem = 
    vtkSMGlobalPropertiesLinkUndoElement::New();
  elem->LinkAdded(mgrname, globalname, proxy, propname);
  this->UndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::GlobalPropertiesLinkRemoved(
  const char* mgrname,
  const char* globalname, vtkSMProxy* proxy, const char* propname)
{
  vtkSMGlobalPropertiesLinkUndoElement* elem = 
    vtkSMGlobalPropertiesLinkUndoElement::New();
  elem->LinkRemoved(mgrname, globalname, proxy, propname);
  this->UndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IgnoreAllChanges: " << this->IgnoreAllChanges << endl;
  os << indent << "ConnectionID: " << this->ConnectionID << endl;
  os << indent << "UndoStack: " << this->UndoStack << endl;
}
