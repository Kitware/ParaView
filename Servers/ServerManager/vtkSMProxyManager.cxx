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
vtkCxxRevisionMacro(vtkSMProxyManager, "1.3");

struct vtkSMProxyManagerInternals
{
  typedef vtkstd::map<vtkStdString, 
                      vtkSmartPointer<vtkPVXMLElement> > ProxyMapType;
  typedef vtkstd::map<vtkStdString, ProxyMapType> GroupMapType;
  GroupMapType GroupMap;
};

//---------------------------------------------------------------------------
vtkSMProxyManager::vtkSMProxyManager()
{
  this->Internals = new vtkSMProxyManagerInternals;
}

//---------------------------------------------------------------------------
vtkSMProxyManager::~vtkSMProxyManager()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::AddElement(const char* groupName, 
                                   const char* name,
                                   vtkPVXMLElement* element)
{
  vtkSMProxyManagerInternals::ProxyMapType& proxyMap = 
    this->Internals->GroupMap[groupName];
  proxyMap[name] = element;
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
    vtkSMProxyManagerInternals::ProxyMapType::iterator it2 =
      it->second.find(proxyName);

    if (it2 != it->second.end())
      {
      vtkPVXMLElement* element = it2->second.GetPointer();
      return this->NewProxy(element);
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyManager::NewProxy(vtkPVXMLElement* pelement)
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

    for(unsigned int i=0; i < pelement->GetNumberOfNestedElements(); ++i)
      {
      vtkPVXMLElement* propElement = pelement->GetNestedElement(i);
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
void vtkSMProxyManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
