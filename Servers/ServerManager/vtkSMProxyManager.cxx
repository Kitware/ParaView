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

#include "vtkCommand.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMCompoundProxyDefinitionLoader.h"
#include "vtkSMDocumentation.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStateLoader.h"
#include "vtkSMUndoStack.h"

#include <vtkstd/map>
#include <vtkstd/set>

#include "vtkStdString.h"

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
vtkCxxRevisionMacro(vtkSMProxyManager, "1.42");
vtkCxxSetObjectMacro(vtkSMProxyManager, UndoStack, vtkSMUndoStack);
//---------------------------------------------------------------------------
vtkSMProxyManager::vtkSMProxyManager()
{
  this->Internals = new vtkSMProxyManagerInternals;
  this->Observer = vtkSMProxyManagerObserver::New();
  this->Observer->SetTarget(this);
  this->UndoStack = 0;
#if 0 // for debugging
  vtkSMProxyRegObserver* obs = new vtkSMProxyRegObserver;
  this->AddObserver(vtkCommand::RegisterEvent, obs);
  this->AddObserver(vtkCommand::UnRegisterEvent, obs);
#endif
}

//---------------------------------------------------------------------------
vtkSMProxyManager::~vtkSMProxyManager()
{
  this->UnRegisterProxies();
  delete this->Internals;

  this->Observer->SetTarget(0);
  this->Observer->Delete();
  this->SetUndoStack(0);
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::InstantiateGroupPrototypes(const char* groupName)
{
  if (!groupName)
    {
    return;
    }

  ostrstream newgroupname;
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
      if (!this->GetProxy(newgroupname.str(), it2->first.c_str()))
        {
        vtkSMProxy* proxy = this->NewProxy(element, groupName);
        proxy->SetConnectionID(
          vtkProcessModuleConnectionManager::GetSelfConnectionID());
        this->RegisterProxy(newgroupname.str(), it2->first.c_str(), proxy);
        proxy->Delete();
        }
      }

    }
  delete[] newgroupname.str();
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::AddElement(const char* groupName, 
                                   const char* name,
                                   vtkPVXMLElement* element)
{
  vtkSMProxyManagerElementMapType& elementMap = 
    this->Internals->GroupMap[groupName];
  elementMap[name] = element;
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
    return this->NewProxy(element, groupName);
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::NewProxy(vtkPVXMLElement* pelement,
                                        const char* groupname)
{
  vtkObject* object = 0;
  ostrstream cname;
  cname << "vtkSM" << pelement->GetName() << ends;
  object = vtkInstantiator::CreateInstance(cname.str());
  delete[] cname.str();

  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(object);
  if (proxy)
    {
    proxy->ReadXMLAttributes(this, pelement);
    proxy->SetXMLGroup(groupname);
    }
  return proxy;
}


//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMProxyManager::NewProxyDocumentation(
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
    vtkSMDocumentation* doc = vtkSMDocumentation::New();
    for (unsigned int cc=0; cc < element->GetNumberOfNestedElements(); cc++)
      {
      vtkPVXMLElement* doc_elem = element->GetNestedElement(cc);
      if (strcmp(doc_elem->GetName(), "Documentation") == 0)
        {
        doc->SetDocumentationElement(doc_elem);
        break;
        }
      }
    return doc;
    }
  vtkErrorMacro("Failed to locate documentation for proxy: " 
    << groupName << ", " <<proxyName);
  return NULL;
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
      return element;
      }
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
unsigned int vtkSMProxyManager::GetNumberOfProxies(const char* group)
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    return it->second.size();
    }
  return 0;
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
      return it2->second.Proxy.GetPointer();
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
      return it2->second.Proxy.GetPointer();
      }
    }
  return 0;
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
      if (proxy == it2->second.Proxy.GetPointer())
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
      if (proxy == it2->second.Proxy.GetPointer())
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
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterProxy(const char* group, const char* name)
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.find(name);
    if (it2 != it->second.end())
      {
      RegisteredProxyInformation info;
      info.Proxy = it2->second.Proxy;
      info.GroupName = it->first.c_str();
      info.ProxyName = it2->first.c_str();
      
      this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);

      it->second.erase(it2);
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
      RegisteredProxyInformation info;
      info.Proxy = it2->second.Proxy;
      info.GroupName = it->first.c_str();
      info.ProxyName = it2->first.c_str();
      
      this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);

      it->second.erase(it2);
      }
    }

}

//---------------------------------------------------------------------------
void vtkSMProxyManager::RegisterProxy(const char* groupname, 
                                      const char* name, 
                                      vtkSMProxy* proxy)
{
  ProxyInfo &obj =
    this->Internals->RegisteredProxyMap[groupname][name];

  obj.Proxy = proxy;
  
  // Add observers to note proxy modification.
  obj.ModifiedObserverTag = proxy->AddObserver(vtkCommand::PropertyModifiedEvent,
    this->Observer);
  obj.UpdateObserverTag = proxy->AddObserver(vtkCommand::UpdateEvent,
    this->Observer);
  // Note, these observer will be removed in the destructor of obj.

  RegisteredProxyInformation info;
  info.Proxy = proxy;
  info.GroupName = groupname;
  info.ProxyName = name;

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
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      // Check is proxy is in the modified set.
      if (!modified_only || 
        this->Internals->ModifiedProxies.find(it2->second.Proxy.GetPointer())
        != this->Internals->ModifiedProxies.end())
        {
        it2->second.Proxy.GetPointer()->UpdateVTKObjects();
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UpdateRegisteredProxies(int modified_only /*=1*/)
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      // Check is proxy is in the modified set.
      if (!modified_only ||
        this->Internals->ModifiedProxies.find(it2->second.Proxy.GetPointer())
        != this->Internals->ModifiedProxies.end())
        {
        it2->second.Proxy.GetPointer()->UpdateVTKObjects();
        }
      }
    }
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
    this->Internals->RegisteredLinkMap.erase(it);
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
  spLoader->SetConnectionID(id);
  spLoader->LoadState(rootElement);
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
  vtkPVXMLElement* rootElement = vtkPVXMLElement::New();
  rootElement->SetName("ServerManagerState");

  this->SaveState(rootElement);

  ofstream os(filename, ios::out);
  rootElement->PrintXML(os, vtkIndent());
  rootElement->Delete();
}


//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveState(vtkPVXMLElement* rootElement)
{
  if (!rootElement)
    {
    rootElement = vtkPVXMLElement::New();
    rootElement->SetName("ServerManagerState");
    }
  else
    {
    rootElement->Register(this);
    }

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
    if (do_group)
      {
      // save the states of all proxies in this group.
      for (; it2 != it->second.end(); it2++)
        {
        if (visited_proxies.find(it2->second.Proxy.GetPointer()) 
          == visited_proxies.end())
          {
          it2->second.Proxy.GetPointer()->SaveState(rootElement);
          visited_proxies.insert(it2->second.Proxy.GetPointer());
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
      for (; it2 != it->second.end(); it2++)
        {
        vtkPVXMLElement* itemElement = vtkPVXMLElement::New();
        itemElement->SetName("Item");
        itemElement->AddAttribute("id", 
                     it2->second.Proxy.GetPointer()->GetSelfIDAsString());
        itemElement->AddAttribute("name", it2->first.c_str());
        collectionElement->AddNestedElement(itemElement);
        itemElement->Delete();
        }
      rootElement->AddNestedElement(collectionElement);
      collectionElement->Delete();
      }
    }

  vtkPVXMLElement* defs = vtkPVXMLElement::New();
  defs->SetName("CompoundProxyDefinitions");
  this->SaveCompoundProxyDefinitions(defs);
  rootElement->AddNestedElement(defs);
  defs->Delete();

  vtkPVXMLElement* links = vtkPVXMLElement::New();
  links->SetName("Links");
  this->SaveRegisteredLinks(links);
  rootElement->AddNestedElement(links);
  links->Delete();

  rootElement->Delete();
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterCompoundProxyDefinitions()
{
  this->Internals->CompoundProxyDefinitions.erase(
    this->Internals->CompoundProxyDefinitions.begin(),
    this->Internals->CompoundProxyDefinitions.end());
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterCompoundProxyDefinition(const char* name)
{
  vtkSMProxyManagerInternals::DefinitionType::iterator it =
    this->Internals->CompoundProxyDefinitions.find(name);
  if ( it != this->Internals->CompoundProxyDefinitions.end() )
    {
    this->Internals->CompoundProxyDefinitions.erase(it);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::RegisterCompoundProxyDefinition(
  const char* name, vtkPVXMLElement* top)
{
  if (!top)
    {
    return;
    }

  this->Internals->CompoundProxyDefinitions[name] = top;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyManager::GetCompoundProxyDefinition(
  const char* name)
{
  if (!name)
    {
    return 0;
    }

  vtkSMProxyManagerInternals::DefinitionType::iterator iter =
    this->Internals->CompoundProxyDefinitions.find(name);
  if (iter != this->Internals->CompoundProxyDefinitions.end())
    {
    return iter->second.GetPointer();
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::LoadCompoundProxyDefinitions(vtkPVXMLElement* root)
{
  if (!root)
    {
    return;
    }
  
  unsigned int numElems = root->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "CompoundProxyDefinition") == 0)
      {
      const char* name = currentElement->GetAttribute("name");
      if (name)
        {
        if (currentElement->GetNumberOfNestedElements() == 1)
          {
          vtkPVXMLElement* defElement = currentElement->GetNestedElement(0);
          if (strcmp(defElement->GetName(), "CompoundProxy") == 0)
            {
            this->RegisterCompoundProxyDefinition(name, defElement);
            }
          }
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::LoadCompoundProxyDefinitions(const char* filename)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName(filename);
  parser->Parse();

  this->LoadCompoundProxyDefinitions(parser->GetRootElement());
  parser->Delete();
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveCompoundProxyDefinitions(
  vtkPVXMLElement* rootElement)
{
  if (!rootElement)
    {
    return;
    }
  vtkSMProxyManagerInternals::DefinitionType::iterator iter =
    this->Internals->CompoundProxyDefinitions.begin();
  for(; iter != this->Internals->CompoundProxyDefinitions.end(); iter++)
    {
    vtkPVXMLElement* elem = iter->second;
    if (elem)
      {
      vtkPVXMLElement* defElement = vtkPVXMLElement::New();
      defElement->SetName("CompoundProxyDefinition");
      defElement->AddAttribute("name", iter->first.c_str());
      defElement->AddNestedElement(elem, 0);
      rootElement->AddNestedElement(defElement);
      defElement->Delete();
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveCompoundProxyDefinitions(const char* filename)
{
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("CompoundProxyDefinitions");
  this->SaveCompoundProxyDefinitions(root);

  ofstream os(filename, ios::out);
  root->PrintXML(os, vtkIndent());
  root->Delete();
}

//---------------------------------------------------------------------------
vtkSMCompoundProxy* vtkSMProxyManager::NewCompoundProxy(const char* name)
{
  vtkPVXMLElement* definition = this->GetCompoundProxyDefinition(name);
  if (!definition)
    {
    return 0;
    }
  // TODO: ConnectionID....it needs to be set as constituent proxies are created.
  vtkSMCompoundProxyDefinitionLoader* loader = 
    vtkSMCompoundProxyDefinitionLoader::New();
  vtkSMCompoundProxy* cproxy = loader->LoadDefinition(definition);
  loader->Delete();
  if (cproxy)
    {
    // Since this Compound proxy was created using a definition, we set
    // the XMLName on the CP to denote the name of the definition from
    // which it was created.
    cproxy->SetXMLName(name);
    }
  return cproxy;
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
void vtkSMProxyManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UndoStack: " << this->UndoStack << endl;
}
