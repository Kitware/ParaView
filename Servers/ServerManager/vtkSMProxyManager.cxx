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
#include "vtkSMInstantiator.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>

#include "vtkStdString.h"


vtkStandardNewMacro(vtkSMProxyManager);
vtkCxxRevisionMacro(vtkSMProxyManager, "1.8");

class vtkSMProxyManagerElementMapType:
  public vtkstd::map<vtkStdString, vtkSmartPointer<vtkPVXMLElement> > {};

class vtkSMProxyManagerProxyMapType:
  public vtkstd::map<vtkStdString, vtkSmartPointer<vtkSMProxy> > {};

struct vtkSMProxyManagerInternals
{
  typedef vtkstd::map<vtkStdString, vtkSMProxyManagerElementMapType> GroupMapType;
  GroupMapType GroupMap;

  typedef vtkstd::map<vtkStdString, vtkSMProxyManagerProxyMapType> ProxyGroupType;
  ProxyGroupType RegisteredProxyMap;
};

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
void vtkSMProxyManager::AddElement(const char* groupName, 
                                   const char* name,
                                   vtkPVXMLElement* element)
{
  vtkSMProxyManagerElementMapType& elementMap = 
    this->Internals->GroupMap[groupName];
  elementMap[name] = element;
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMProxyManager::NewProperty(vtkPVXMLElement* pelement)
{
  vtkObject* object = 0;
  ostrstream name;
  name << "vtkSM" << pelement->GetName() << ends;
  object = vtkInstantiator::CreateInstance(name.str());
  delete[] name.str();

  vtkSMProperty* property = vtkSMProperty::SafeDownCast(object);
  if (property)
    {
    property->ReadXMLAttributes(pelement);
    }
  return property;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::NewProxy(
  const char* groupName, const char* proxyName)
{
  if (!groupName || !proxyName)
    {
    return 0;
    }
  vtkSMProxyManagerInternals::GroupMapType::iterator it =
    this->Internals->GroupMap.find(groupName);
  if (it != this->Internals->GroupMap.end())
    {
    vtkSMProxyManagerElementMapType::iterator it2 =
      it->second.find(proxyName);

    if (it2 != it->second.end())
      {
      vtkPVXMLElement* element = it2->second.GetPointer();
      return this->NewProxy(element, groupName);
      }
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
    const char* className = pelement->GetAttribute("class");
    if(className)
      {
      proxy->SetVTKClassName(className);
      }

    const char* xmlname = pelement->GetAttribute("name");
    if(xmlname)
      {
      proxy->SetXMLName(xmlname);
      }
    proxy->SetXMLGroup(groupname);

    for(unsigned int i=0; i < pelement->GetNumberOfNestedElements(); ++i)
      {
      vtkPVXMLElement* propElement = pelement->GetNestedElement(i);
      if (strcmp(propElement->GetName(), "SubProxy")==0)
        {
        vtkPVXMLElement* subElement = propElement->GetNestedElement(0);
        if (subElement)
          {
          const char* name = subElement->GetAttribute("name");
          if (name)
            {
            vtkSMProxy* subproxy = this->NewProxy(subElement, 0);
            proxy->AddSubProxy(name, subproxy);
            subproxy->Delete();
            }
          }
        }
      const char* name = propElement->GetAttribute("name");
      if (name)
        {
        vtkSMProperty* prop = this->NewProperty(propElement);
        if (prop)
          {
          proxy->AddProperty(name, prop);
          prop->Delete();
          }
        }
      }
    }
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
      return it2->second;
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
      return it2->second;
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
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::SaveState(const char* filename)
{
  ofstream os(filename, ios::out);
  vtkIndent indent;

  vtkSMProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
    {
    os << indent << "Proxy group : " << it->first.c_str() << endl;
    vtkSMProxyManagerProxyMapType::iterator it2 =
      it->second.begin();
    for (; it2 != it->second.end(); it2++)
      {
      it2->second->SaveState(it2->first.c_str(), &os, indent.GetNextIndent());
      }
    }
  
}

//---------------------------------------------------------------------------
void vtkSMProxyManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
