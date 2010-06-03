/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyManager.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h" // for PARAVIEW_VERSION_*
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxyDefinitionLoader.h"
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMDocumentation.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyDefinitionIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMStateLoader.h"
#include "vtkSMUndoStack.h"
#include "vtkSMWriterFactory.h"
#include "vtkSMXMLParser.h"
#include "vtkStdString.h"
#include "vtkStringList.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtksys/DateStamp.h> // For date stamp
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>

#include "vtkSMProxyManagerInternals.h"

#if 0 // for debugging
class vtkSMProxyRegObserver : public vtkCommand
{
public:
  virtual void Execute(vtkObject*, unsigned long event, void* data)
    {
      vtkSMProxyManager::RegisteredProxyInformation* info =
        (vtkSMProxyManager::RegisteredProxyInformation*)data;
      cout << info->Proxy
           << " " << vtkCommand::GetStringFromEventId(event)
           << " " << info->GroupName
           << " " << info->ProxyName
           << endl;
    }
};
#endif


#define PARAVIEW_SOURCE_VERSION "paraview version " PARAVIEW_VERSION_FULL ", Date: " vtksys_DATE_STAMP_STRING

class vtkSMProxyManagerProxySet : public vtkstd::set<vtkSMProxy*> {};

//*****************************************************************************
class vtkSMProxyManagerObserver : public vtkCommand
{
public:
  static vtkSMProxyManagerObserver* New()
    { return new vtkSMProxyManagerObserver(); }

  void SetTarget(vtkSMProxyManager* t)
    {
    this->Target = t;
    }

  virtual void Execute(vtkObject *obj, unsigned long event, void* data)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(obj, event, data);
      }
    }

protected:
  vtkSMProxyManagerObserver()
    {
    this->Target = 0;
    }
  vtkSMProxyManager* Target;
};

//*****************************************************************************
vtkStandardNewMacro(vtkSMProxyManager);
//---------------------------------------------------------------------------
vtkSMProxyManager::vtkSMProxyManager()
{
  this->UpdateInputProxies = 0;
  this->Internals = new vtkSMProxyManagerInternals;
  this->Observer = vtkSMProxyManagerObserver::New();
  this->Observer->SetTarget(this);
#if 0 // for debugging
  vtkSMProxyRegObserver* obs = new vtkSMProxyRegObserver;
  this->AddObserver(vtkCommand::RegisterEvent, obs);
  this->AddObserver(vtkCommand::UnRegisterEvent, obs);
#endif

  this->ReaderFactory = vtkSMReaderFactory::New();
  this->WriterFactory = vtkSMWriterFactory::New();
  this->ProxyDefinitionsUpdated = false;
}

//---------------------------------------------------------------------------
vtkSMProxyManager::~vtkSMProxyManager()
{
  this->UnRegisterProxies();
  delete this->Internals;

  this->Observer->SetTarget(0);
  this->Observer->Delete();

  this->ReaderFactory->Delete();
  this->ReaderFactory = 0;

  this->WriterFactory->Delete();
  this->WriterFactory = 0;
}

//----------------------------------------------------------------------------
const char* vtkSMProxyManager::GetParaViewSourceVersion()
{
  return PARAVIEW_SOURCE_VERSION;
}

//----------------------------------------------------------------------------
int vtkSMProxyManager::GetVersionMajor()
{
  return PARAVIEW_VERSION_MAJOR;
}

//----------------------------------------------------------------------------
int vtkSMProxyManager::GetVersionMinor()
{
  return PARAVIEW_VERSION_MINOR;
}

//----------------------------------------------------------------------------
int vtkSMProxyManager::GetVersionPatch()
{
  return PARAVIEW_VERSION_PATCH;
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::InstantiateGroupPrototypes(const char* groupName)
{
  if (!groupName)
    {
    return;
    }

  vtksys_ios::ostringstream newgroupname;
  newgroupname << groupName << "_prototypes" << ends;
  // Find the XML elements from which the proxies can be instantiated and
  // initialized
  vtkSMProxyManagerInternals::GroupMapType::iterator it =
    this->Internals->GroupMap.find(groupName);
  if (it != this->Internals->GroupMap.end())
    {
    vtkSMProxyManagerElementMapType::iterator it2 =
      it->second.begin();

    for(; it2 != it->second.end(); it2++)
      {
      vtkPVXMLElement* element = it2->second.GetPointer();
      if (!this->GetProxy(newgroupname.str().c_str(), it2->first.c_str()))
        {
        vtkSMProxy* proxy = this->NewProxy(element, groupName, it2->first.c_str());
        if (proxy)
          {
          proxy->SetConnectionID(
            vtkProcessModuleConnectionManager::GetNullConnectionID());
          this->RegisterProxy(newgroupname.str().c_str(), it2->first.c_str(), proxy);
          proxy->Delete();
          }
        }
      }

    }
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::InstantiatePrototypes()
{
  vtkSMProxyManagerInternals::GroupMapType::iterator it =
    this->Internals->GroupMap.begin();
  for (; it != this->Internals->GroupMap.end(); ++it)
    {
    this->InstantiateGroupPrototypes(it->first.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::AddElement(const char* groupName,
                                   const char* name,
                                   vtkPVXMLElement* element)
{
  vtkSMProxyManagerElementMapType& elementMap =
    this->Internals->GroupMap[groupName];

  if (element->GetName() && strcmp(element->GetName(), "Extension") == 0)
    {
    // This is an extension for an existing definition.
    vtkSMProxyManagerElementMapType::iterator iter = elementMap.find(name);
    if (iter == elementMap.end())
      {
      vtkWarningMacro("Extension for (" << groupName << ", " << name
        << ") ignored since could not find core definition.");
      return;
      }
    for (unsigned int cc=0; cc < element->GetNumberOfNestedElements(); cc++)
      {
      iter->second->AddNestedElement(element->GetNestedElement(cc));
      }
    }
  else
    {
    elementMap[name] = element;
    }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::NewProxy(
  const char* groupName, const char* proxyName)
{
  if (!groupName || !proxyName)
    {
    return 0;
    }
  // Find the XML element from which the proxy can be instantiated and
  // initialized
  vtkPVXMLElement* element = this->GetProxyElement(groupName,
    proxyName);
  if (element)
    {
    return this->NewProxy(element, groupName, proxyName);
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::NewProxy(vtkPVXMLElement* pelement,
                                        const char* groupname,
                                        const char* proxyname)
{
  if (strcmp(pelement->GetName(), "CompoundSourceProxy") == 0)
    {
    vtkSMCompoundProxyDefinitionLoader* loader =
      vtkSMCompoundProxyDefinitionLoader::New();
    vtkSMCompoundSourceProxy* cproxy = loader->LoadDefinition(pelement);
    loader->Delete();
    if (cproxy)
      {
      cproxy->SetXMLName(proxyname);
      cproxy->SetXMLGroup(groupname);
      }
    return cproxy;
    }

  vtkObject* object = 0;
  vtksys_ios::ostringstream cname;
  cname << "vtkSM" << pelement->GetName() << ends;
  object = vtkInstantiator::CreateInstance(cname.str().c_str());

  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(object);
  if (proxy)
    {
    proxy->ReadXMLAttributes(this, pelement);
    proxy->SetXMLName(proxyname);
    proxy->SetXMLGroup(groupname);
    }
  else
    {
    vtkWarningMacro("Creation of new proxy " << cname.str() << " failed ("
                    << groupname << ", " << proxyname << ").");
    }
  return proxy;
}


//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMProxyManager::GetProxyDocumentation(
  const char* groupName, const char* proxyName)
{
  if (!groupName || !proxyName)
    {
    return 0;
    }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  return proxy? proxy->GetDocumentation() : NULL;
}

//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMProxyManager::GetPropertyDocumentation(
  const char* groupName, const char* proxyName, const char* propertyName)
{
  if (!groupName || !proxyName || !propertyName)
    {
    return 0;
    }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  if (proxy)
    {
    vtkSMProperty* prop = proxy->GetProperty(propertyName);
    if (prop)
      {
      return prop->GetDocumentation();
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkSMProxyManager::ProxyElementExists(const char* groupName,
                                          const char* proxyName)
{
  if (!groupName || !proxyName)
    {
    return 0;
    }
  // Find the XML element from the proxy.
  //
  vtkSMProxyManagerInternals::GroupMapType::iterator it =
    this->Internals->GroupMap.find(groupName);
  if (it != this->Internals->GroupMap.end())
    {
    vtkSMProxyManagerElementMapType::iterator it2 =
      it->second.find(proxyName);

    if (it2 != it->second.end())
      {
      vtkPVXMLElement* element = it2->second.GetPointer();
      if (element)
        {
        return 1;
        }
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyManager::GetProxyElement(const char* groupName,
                                                    const char* proxyName)
{
  vtkPVXMLElement* element = this->Internals->GetProxyElement(groupName, proxyName);
  if (element)
    {
    return element;
    }

  vtkErrorMacro( << "No proxy that matches: group=" << groupName
                 << " and proxy=" << proxyName << " were found.");
  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyManager::GetNumberOfXMLGroups()
{
  return this->Internals->GroupMap.size();
}

//---------------------------------------------------------------------------
const char* vtkSMProxyManager::GetXMLGroupName(unsigned int n)
{
  unsigned int idx;
  vtkSMProxyManagerInternals::GroupMapType::iterator it =
    this->Internals->GroupMap.begin();
  for (idx=0;
       it != this->Internals->GroupMap.end() && idx < n;
       it++)
    {
    idx++;
    }

  if (idx == n && it != this->Internals->GroupMap.end())
    {
    return it->first.c_str();
    }
  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyManager::GetNumberOfXMLProxies(const char* groupName)
{
  vtkSMProxyManagerInternals::GroupMapType::iterator it =
    this->Internals->GroupMap.find(groupName);
  if (it != this->Internals->GroupMap.end())
    {
    return it->second.size();
    }
  return 0;
}


//---------------------------------------------------------------------------
const char* vtkSMProxyManager::GetXMLProxyName(const char* groupName,
  unsigned int n)
{
  vtkSMProxyManagerInternals::GroupMapType::iterator it =
    this->Internals->GroupMap.find(groupName);
  if (it != this->Internals->GroupMap.end())
    {
    vtkSMProxyManagerElementMapType::iterator it2 =
      it->second.begin();
    unsigned int idx;
    for (idx=0;
      it2 != it->second.end() && idx < n;
      it2++)
      {
      idx++;
      }
    if (idx == n && it2 != it->second.end())
      {
      return it2->first.c_str();
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyManager::GetNumberOfProxies(const char* group)
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    int size = 0;
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); ++it2)
      {
      size += it2->second.size();
      }
    return size;
    }
  return 0;
}

//---------------------------------------------------------------------------
// No errors are raised if a proxy definition for the requested proxy is not
// found.
vtkSMProxy* vtkSMProxyManager::GetPrototypeProxy(const char* groupname,
  const char* name)
{
  vtkstd::string protype_group = groupname;
  protype_group += "_prototypes";
  vtkSMProxy* proxy = this->GetProxy(protype_group.c_str(), name);
  if (proxy)
    {
    return proxy;
    }

  if (!this->Internals->GetProxyElement(groupname, name))
    {
    // No definition was located for the requested proxy.
    // Cannot create the prototype.
    return 0;
    }

  proxy = this->NewProxy(groupname, name);
  if (!proxy)
    {
    return 0;
    }
  proxy->SetConnectionID(
    vtkProcessModuleConnectionManager::GetNullConnectionID());
  // register the proxy as a prototype.
  this->RegisterProxy(protype_group.c_str(), name, proxy);
  proxy->Delete();
  return proxy;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::GetProxy(const char* group, const char* name)
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.find(name);
    if (it2 != it->second.end())
      {
      if (it2->second.begin() != it2->second.end())
        {
        return it2->second.front()->Proxy.GetPointer();
        }
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::GetProxy(const char* groupname, int id)
{
  vtkClientServerID cid(id);
  return this->GetProxy(groupname, cid);
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::GetProxy(const char* groupname, vtkClientServerID id)
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if (it != this->Internals->RegisteredProxyMap.end())
    {
    vtkSMProxyManagerProxyMapType::iterator it2;
    for (it2 = it->second.begin(); it2 != it->second.end(); it2++)
      {
      vtkSMProxyManagerProxyListType::iterator it3;
      for (it3 = it2->second.begin();it3 != it2->second.end(); it3++)
        {
        if (it3->GetPointer()->Proxy->GetSelfID() == id)
          {
          return it3->GetPointer()->Proxy;
          }
        }
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::GetProxy(const char* name)
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.find(name);
    if (it2 != it->second.end())
      {
      if (it2->second.begin() != it2->second.end())
        {
        return it2->second.front()->Proxy.GetPointer();
        }
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::GetProxy(vtkIdType vtkNotUsed(connectionID),
  vtkClientServerID id)
{
  return vtkSMProxy::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetObjectFromID(id));
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::GetProxy(vtkIdType connectionID, int id)
{
  vtkClientServerID cid;
  cid.ID = id;
  return this->GetProxy(connectionID, cid);
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::GetProxies(const char* group,
  const char* name, vtkCollection* collection)
{
  collection->RemoveAllItems();
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if(it != this->Internals->RegisteredProxyMap.end())
    {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
    if (it2 != it->second.end())
      {
      vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
      for (; it3 != it2->second.end(); ++it3)
        {
        collection->AddItem(it3->GetPointer()->Proxy);
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::GetProxyNames(const char* groupname,
                                      vtkSMProxy* proxy, vtkStringList* names)
{
  if (!names)
    {
    return;
    }
  names->RemoveAllItems();

  if (!groupname || !proxy)
    {
    return;
    }

  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      if (it2->second.Contains(proxy))
        {
        names->AddString(it2->first.c_str());
        }
      }
    }
}


//---------------------------------------------------------------------------
const char* vtkSMProxyManager::GetProxyName(const char* groupname,
                                            vtkSMProxy* proxy)
{
  if (!groupname || !proxy)
    {
    return 0;
    }

  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      if (it2->second.Contains(proxy))
        {
        return it2->first.c_str();
        }
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMProxyManager::GetProxyName(const char* groupname,
                                            unsigned int idx)
{
  if (!groupname)
    {
    return 0;
    }

  unsigned int counter=0;

  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      if (counter == idx)
        {
        return it2->first.c_str();
        }
      counter++;
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
const char* vtkSMProxyManager::IsProxyInGroup(vtkSMProxy* proxy,
                                              const char* groupname)
{
  if (!proxy || !groupname)
    {
    return 0;
    }
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      if (it2->second.Contains(proxy))
        {
        return it2->first.c_str();
        }
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterProxies()
{
  this->Internals->RegisteredProxyMap.erase(
    this->Internals->RegisteredProxyMap.begin(),
    this->Internals->RegisteredProxyMap.end());
  this->Internals->ModifiedProxies.clear();
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterProxy(const char* group, const char* name,
  vtkSMProxy* proxy)
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
    if (it2 != it->second.end())
      {
      vtkSMProxyManagerProxyListType::iterator it3 = it2->second.Find(proxy);
      if (it3 != it2->second.end())
        {
        RegisteredProxyInformation info;
        info.Proxy = it3->GetPointer()->Proxy;
        info.GroupName = it->first.c_str();
        info.ProxyName = it2->first.c_str();
        info.Type = RegisteredProxyInformation::PROXY;

        this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
        this->UnMarkProxyAsModified(info.Proxy);
        it2->second.erase(it3);
        }
      if (it2->second.size() == 0)
        {
        it->second.erase(it2);
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterProxy(const char* group, const char* name)
{
  // Legacy API, unregister only the first one in that group.
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.find(name);
    if (it2 != it->second.end())
      {
      if (it2->second.size() > 0)
        {
        vtkSMProxyManagerProxyListType::iterator it3 =
          it2->second.begin();

        RegisteredProxyInformation info;
        info.Proxy = it3->GetPointer()->Proxy;
        info.GroupName = it->first.c_str();
        info.ProxyName = it2->first.c_str();
        info.Type = RegisteredProxyInformation::PROXY;

        this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
        this->UnMarkProxyAsModified(info.Proxy);
        it2->second.erase(it3);
        }
      if (it2->second.size() == 0)
        {
        it->second.erase(it2);
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterProxy(const char* name)
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.find(name);
    if (it2 != it->second.end())
      {
      this->UnRegisterProxy(it->first.c_str(), name);
      }
    }

}

//---------------------------------------------------------------------------
struct vtkSMProxyManagerProxyInformation
{
  vtkstd::string GroupName;
  vtkstd::string ProxyName;
  vtkSMProxy* Proxy;
};
//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterProxy(vtkSMProxy* proxy)
{
  vtkstd::vector<vtkSMProxyManagerProxyInformation> toUnRegister;

  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    vtkSMProxyManagerProxyMapType::iterator it2;
    for (it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      {
      if (it2->second.Contains(proxy))
        {
        vtkSMProxyManagerProxyInformation info;
        info.GroupName = it->first;
        info.ProxyName = it2->first;
        toUnRegister.push_back(info);
        }
      }
    }

  vtkstd::vector<vtkSMProxyManagerProxyInformation>::iterator vIter =
    toUnRegister.begin();
  for (;vIter != toUnRegister.end(); ++vIter)
    {
    this->UnRegisterProxy(vIter->GroupName.c_str(), vIter->ProxyName.c_str(),
      proxy);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterProxies(vtkIdType cid)
{
  vtkstd::vector<vtkSMProxyManagerProxyInformation> toUnRegister;
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();
  iter->SetModeToAll();
  iter->SetConnectionID(cid);

  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxyManagerProxyInformation info;
    info.GroupName = iter->GetGroup();
    info.ProxyName = iter->GetKey();
    info.Proxy = iter->GetProxy();
    toUnRegister.push_back(info);
    }
  iter->Delete();

  vtkstd::vector<vtkSMProxyManagerProxyInformation>::iterator vIter =
    toUnRegister.begin();
  for (;vIter != toUnRegister.end(); ++vIter)
    {
    this->UnRegisterProxy(vIter->GroupName.c_str(), vIter->ProxyName.c_str(),
      vIter->Proxy);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::RegisterProxy(const char* groupname,
                                      const char* name,
                                      vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return;
    }

  vtkSMProxyManagerProxyListType &proxy_list =
    this->Internals->RegisteredProxyMap[groupname][name];
  if (proxy_list.Contains(proxy))
    {
    return;
    }

  vtkSMProxyManagerProxyInfo* proxyInfo = vtkSMProxyManagerProxyInfo::New();
  proxy_list.push_back(proxyInfo);
  proxyInfo->Delete();

  proxyInfo->Proxy = proxy;
  // Add observers to note proxy modification.
  proxyInfo->ModifiedObserverTag = proxy->AddObserver(
    vtkCommand::PropertyModifiedEvent, this->Observer);
  proxyInfo->StateChangedObserverTag = proxy->AddObserver(
    vtkCommand::StateChangedEvent, this->Observer);
  proxyInfo->UpdateObserverTag = proxy->AddObserver(vtkCommand::UpdateEvent,
    this->Observer);
  proxyInfo->UpdateInformationObserverTag = proxy->AddObserver(
    vtkCommand::UpdateInformationEvent, this->Observer);

  // Note, these observer will be removed in the destructor of proxyInfo.

  RegisteredProxyInformation info;
  info.Proxy = proxy;
  info.GroupName = groupname;
  info.ProxyName = name;
  info.Type = RegisteredProxyInformation::PROXY;

  this->InvokeEvent(vtkCommand::RegisterEvent, &info);
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UpdateRegisteredProxies(const char* groupname,
  int modified_only /*=1*/)
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
      for (; it3 != it2->second.end(); ++it3)
        {
        // Check is proxy is in the modified set.
        if (!modified_only ||
          this->Internals->ModifiedProxies.find(it3->GetPointer()->Proxy.GetPointer())
          != this->Internals->ModifiedProxies.end())
          {
          it3->GetPointer()->Proxy.GetPointer()->UpdateVTKObjects();
          }
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UpdateRegisteredProxies(int modified_only /*=1*/)
{
  vtksys::RegularExpression prototypesRe("_prototypes$");

  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    if (prototypesRe.find(it->first))
      {
      // skip the prototypes.
      continue;
      }

    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      // Check is proxy is in the modified set.
      vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
      for (; it3 != it2->second.end(); ++it3)
        {
        // Check is proxy is in the modified set.
        if (!modified_only ||
          this->Internals->ModifiedProxies.find(it3->GetPointer()->Proxy.GetPointer())
          != this->Internals->ModifiedProxies.end())
          {
          vtksys_ios::ostringstream log;
          log << "Updating Proxy: " << it3->GetPointer()->Proxy.GetPointer() << "--("
            << it3->GetPointer()->Proxy->GetXMLGroup()
            << it3->GetPointer()->Proxy->GetXMLName()
            << ")";
          vtkProcessModule::DebugLog(log.str().c_str());
          it3->GetPointer()->Proxy.GetPointer()->UpdateVTKObjects();
          }
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UpdateRegisteredProxiesInOrder(int modified_only/*=1*/)
{
  this->UpdateInputProxies = 1;
  this->UpdateRegisteredProxies(modified_only);
  this->UpdateInputProxies = 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UpdateProxyInOrder(vtkSMProxy* proxy)
{
  this->UpdateInputProxies = 1;
  proxy->UpdateVTKObjects();
  this->UpdateInputProxies = 0;
}

//---------------------------------------------------------------------------
int vtkSMProxyManager::GetNumberOfLinks()
{
  return this->Internals->RegisteredLinkMap.size();
}

//---------------------------------------------------------------------------
const char* vtkSMProxyManager::GetLinkName(int idx)
{
  int numlinks = this->GetNumberOfLinks();
  if(idx >= numlinks)
    {
    return NULL;
    }
  vtkSMProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.begin();
  for(int i=0; i<idx; i++)
    {
    it++;
    }
  return it->first.c_str();
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::RegisterLink(const char* name, vtkSMLink* link)
{
  vtkSMProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.find(name);
  if (it != this->Internals->RegisteredLinkMap.end())
    {
    vtkWarningMacro("Replacing previously registered link with name " << name);
    }
  this->Internals->RegisteredLinkMap[name] = link;

  RegisteredProxyInformation info;
  info.Proxy = 0;
  info.GroupName = 0;
  info.ProxyName = name;
  info.Type = RegisteredProxyInformation::LINK;
  this->InvokeEvent(vtkCommand::RegisterEvent, &info);
}

//---------------------------------------------------------------------------
vtkSMLink* vtkSMProxyManager::GetRegisteredLink(const char* name)
{
  vtkSMProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.find(name);
  if (it != this->Internals->RegisteredLinkMap.end())
    {
    return it->second.GetPointer();
    }
  return NULL;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterLink(const char* name)
{
  vtkSMProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.find(name);
  if (it != this->Internals->RegisteredLinkMap.end())
    {
    RegisteredProxyInformation info;
    info.Proxy = 0;
    info.GroupName = 0;
    info.ProxyName = name;
    info.Type = RegisteredProxyInformation::LINK;
    this->Internals->RegisteredLinkMap.erase(it);
    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterAllLinks()
{
  this->Internals->RegisteredLinkMap.clear();
}


//---------------------------------------------------------------------------
void vtkSMProxyManager::ExecuteEvent(vtkObject* obj, unsigned long event,
  void* data)
{
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(obj);
  if (!proxy)
    {
    // We are only interested in proxy events.
    return;
    }

  switch (event)
    {
  case vtkCommand::PropertyModifiedEvent:
      {
      // Some property on the proxy has been modified.
      this->MarkProxyAsModified(proxy);
      ModifiedPropertyInformation info;
      info.Proxy = proxy;
      info.PropertyName = reinterpret_cast<const char*>(data);
      if (info.PropertyName)
        {
        this->InvokeEvent(vtkCommand::PropertyModifiedEvent,
          &info);
        }
      }
    break;

  case vtkCommand::StateChangedEvent:
      {
      // I wonder if I need to mark the proxy modified. Cause when state
      // changes, the properties are pushed as well so ideally we should call
      // MarkProxyAsModified() and then UnMarkProxyAsModified() :).
      // this->MarkProxyAsModified(proxy);

      StateChangedInformation info;
      info.Proxy = proxy;
      info.StateChangeElement = reinterpret_cast<vtkPVXMLElement*>(data);
      if (info.StateChangeElement)
        {
        this->InvokeEvent(vtkCommand::StateChangedEvent, &info);
        }
      }
    break;

  case vtkCommand::UpdateInformationEvent:
    this->InvokeEvent(vtkCommand::UpdateInformationEvent, proxy);
    break;

  case vtkCommand::UpdateEvent:
    // Proxy has been updated.
    this->UnMarkProxyAsModified(proxy);
    break;
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::MarkProxyAsModified(vtkSMProxy* proxy)
{
  this->Internals->ModifiedProxies.insert(proxy);
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnMarkProxyAsModified(vtkSMProxy* proxy)
{
  vtkSMProxyManagerInternals::SetOfProxies::iterator it =
    this->Internals->ModifiedProxies.find(proxy);
  if (it != this->Internals->ModifiedProxies.end())
    {
    this->Internals->ModifiedProxies.erase(it);
    }
}
//---------------------------------------------------------------------------
int vtkSMProxyManager::AreProxiesModified()
{
  return (this->Internals->ModifiedProxies.size() > 0)? 1: 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::LoadState(const char* filename, vtkIdType id,
  vtkSMStateLoader* loader/*=NULL*/)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName(filename);
  parser->Parse();

  this->LoadState(parser->GetRootElement(), id, loader);
  parser->Delete();
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::LoadState(vtkPVXMLElement* rootElement, vtkIdType id,
  vtkSMStateLoader* loader/*=NULL*/)
{
  if (!rootElement)
    {
    return;
    }
  vtkSmartPointer<vtkSMStateLoader> spLoader;

  if (!loader)
    {
    spLoader = vtkSmartPointer<vtkSMStateLoader>::New();
    }
  else
    {
    spLoader = loader;
    }
  spLoader->GetProxyLocator()->SetConnectionID(id);
  if (spLoader->LoadState(rootElement))
    {
    LoadStateInformation info;
    info.RootElement = rootElement;
    info.ProxyLocator = spLoader->GetProxyLocator();
    this->InvokeEvent(vtkCommand::LoadStateEvent, &info);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::LoadState(const char* filename,
  vtkSMStateLoader* loader/*=NULL*/)
{
  this->LoadState(filename,
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
    loader);
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::LoadState(vtkPVXMLElement* rootElement,
  vtkSMStateLoader* loader/*=NULL*/)
{
  this->LoadState(rootElement,
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
    loader);
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveState(const char* filename)
{
  vtkPVXMLElement* rootElement = this->SaveState();
  ofstream os(filename, ios::out);
  rootElement->PrintXML(os, vtkIndent());
  rootElement->Delete();
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyManager::SaveState()
{
  vtkPVXMLElement* smstate = this->SaveStateInternal(
    vtkProcessModuleConnectionManager::GetNullConnectionID(), 0, 0);

  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("GenericParaViewApplication");
  root->AddNestedElement(smstate);
  smstate->FastDelete();

  LoadStateInformation info;
  info.RootElement = root;
  info.ProxyLocator = NULL;
  this->InvokeEvent(vtkCommand::SaveStateEvent, &info);
  return root;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyManager::SaveState(vtkIdType connectionID)
{
  vtkPVXMLElement* smstate = this->SaveStateInternal(connectionID, 0, 0);

  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("GenericParaViewApplication");
  root->AddNestedElement(smstate);
  smstate->FastDelete();

  LoadStateInformation info;
  info.RootElement = root;
  info.ProxyLocator = NULL;
  this->InvokeEvent(vtkCommand::SaveStateEvent, &info);
  return root;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyManager::SaveRevivalState(vtkIdType connectionID)
{
  return this->SaveStateInternal(connectionID, 0, 1);
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyManager::SaveState(vtkCollection* proxies,
  bool save_referred_proxies)
{
  vtkSMProxyManagerProxySet setOfProxies;
  for (int cc=0; cc < proxies->GetNumberOfItems(); ++cc)
    {
    vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(proxies->GetItemAsObject(cc));
    if (proxy)
      {
      setOfProxies.insert(proxy);
      if (save_referred_proxies)
        {
        this->CollectReferredProxies(setOfProxies, proxy);
        }
      }
    }

  return this->SaveStateInternal(
    vtkProcessModuleConnectionManager::GetNullConnectionID(),
    &setOfProxies, 0);
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::CollectReferredProxies(
  vtkSMProxyManagerProxySet& setOfProxies, vtkSMProxy* proxy)
{
  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(proxy->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      iter->GetProperty());
    for (unsigned int cc=0; pp && (pp->GetNumberOfProxies() > cc); cc++)
      {
      vtkSMProxy* referredProxy = pp->GetProxy(cc);
      if (referredProxy)
        {
        setOfProxies.insert(referredProxy);
        this->CollectReferredProxies(setOfProxies, referredProxy);
        }
      }
    }
}


//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyManager::SaveStateInternal(vtkIdType connectionID,
 vtkSMProxyManagerProxySet* proxySet, int revival)
{
  vtkPVXMLElement* rootElement = vtkPVXMLElement::New();
  rootElement->SetName("ServerManagerState");

  // Set version number on the state element.
  vtksys_ios::ostringstream version_string;
  version_string << this->GetVersionMajor() << "."
    << this->GetVersionMinor() << "." << this->GetVersionPatch();
  rootElement->AddAttribute("version", version_string.str().c_str());


  vtkstd::set<vtkstd::string> seen;
  vtkstd::set<vtkSMProxy*> visited_proxies; // set of proxies already added.

  // First save the state of all proxies
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();

    // Do not save the state of prototypes.
    const char* protstr = "_prototypes";
    const char* colname = it->first.c_str();
    int do_group = 1;
    if (strlen(colname) > strlen(protstr))
      {
      const char* newstr = colname + strlen(colname) -
        strlen(protstr);
      if (strcmp(newstr, protstr) == 0)
        {
        do_group = 0;
        }
      }
    else if ( colname[0] == '_' )
      {
      do_group = 0;
      }
    if (!do_group)
      {
      continue;
      }

    // save the states of all proxies in this group.
    for (; it2 != it->second.end(); it2++)
      {
      vtkSMProxyManagerProxyListType::iterator it3
        = it2->second.begin();
      for (; it3 != it2->second.end(); ++it3)
        {
        if (visited_proxies.find(it3->GetPointer()->Proxy.GetPointer())
          != visited_proxies.end())
          {
          // proxy has been saved.
          continue;
          }
        if (proxySet && proxySet->find(it3->GetPointer()->Proxy.GetPointer()) == proxySet->end())
          {
          // proxy set was specified, and the indicated proxy
          // does not belong to that set.
          continue;
          }
        if (connectionID == vtkProcessModuleConnectionManager::GetNullConnectionID() ||
          it3->GetPointer()->Proxy.GetPointer()->GetConnectionID() == connectionID)
          {
          vtkPVXMLElement* proxyElement =
            it3->GetPointer()->Proxy.GetPointer()->SaveState(rootElement);
          if (revival && proxyElement)
            {
            it3->GetPointer()->Proxy.GetPointer()->SaveRevivalState(proxyElement);
            }
          visited_proxies.insert(it3->GetPointer()->Proxy.GetPointer());
          }
        }
      }
    }

  // Save the proxy collections. This is done seprately because
  // one proxy can be in more than one group.
  it = this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    // Do not save the state of prototypes.
    const char* protstr = "_prototypes";
    int do_group = 1;
    if (strlen(it->first.c_str()) > strlen(protstr))
      {
      const char* newstr = it->first.c_str() + strlen(it->first.c_str()) -
        strlen(protstr);
      if (strcmp(newstr, protstr) == 0)
        {
        do_group = 0;
        }
      }
    if (do_group)
      {
      vtkPVXMLElement* collectionElement = vtkPVXMLElement::New();
      collectionElement->SetName("ProxyCollection");
      collectionElement->AddAttribute("name", it->first.c_str());
      vtkSMProxyManagerProxyMapType::iterator it2 =
        it->second.begin();
      bool some_proxy_added = false;
      for (; it2 != it->second.end(); it2++)
        {
        vtkSMProxyManagerProxyListType::iterator it3 =
          it2->second.begin();
        for (; it3 != it2->second.end(); ++it3)
          {
          if (visited_proxies.find(it3->GetPointer()->Proxy.GetPointer()) != visited_proxies.end())
            {
            vtkPVXMLElement* itemElement = vtkPVXMLElement::New();
            itemElement->SetName("Item");
            itemElement->AddAttribute("id",
              it3->GetPointer()->Proxy.GetPointer()->GetSelfIDAsString());
            itemElement->AddAttribute("name", it2->first.c_str());
            collectionElement->AddNestedElement(itemElement);
            itemElement->Delete();
            some_proxy_added = true;
            }
          }
        }
      // Avoid addition of empty groups.
      if (some_proxy_added)
        {
        rootElement->AddNestedElement(collectionElement);
        }
      collectionElement->Delete();
      }
    }

  // TODO: Save definitions for only referred compound proxy definitions
  // when saving state for subset of proxies.
  vtkPVXMLElement* defs = vtkPVXMLElement::New();
  defs->SetName("CustomProxyDefinitions");
  this->SaveCustomProxyDefinitions(defs);
  rootElement->AddNestedElement(defs);
  defs->Delete();

  // TODO: Save links as per connection ID
  // TODO: What to do with links when saving state for a
  // subset of proxies?
  if (!proxySet)
    {
    vtkPVXMLElement* links = vtkPVXMLElement::New();
    links->SetName("Links");
    this->SaveRegisteredLinks(links);
    rootElement->AddNestedElement(links);
    links->Delete();
    }

  vtkPVXMLElement* globalProps = vtkPVXMLElement::New();
  globalProps->SetName("GlobalPropertiesManagers");
  this->SaveGlobalPropertiesManagers(globalProps);
  rootElement->AddNestedElement(globalProps);
  globalProps->Delete();

  return rootElement;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterCustomProxyDefinitions()
{
  vtkSMProxyManagerInternals::GroupMapType::iterator groupIter =
    this->Internals->GroupMap.begin();
  for ( ;groupIter != this->Internals->GroupMap.end(); ++groupIter)
    {
    vtkSMProxyManagerElementMapType::iterator elemIter=
      groupIter->second.begin();

    for(; elemIter != groupIter->second.end(); )
      {
      if (elemIter->second.Custom)
        {
        vtkSMProxyManagerElementMapType::iterator prev = elemIter;
        elemIter++;
        groupIter->second.erase(prev);
        }
      else
        {
        elemIter++;
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterCustomProxyDefinition(
  const char* group, const char* name)
{
  vtkSMProxyManagerElementMapType& elementMap =
    this->Internals->GroupMap[group];
  vtkSMProxyManagerElementMapType::iterator iter = elementMap.find(name) ;
  if (iter != elementMap.end() && iter->second.Custom)
    {
    RegisteredProxyInformation info;
    info.Proxy = 0;
    info.GroupName = group;
    info.ProxyName = name;
    info.Type = RegisteredProxyInformation::COMPOUND_PROXY_DEFINITION;
    bool prev = this->ProxyDefinitionsUpdated;
    this->ProxyDefinitionsUpdated = true;
    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
    this->ProxyDefinitionsUpdated = prev;
    elementMap.erase(iter);
    return;
    }
  else
    {
    vtkErrorMacro("No custom proxy definition found with group \""
      << group << "\" and name \"" << name << "\".");
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::RegisterCustomProxyDefinition(
  const char* group, const char* name, vtkPVXMLElement* top)
{
  if (!top)
    {
    return;
    }


  vtkSMProxyManagerElementMapType& elementMap =
    this->Internals->GroupMap[group];
  vtkSMProxyManagerElementMapType::iterator iter = elementMap.find(name);
  if (iter != elementMap.end())
    {
    // There's a possibility that the custom proxy definition is the
    // state has already been loaded (or another proxy definition with
    // the same name exists). If that existing definition matches what
    // the state is requesting, we don't need to raise any errors,
    // simply skip it.

    if (!iter->second.DefinitionsMatch(top))
      {
      vtkErrorMacro("Proxy definition has already been registered with name \""
        << name << "\" under group \"" << group <<"\".");
      }
    return;
    }

  elementMap[name] = vtkSMProxyManagerElement(top, true);

  RegisteredProxyInformation info;
  info.Proxy = 0;
  info.GroupName = group;
  info.ProxyName = name;
  info.Type = RegisteredProxyInformation::COMPOUND_PROXY_DEFINITION;
  bool prev = this->ProxyDefinitionsUpdated;
  this->ProxyDefinitionsUpdated = true;
  this->InvokeEvent(vtkCommand::RegisterEvent, &info);
  this->ProxyDefinitionsUpdated = prev;
}

//---------------------------------------------------------------------------
// Does not raise an error if definition is not found.
vtkPVXMLElement* vtkSMProxyManager::GetProxyDefinition(
  const char* group, const char* name)
{
  if (!name || !group)
    {
    return 0;
    }

  return this->Internals->GetProxyElement(group, name);
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::LoadCustomProxyDefinitions(vtkPVXMLElement* root)
{
  if (!root)
    {
    return;
    }

  vtksys::RegularExpression proxyDefRe(".*Proxy$");
  unsigned int numElems = root->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "CustomProxyDefinition") == 0)
      {
      const char* group = currentElement->GetAttribute("group");
      const char* name = currentElement->GetAttribute("name");
      if (name && group)
        {
        if (currentElement->GetNumberOfNestedElements() == 1)
          {
          vtkPVXMLElement* defElement = currentElement->GetNestedElement(0);
          const char* tagName = defElement->GetName();
          if (tagName && proxyDefRe.find(tagName))
            {
            // Register custom proxy definitions for all elements ending with
            // "Proxy".
            this->RegisterCustomProxyDefinition(group, name, defElement);
            }
          }
        }
      else
        {
        vtkErrorMacro("Missing name or group");
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::LoadCustomProxyDefinitions(const char* filename)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName(filename);
  parser->Parse();

  this->LoadCustomProxyDefinitions(parser->GetRootElement());
  parser->Delete();
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveCustomProxyDefinitions(
  vtkPVXMLElement* rootElement)
{
  if (!rootElement)
    {
    vtkErrorMacro("root element must be specified.");
    return;
    }

  vtkSMProxyDefinitionIterator* iter = vtkSMProxyDefinitionIterator::New();
  iter->SetModeToCustomOnly();

  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkPVXMLElement* elem = iter->GetDefinition();
    if (elem)
      {
      vtkPVXMLElement* defElement = vtkPVXMLElement::New();
      defElement->SetName("CustomProxyDefinition");
      defElement->AddAttribute("name", iter->GetKey());
      defElement->AddAttribute("group", iter->GetGroup());
      defElement->AddNestedElement(elem, 0);
      rootElement->AddNestedElement(defElement);
      defElement->Delete();
      }
    }
  iter->Delete();
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveCustomProxyDefinitions(const char* filename)
{
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("CustomProxyDefinitions");
  this->SaveCustomProxyDefinitions(root);

  ofstream os(filename, ios::out);
  root->PrintXML(os, vtkIndent());
  root->Delete();
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveRegisteredLinks(vtkPVXMLElement* rootElement)
{
  vtkSMProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.begin();
  for (; it != this->Internals->RegisteredLinkMap.end(); ++it)
    {
    it->second.GetPointer()->SaveState(it->first.c_str(), rootElement);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveGlobalPropertiesManagers(vtkPVXMLElement* root)
{
  vtkSMProxyManagerInternals::GlobalPropertiesManagersType::iterator iter;
  for (iter = this->Internals->GlobalPropertiesManagers.begin();
    iter != this->Internals->GlobalPropertiesManagers.end(); ++iter)
    {
    vtkPVXMLElement* elem = iter->second->SaveLinkState(root);
    if (elem)
      {
      elem->AddAttribute("name", iter->first.c_str());
      }
    }
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyManager::GetProxyHints(
  const char* groupName, const char* proxyName)
{
  if (!groupName || !proxyName)
    {
    return 0;
    }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  return proxy? proxy->GetHints() : NULL;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyManager::GetPropertyHints(
  const char* groupName, const char* proxyName, const char* propertyName)
{
  if (!groupName || !proxyName || !propertyName)
    {
    return 0;
    }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  if (proxy)
    {
    vtkSMProperty* prop = proxy->GetProperty(propertyName);
    if (prop)
      {
      return prop->GetHints();
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::RegisterSelectionModel(
  const char* name, vtkSMProxySelectionModel* model)
{
  if (!model)
    {
    vtkErrorMacro("Cannot register a null model.");
    return;
    }
  if (!name)
    {
    vtkErrorMacro("Cannot register model with no name.");
    return;
    }

  vtkSMProxySelectionModel* curmodel = this->GetSelectionModel(name);
  if (curmodel && curmodel == model)
    {
    // already registered.
    return;
    }

  if (curmodel)
    {
    vtkWarningMacro("Replacing existing selection model: " << name);
    }
  this->Internals->SelectionModels[name] = model;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterSelectionModel( const char* name)
{
  this->Internals->SelectionModels.erase(name);
}

//---------------------------------------------------------------------------
vtkSMProxySelectionModel* vtkSMProxyManager::GetSelectionModel(
  const char* name)
{
  vtkSMProxyManagerInternals::SelectionModelsType::iterator iter =
    this->Internals->SelectionModels.find(name);
  if (iter == this->Internals->SelectionModels.end())
    {
    return 0;
    }

  return iter->second;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::SetGlobalPropertiesManager(const char* name,
    vtkSMGlobalPropertiesManager* mgr)
{
  vtkSMGlobalPropertiesManager* old_mgr = this->GetGlobalPropertiesManager(name);
  if (old_mgr == mgr)
    {
    return;
    }
  this->RemoveGlobalPropertiesManager(name);
  this->Internals->GlobalPropertiesManagers[name] = mgr;

  RegisteredProxyInformation info;
  info.Proxy = mgr;
  info.GroupName = NULL;
  info.ProxyName = name;
  info.Type = RegisteredProxyInformation::GLOBAL_PROPERTIES_MANAGER;
  this->InvokeEvent(vtkCommand::RegisterEvent, &info);
}

//---------------------------------------------------------------------------
const char* vtkSMProxyManager::GetGlobalPropertiesManagerName(
  vtkSMGlobalPropertiesManager* mgr)
{
  vtkSMProxyManagerInternals::GlobalPropertiesManagersType::iterator iter;
  for (iter = this->Internals->GlobalPropertiesManagers.begin();
    iter != this->Internals->GlobalPropertiesManagers.end(); ++iter)
    {
    if (iter->second == mgr)
      {
      return iter->first.c_str();
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMGlobalPropertiesManager* vtkSMProxyManager::GetGlobalPropertiesManager(
  const char* name)
{
  return this->Internals->GlobalPropertiesManagers[name].GetPointer();
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::RemoveGlobalPropertiesManager(const char* name)
{
  vtkSMGlobalPropertiesManager* gm = this->GetGlobalPropertiesManager(name);
  if (gm)
    {
    RegisteredProxyInformation info;
    info.Proxy = gm;
    info.GroupName = NULL;
    info.ProxyName = name;
    info.Type = RegisteredProxyInformation::GLOBAL_PROPERTIES_MANAGER;
    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
    }
  this->Internals->GlobalPropertiesManagers.erase(name);
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyManager::GetNumberOfGlobalPropertiesManagers()
{
  return static_cast<unsigned int>(
    this->Internals->GlobalPropertiesManagers.size());
}

//---------------------------------------------------------------------------
vtkSMGlobalPropertiesManager* vtkSMProxyManager::GetGlobalPropertiesManager(
  unsigned int index)
{
  unsigned int cur =0;
  vtkSMProxyManagerInternals::GlobalPropertiesManagersType::iterator iter;
  for (iter = this->Internals->GlobalPropertiesManagers.begin();
    iter != this->Internals->GlobalPropertiesManagers.end(); ++iter, ++cur)
    {
    if (cur == index)
      {
      return iter->second;
      }
    }

  return NULL;
}

//---------------------------------------------------------------------------
bool vtkSMProxyManager::LoadConfigurationXML(const char* xml)
{
  vtkSmartPointer<vtkSMXMLParser> parser =
    vtkSmartPointer<vtkSMXMLParser>::New();
  if (xml && parser->Parse(xml))
    {
    parser->ProcessConfiguration(this);
    return true;
    }
  return false;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent <<  "UpdateInputProxies: " <<  this->UpdateInputProxies << endl;
}
