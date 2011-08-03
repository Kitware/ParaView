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

#ifndef __vtkSMProxyManagerInternals_h
#define __vtkSMProxyManagerInternals_h

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMLink.h"
#include "vtkSMMessage.h"
#include "vtkSMOutputPort.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSession.h"
#include "vtkStdString.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>

// Sub-classed to avoid symbol length explosion.
class vtkSMProxyManagerProxyInfo : public vtkObjectBase
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;
  unsigned long ModifiedObserverTag;
  unsigned long StateChangedObserverTag;
  unsigned long UpdateObserverTag;
  unsigned long UpdateInformationObserverTag;

  static vtkSMProxyManagerProxyInfo* New()
    {
    return new vtkSMProxyManagerProxyInfo();
    }

  // Description:
  // this needs to be overridden otherwise vtkDebugLeaks warnings are objects
  // are destroyed.
  void UnRegister()
    {
    int refcount = this->GetReferenceCount()-1;
    this->SetReferenceCount(refcount);
    if (refcount <= 0)
      {
#ifdef VTK_DEBUG_LEAKS
      vtkDebugLeaks::DestructClass("vtkSMProxyManagerProxyInfo");
#endif
      delete this;
      }
    }
  virtual void UnRegister(vtkObjectBase *)
    { this->UnRegister(); }

private:
  vtkSMProxyManagerProxyInfo()
    {
    this->ModifiedObserverTag = 0;
    this->StateChangedObserverTag = 0;
    this->UpdateObserverTag = 0;
    this->UpdateInformationObserverTag = 0;
    }
  ~vtkSMProxyManagerProxyInfo()
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
      this->Proxy.GetPointer()->RemoveObserver(
        this->UpdateInformationObserverTag);
      this->UpdateInformationObserverTag = 0;
      }
    }
};

//-----------------------------------------------------------------------------
class vtkSMProxyManagerProxyListType :
  public vtkstd::vector<vtkSmartPointer<vtkSMProxyManagerProxyInfo> >
{
public:
  // Returns if the proxy exists in  this vector.
  bool Contains(vtkSMProxy* proxy) 
    {
    vtkSMProxyManagerProxyListType::iterator iter =
      this->begin();
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
    vtkSMProxyManagerProxyListType::iterator iter =
      this->begin();
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
class vtkSMProxyManagerProxyMapType:
  public vtkstd::map<vtkStdString, vtkSMProxyManagerProxyListType> {};
//-----------------------------------------------------------------------------
struct vtkSMProxyManagerEntry
{
  vtkstd::string Group;
  vtkstd::string Name;
  vtkSmartPointer<vtkSMProxy> Proxy;

  vtkSMProxyManagerEntry(const char* group, const char* name, vtkSMProxy* proxy)
    {
    this->Group = group;
    this->Name = name;
    this->Proxy = proxy;
    }

  bool operator==(const vtkSMProxyManagerEntry &other) const {
    return this->Group == other.Group && this->Name == other.Name
        && this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID();
  }

  bool operator<(const vtkSMProxyManagerEntry &other) const {
    if(this->Proxy->GetGlobalID() < other.Proxy->GetGlobalID())
      {
      return true;
      }
    else if( this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID() &&
             this->Group == other.Group)
      {
      return this->Name < other.Name;
      }
    else if (this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID())
      {
      return this->Group < other.Group;
      }
    return false;
  }

  bool operator>(const vtkSMProxyManagerEntry &other) const {
    if(this->Proxy->GetGlobalID() > other.Proxy->GetGlobalID())
      {
      return true;
      }
    else if( this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID() &&
             this->Group == other.Group)
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

struct vtkSMProxyManagerInternals
{
  // This data structure stores actual proxy instances grouped in
  // collections.
  typedef
  vtkstd::map<vtkStdString, vtkSMProxyManagerProxyMapType> ProxyGroupType;
  ProxyGroupType RegisteredProxyMap;

  // This data structure stores the tuples(group, name, proxy) to compute
  // diff when a state is loaded.
  typedef
  vtkstd::set<vtkSMProxyManagerEntry> GroupNameProxySet;
  GroupNameProxySet RegisteredProxyTuple;

  // This data structure stores a set of proxies that have been modified.
  typedef vtkstd::set<vtkSMProxy*> SetOfProxies;
  SetOfProxies ModifiedProxies;

  // Data structure to save registered links.
  typedef vtkstd::map<vtkStdString, vtkSmartPointer<vtkSMLink> >
    LinkType;
  LinkType RegisteredLinkMap;

  // Data structure for selection models.
  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkSMProxySelectionModel> >
    SelectionModelsType;
  SelectionModelsType SelectionModels;

  // Data structure for storing GlobalPropertiesManagers.
  typedef vtkstd::map<vtkstd::string,
          vtkSmartPointer<vtkSMGlobalPropertiesManager> >
            GlobalPropertiesManagersType;
  typedef vtkstd::map<vtkstd::string,
          unsigned long >
            GlobalPropertiesManagersCallBackIDType;
  GlobalPropertiesManagersType GlobalPropertiesManagers;
  GlobalPropertiesManagersCallBackIDType GlobalPropertiesManagersCallBackID;

  // Data structure for storing the fullState
  vtkSMMessage State;

  // Keep ref to the proxyManager to access the session
  vtkSMProxyManager *ProxyManager;

  // Helper methods -----------------------------------------------------------
  void FindProxyTuples(vtkSMProxy* proxy,
                       vtkstd::set<vtkSMProxyManagerEntry> &tuplesFounds)
    {
    vtkstd::set<vtkSMProxyManagerEntry>::iterator iter;
    iter = this->RegisteredProxyTuple.begin();
    while(iter != this->RegisteredProxyTuple.end())
      {
      if(iter->Proxy == proxy)
        {
        tuplesFounds.insert(*iter);
        }
      iter++;
      }
    }

  // --------------------------------------------------------------------------
  void ComputeDelta(const vtkSMMessage* newState,
                    vtkSMProxyLocator* locator,
                    vtkstd::set<vtkSMProxyManagerEntry> &toRegister,
                    vtkstd::set<vtkSMProxyManagerEntry> &toUnregister)
    {
    // Fill equivalent temporary data structure
    vtkstd::set<vtkSMProxyManagerEntry> newStateContent;
    int max = newState->ExtensionSize(PXMRegistrationState::registered_proxy);
    for(int cc=0; cc < max; cc++)
      {
      PXMRegistrationState_Entry reg =
          newState->GetExtension(PXMRegistrationState::registered_proxy, cc);
      vtkSMProxy *proxy = locator->LocateProxy(reg.global_id());
      if(proxy)
        {
        newStateContent.insert(vtkSMProxyManagerEntry(reg.group().c_str(), reg.name().c_str(), proxy));
        }
      }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // FIXME there might be a better way to do that
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // Look for proxy to Register
    vtkstd::set<vtkSMProxyManagerEntry>::iterator iter;
    iter = newStateContent.begin();
    while(iter != newStateContent.end())
      {
      if(this->RegisteredProxyTuple.find(*iter) == this->RegisteredProxyTuple.end())
        {
        toRegister.insert(*iter);
        }
      iter++;
      }

    // Look for proxy to Unregister
    iter = this->RegisteredProxyTuple.begin();
    vtksys::RegularExpression prototypesRe("_prototypes$");
    while(iter != this->RegisteredProxyTuple.end())
      {
      if( newStateContent.find(*iter) == newStateContent.end() &&
          !prototypesRe.find(iter->Group.c_str()))
        {
        toUnregister.insert(*iter);
        }
      iter++;
      }
    }

  // --------------------------------------------------------------------------
  void RemoveTuples( const char* name,
                     vtkstd::set<vtkSMProxyManagerEntry> &removedEntries)
    {
    // Convert param
    vtkstd::string nameString = name;

    // Deal with set
    GroupNameProxySet resultSet;
    vtkstd::set<vtkSMProxyManagerEntry>::iterator iter;
    iter = this->RegisteredProxyTuple.begin();
    while(iter != this->RegisteredProxyTuple.end())
      {
      if(iter->Name != nameString)
        {
        resultSet.insert(*iter);
        }
      iter++;
      }
    this->RegisteredProxyTuple = resultSet;

    // Deal with map only (true)
    vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
        this->RegisteredProxyMap.begin();
    for (; it != this->RegisteredProxyMap.end(); it++)
      {
      vtkSMProxyManagerProxyMapType::iterator it2 =
        it->second.find(name);
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
    for(int cc=0; cc < nbRegisteredProxy; ++cc)
      {
      const PXMRegistrationState_Entry *reg =
          &backup.GetExtension(PXMRegistrationState::registered_proxy, cc);

      if( reg->name() != nameString ) // Keep it
        {
        this->State.AddExtension(PXMRegistrationState::registered_proxy)->CopyFrom(*reg);
        }
      }
    }

  // --------------------------------------------------------------------------
  void RemoveTuples(const char* group, const char* name,
                    vtkstd::set<vtkSMProxyManagerEntry> &removedEntries,
                    bool doMapOnly)
    {
    // Convert parameters
    vtkstd::string groupString = group;
    vtkstd::string nameString = name;

    // Deal with set
    if(!doMapOnly)
      {
      GroupNameProxySet resultSet;
      vtkstd::set<vtkSMProxyManagerEntry>::iterator iter;
      iter = this->RegisteredProxyTuple.begin();
      while(iter != this->RegisteredProxyTuple.end())
        {
        if(iter->Group != groupString || iter->Name != nameString)
          {
          resultSet.insert(*iter);
          }
        iter++;
        }
      this->RegisteredProxyTuple = resultSet;
      }

    // Deal with map
    vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
        this->RegisteredProxyMap.find(group);
    if ( it != this->RegisteredProxyMap.end() )
      {
      vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
      if (it2 != it->second.end())
        {
        vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
        while(it3 != it2->second.end())
          {
          removedEntries.insert(vtkSMProxyManagerEntry( group, name,
                                                        (*it3)->Proxy));
          it3++;
          }
        it->second.erase(it2);
        }
      }

    // Deal with state
    vtksys::RegularExpression prototypesRe("_prototypes$");
    if(!doMapOnly && !prototypesRe.find(group))
      {
      vtkSMMessage backup;
      backup.CopyFrom(this->State);
      int nbRegisteredProxy =
          this->State.ExtensionSize(PXMRegistrationState::registered_proxy);
      this->State.ClearExtension(PXMRegistrationState::registered_proxy);
      for(int cc=0; cc < nbRegisteredProxy; ++cc)
        {
        const PXMRegistrationState_Entry *reg =
            &backup.GetExtension(PXMRegistrationState::registered_proxy, cc);

        if( reg->group() != groupString || reg->name() != nameString ) // Keep it
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
    vtkstd::string groupString = group;
    vtkstd::string nameString = name;

    // Will be overriden if the proxy is found
    bool found = false;

    // Deal with set
    this->RegisteredProxyTuple.erase(vtkSMProxyManagerEntry(group,name,proxy));

    // Deal with map
    vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
        this->RegisteredProxyMap.find(group);
    if ( it != this->RegisteredProxyMap.end() )
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


      for(int cc=0; cc < nbRegisteredProxy; ++cc)
        {
        const PXMRegistrationState_Entry *reg =
            &backup.GetExtension(PXMRegistrationState::registered_proxy, cc);

        if( reg->group() ==  groupString && reg->name() == nameString
            && reg->global_id() == proxy->GetGlobalID() )
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
    vtkstd::map<vtkstd::string, vtkSmartPointer<vtkSMProxySelectionModel> >::iterator iter;
    for(iter  = this->SelectionModels.begin();
        iter != this->SelectionModels.end();
        iter++)
      {
      PXMRegistrationState_Entry *selModel =
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
    for(iter  = this->RegisteredLinkMap.begin();
        iter != this->RegisteredLinkMap.end();
        iter++)
      {
      PXMRegistrationState_Entry *linkEntry =
          this->State.AddExtension(PXMRegistrationState::registered_link);
      linkEntry->set_name(iter->first);
      linkEntry->set_global_id(iter->second->GetGlobalID());
      }
    }
  // --------------------------------------------------------------------------
};

#endif

