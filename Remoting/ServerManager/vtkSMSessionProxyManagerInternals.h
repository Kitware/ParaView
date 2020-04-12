/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManagerInternals.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSMProxyManagerInternals_h
#define vtkSMProxyManagerInternals_h

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDebugLeaks.h"
#include "vtkNew.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMLink.h"
#include "vtkSMMessage.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSession.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <vtksys/RegularExpression.hxx>

// Sub-classed to avoid symbol length explosion.
class vtkSMProxyManagerProxyInfo : public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkSMProxyManagerProxyInfo, vtkObjectBase);

  vtkSmartPointer<vtkSMProxy> Proxy;
  unsigned long ModifiedObserverTag;
  unsigned long StateChangedObserverTag;
  unsigned long UpdateObserverTag;
  unsigned long UpdateInformationObserverTag;

  static vtkSMProxyManagerProxyInfo* New()
  {
    // This is required everytime we're implementing our own New() to avoid
    // "Deleting unknown object" warning from vtkDebugLeaks.
    vtkSMProxyManagerProxyInfo* ret = new vtkSMProxyManagerProxyInfo();
    ret->InitializeObjectBase();
    return ret;
  }

private:
  vtkSMProxyManagerProxyInfo()
  {
    this->ModifiedObserverTag = 0;
    this->StateChangedObserverTag = 0;
    this->UpdateObserverTag = 0;
    this->UpdateInformationObserverTag = 0;
  }
  ~vtkSMProxyManagerProxyInfo() override
  {
    // Remove observers.
    if (this->ModifiedObserverTag && this->Proxy.GetPointer())
    {
      this->Proxy.GetPointer()->RemoveObserver(this->ModifiedObserverTag);
      this->ModifiedObserverTag = 0;
    }
    if (this->StateChangedObserverTag && this->Proxy.GetPointer())
    {
      this->Proxy.GetPointer()->RemoveObserver(this->StateChangedObserverTag);
      this->StateChangedObserverTag = 0;
    }
    if (this->UpdateObserverTag && this->Proxy.GetPointer())
    {
      this->Proxy.GetPointer()->RemoveObserver(this->UpdateObserverTag);
      this->UpdateObserverTag = 0;
    }
    if (this->UpdateInformationObserverTag && this->Proxy.GetPointer())
    {
      this->Proxy.GetPointer()->RemoveObserver(this->UpdateInformationObserverTag);
      this->UpdateInformationObserverTag = 0;
    }
  }
};

//-----------------------------------------------------------------------------
class vtkSMProxyManagerProxyListType
  : public std::vector<vtkSmartPointer<vtkSMProxyManagerProxyInfo> >
{
public:
  // Returns if the proxy exists in  this vector.
  bool Contains(vtkSMProxy* proxy)
  {
    vtkSMProxyManagerProxyListType::iterator iter = this->begin();
    for (; iter != this->end(); ++iter)
    {
      if (iter->GetPointer()->Proxy == proxy)
      {
        return true;
      }
    }
    return false;
  }
  vtkSMProxyManagerProxyListType::iterator Find(vtkSMProxy* proxy)
  {
    vtkSMProxyManagerProxyListType::iterator iter = this->begin();
    for (; iter != this->end(); ++iter)
    {
      if (iter->GetPointer()->Proxy.GetPointer() == proxy)
      {
        return iter;
      }
    }
    return this->end();
  }
};

//-----------------------------------------------------------------------------
class vtkSMProxyManagerProxyMapType : public std::map<std::string, vtkSMProxyManagerProxyListType>
{
};
//-----------------------------------------------------------------------------
struct vtkSMProxyManagerEntry
{
  std::string Group;
  std::string Name;
  vtkSmartPointer<vtkSMProxy> Proxy;

  vtkSMProxyManagerEntry(const char* group, const char* name, vtkSMProxy* proxy)
  {
    this->Group = group;
    this->Name = name;
    this->Proxy = proxy;
  }

  bool operator==(const vtkSMProxyManagerEntry& other) const
  {
    return this->Group == other.Group && this->Name == other.Name &&
      this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID();
  }

  bool operator<(const vtkSMProxyManagerEntry& other) const
  {
    if (this->Proxy->GetGlobalID() < other.Proxy->GetGlobalID())
    {
      return true;
    }
    else if (this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID() && this->Group == other.Group)
    {
      return this->Name < other.Name;
    }
    else if (this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID())
    {
      return this->Group < other.Group;
    }
    return false;
  }

  bool operator>(const vtkSMProxyManagerEntry& other) const
  {
    if (this->Proxy->GetGlobalID() > other.Proxy->GetGlobalID())
    {
      return true;
    }
    else if (this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID() && this->Group == other.Group)
    {
      return this->Name > other.Name;
    }
    else if (this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID())
    {
      return this->Group > other.Group;
    }
    return false;
  }
};
//-----------------------------------------------------------------------------
struct vtkSMSessionProxyManagerInternals
{
  // This data structure stores actual proxy instances grouped in
  // collections.
  typedef std::map<std::string, vtkSMProxyManagerProxyMapType> ProxyGroupType;
  ProxyGroupType RegisteredProxyMap;

  // This data structure stores the tuples(group, name, proxy) to compute
  // diff when a state is loaded.
  typedef std::set<vtkSMProxyManagerEntry> GroupNameProxySet;
  GroupNameProxySet RegisteredProxyTuple;

  // This data structure stores a set of proxies that have been modified.
  typedef std::set<vtkSMProxy*> SetOfProxies;
  SetOfProxies ModifiedProxies;

  // Data structure to save registered links.
  typedef std::map<std::string, vtkSmartPointer<vtkSMLink> > LinkType;
  LinkType RegisteredLinkMap;

  // Data structure for selection models.
  typedef std::map<std::string, vtkSmartPointer<vtkSMProxySelectionModel> > SelectionModelsType;
  SelectionModelsType SelectionModels;

  // Data structure for storing the fullState
  vtkSMMessage State;

  // Keep ref to the proxyManager to access the session
  vtkSMSessionProxyManager* ProxyManager;

  // Helper methods -----------------------------------------------------------
  void FindProxyTuples(vtkSMProxy* proxy, std::set<vtkSMProxyManagerEntry>& tuplesFounds)
  {
    std::set<vtkSMProxyManagerEntry>::iterator iter;
    iter = this->RegisteredProxyTuple.begin();
    while (iter != this->RegisteredProxyTuple.end())
    {
      if (iter->Proxy == proxy)
      {
        tuplesFounds.insert(*iter);
      }
      iter++;
    }
  }

  // --------------------------------------------------------------------------
  void ComputeDelta(const vtkSMMessage* newState, vtkSMProxyLocator* locator,
    std::set<vtkSMProxyManagerEntry>& toRegister, std::set<vtkSMProxyManagerEntry>& toUnregister)
  {
    // Fill equivalent temporary data structure
    std::set<vtkSMProxyManagerEntry> newStateContent;
    int max = newState->ExtensionSize(PXMRegistrationState::registered_proxy);
    for (int cc = 0; cc < max; cc++)
    {
      PXMRegistrationState_Entry reg =
        newState->GetExtension(PXMRegistrationState::registered_proxy, cc);
      vtkSMProxy* proxy = locator->LocateProxy(reg.global_id());
      if (proxy)
      {
        newStateContent.insert(
          vtkSMProxyManagerEntry(reg.group().c_str(), reg.name().c_str(), proxy));
      }
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // FIXME there might be a better way to do that
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // Look for proxy to Register
    std::set<vtkSMProxyManagerEntry>::iterator iter;
    iter = newStateContent.begin();
    while (iter != newStateContent.end())
    {
      if (this->RegisteredProxyTuple.find(*iter) == this->RegisteredProxyTuple.end())
      {
        toRegister.insert(*iter);
      }
      iter++;
    }

    // Look for proxy to Unregister
    iter = this->RegisteredProxyTuple.begin();
    vtksys::RegularExpression prototypesRe("_prototypes$");
    while (iter != this->RegisteredProxyTuple.end())
    {
      if (newStateContent.find(*iter) == newStateContent.end() &&
        !prototypesRe.find(iter->Group.c_str()))
      {
        toUnregister.insert(*iter);
      }
      iter++;
    }
  }

  // --------------------------------------------------------------------------
  void RemoveTuples(const char* name, std::set<vtkSMProxyManagerEntry>& removedEntries)
  {
    // Convert param
    std::string nameString = name;

    // Deal with set
    GroupNameProxySet resultSet;
    std::set<vtkSMProxyManagerEntry>::iterator iter;
    iter = this->RegisteredProxyTuple.begin();
    while (iter != this->RegisteredProxyTuple.end())
    {
      if (iter->Name != nameString)
      {
        resultSet.insert(*iter);
      }
      iter++;
    }
    this->RegisteredProxyTuple = resultSet;

    // Deal with map only (true)
    vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
      this->RegisteredProxyMap.begin();
    for (; it != this->RegisteredProxyMap.end(); it++)
    {
      vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
      if (it2 != it->second.end())
      {
        this->RemoveTuples(it->first.c_str(), name, removedEntries, true);
      }
    }

    // Deal with State
    vtkSMMessage backup;
    backup.CopyFrom(this->State);
    int nbRegisteredProxy = this->State.ExtensionSize(PXMRegistrationState::registered_proxy);
    this->State.ClearExtension(PXMRegistrationState::registered_proxy);
    for (int cc = 0; cc < nbRegisteredProxy; ++cc)
    {
      const PXMRegistrationState_Entry* reg =
        &backup.GetExtension(PXMRegistrationState::registered_proxy, cc);

      if (reg->name() != nameString) // Keep it
      {
        this->State.AddExtension(PXMRegistrationState::registered_proxy)->CopyFrom(*reg);
      }
    }
  }

  // --------------------------------------------------------------------------
  void RemoveTuples(const char* group, const char* name,
    std::set<vtkSMProxyManagerEntry>& removedEntries, bool doMapOnly)
  {
    // Convert parameters
    std::string groupString = group;
    std::string nameString = name;

    // Deal with set
    if (!doMapOnly)
    {
      GroupNameProxySet resultSet;
      std::set<vtkSMProxyManagerEntry>::iterator iter;
      iter = this->RegisteredProxyTuple.begin();
      while (iter != this->RegisteredProxyTuple.end())
      {
        if (iter->Group != groupString || iter->Name != nameString)
        {
          resultSet.insert(*iter);
        }
        iter++;
      }
      this->RegisteredProxyTuple = resultSet;
    }

    // Deal with map
    vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
      this->RegisteredProxyMap.find(group);
    if (it != this->RegisteredProxyMap.end())
    {
      vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
      if (it2 != it->second.end())
      {
        vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
        while (it3 != it2->second.end())
        {
          removedEntries.insert(vtkSMProxyManagerEntry(group, name, (*it3)->Proxy));
          it3++;
        }
        it->second.erase(it2);
      }
    }

    // Deal with state
    vtksys::RegularExpression prototypesRe("_prototypes$");
    if (!doMapOnly && !prototypesRe.find(group))
    {
      vtkSMMessage backup;
      backup.CopyFrom(this->State);
      int nbRegisteredProxy = this->State.ExtensionSize(PXMRegistrationState::registered_proxy);
      this->State.ClearExtension(PXMRegistrationState::registered_proxy);
      for (int cc = 0; cc < nbRegisteredProxy; ++cc)
      {
        const PXMRegistrationState_Entry* reg =
          &backup.GetExtension(PXMRegistrationState::registered_proxy, cc);

        if (reg->group() != groupString || reg->name() != nameString) // Keep it
        {
          this->State.AddExtension(PXMRegistrationState::registered_proxy)->CopyFrom(*reg);
        }
      }
    }
  }

  // --------------------------------------------------------------------------
  // Return true if the given tuple has been found
  bool RemoveTuples(const char* group, const char* name, vtkSMProxy* proxy)
  {
    // Convert parameters
    std::string groupString = group;
    std::string nameString = name;

    // Will be overridden if the proxy is found
    bool found = false;

    // Deal with set
    this->RegisteredProxyTuple.erase(vtkSMProxyManagerEntry(group, name, proxy));

    // Deal with map
    vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
      this->RegisteredProxyMap.find(group);
    if (it != this->RegisteredProxyMap.end())
    {
      vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
      if (it2 != it->second.end())
      {
        vtkSMProxyManagerProxyListType::iterator it3 = it2->second.Find(proxy);
        if (it3 != it2->second.end())
        {
          found = true;
          it2->second.erase(it3);
        }
        if (it2->second.size() == 0)
        {
          it->second.erase(it2);
        }
      }
    }

    // Deal with state
    vtksys::RegularExpression prototypesRe("_prototypes$");
    if (!prototypesRe.find(group))
    {
      vtkSMMessage backup;
      backup.CopyFrom(this->State);

      int nbRegisteredProxy = this->State.ExtensionSize(PXMRegistrationState::registered_proxy);
      this->State.ClearExtension(PXMRegistrationState::registered_proxy);

      for (int cc = 0; cc < nbRegisteredProxy; ++cc)
      {
        const PXMRegistrationState_Entry* reg =
          &backup.GetExtension(PXMRegistrationState::registered_proxy, cc);

        if (reg->group() == groupString && reg->name() == nameString &&
          reg->global_id() == proxy->GetGlobalID())
        {
          // Do not keep it
        }
        else
        {
          // Keep it
          this->State.AddExtension(PXMRegistrationState::registered_proxy)->CopyFrom(*reg);
        }
      }
    }

    return found;
  }

  // --------------------------------------------------------------------------
  void UpdateProxySelectionModelState()
  {
    this->State.ClearExtension(PXMRegistrationState::registered_selection_model);
    std::map<std::string, vtkSmartPointer<vtkSMProxySelectionModel> >::iterator iter;
    for (iter = this->SelectionModels.begin(); iter != this->SelectionModels.end(); iter++)
    {
      PXMRegistrationState_Entry* selModel =
        this->State.AddExtension(PXMRegistrationState::registered_selection_model);
      selModel->set_name(iter->first);
      selModel->set_global_id(iter->second->GetGlobalID());
    }
  }
  // --------------------------------------------------------------------------
  void UpdateLinkState()
  {
    this->State.ClearExtension(PXMRegistrationState::registered_link);

    LinkType::iterator iter;
    for (iter = this->RegisteredLinkMap.begin(); iter != this->RegisteredLinkMap.end(); iter++)
    {
      PXMRegistrationState_Entry* linkEntry =
        this->State.AddExtension(PXMRegistrationState::registered_link);
      linkEntry->set_name(iter->first);
      linkEntry->set_global_id(iter->second->GetGlobalID());
    }
  }
  // --------------------------------------------------------------------------
};

#endif

// VTK-HeaderTest-Exclude: vtkSMSessionProxyManagerInternals.h
