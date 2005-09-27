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

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkInstantiator.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>
#include <vtkstd/set>

#include "vtkStdString.h"

#include "vtkSMProxyManagerInternals.h"

vtkStandardNewMacro(vtkSMProxyManager);
vtkCxxRevisionMacro(vtkSMProxyManager, "1.26");

//---------------------------------------------------------------------------
vtkSMProxyManager::vtkSMProxyManager()
{
  this->Internals = new vtkSMProxyManagerInternals;
}

//---------------------------------------------------------------------------
vtkSMProxyManager::~vtkSMProxyManager()
{
  this->UnRegisterProxies();
  delete this->Internals;
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
      return it2->second.GetPointer();
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
      return it2->second.GetPointer();
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
      if (proxy == it2->second.GetPointer())
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
      if (proxy == it2->second.GetPointer())
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
      it->second.erase(it2);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::RegisterProxy(const char* groupname, 
                                      const char* name, 
                                      vtkSMProxy* proxy)
{
  this->Internals->RegisteredProxyMap[groupname][name] = proxy;
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UpdateRegisteredProxies(const char* groupname)
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if ( it != this->Internals->RegisteredProxyMap.end() )
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      it2->second->UpdateVTKObjects();
      }
      
    for (it2 = it->second.begin(); it2 != it->second.end(); it2++)
      {
      it2->second->UpdateInformation();
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::UpdateRegisteredProxies()
{
  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      it2->second->UpdateVTKObjects();
      }

    for (it2 = it->second.begin(); it2 != it->second.end(); it2++)
      {
      it2->second->UpdateInformation();
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveState(const char* filename)
{
  ofstream os(filename, ios::out);
  vtkIndent indent;
  this->SaveState(0, &os, indent);
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveState(const char*, ostream* os, vtkIndent indent)
{

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
        if (visited_proxies.find(it2->second) == visited_proxies.end())
          {
          it2->second->SaveState(it2->first.c_str(), os, indent);
          visited_proxies.insert(it2->second);
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
      *os << indent 
         << "<ProxyCollection name=\"" << it->first.c_str() << "\">" << endl;
      vtkSMProxyManagerProxyMapType::iterator it2 =
        it->second.begin();
      for (; it2 != it->second.end(); it2++)
        {
        *os << indent.GetNextIndent()
           << "<Item "
           << "id=\""<< it2->second->GetName() << "\" "
           << "name=\"" << it2->first.c_str() << "\" "
           << "/>" << endl;
        }
      *os << indent << "</ProxyCollection>" << endl;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
