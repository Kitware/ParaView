/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGlobalPropertiesProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMGlobalPropertiesProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyLocator.h"
#include "vtkWeakPointer.h"

#include <list>
#include <map>

class vtkSMGlobalPropertiesProxy::vtkInternals
{
public:
  class vtkValue
  {
  public:
    vtkWeakPointer<vtkSMProxy> Proxy;
    std::string PropertyName;
    unsigned long ObserverId;
    vtkValue()
      : ObserverId(0)
    {
    }
    void RemoveObserver()
    {
      vtkSMProperty* prop =
        this->Proxy ? this->Proxy->GetProperty(this->PropertyName.c_str()) : NULL;
      if (prop && this->ObserverId > 0)
      {
        prop->RemoveObserver(this->ObserverId);
      }
      this->ObserverId = 0;
    }
  };

  typedef std::list<vtkValue> VectorOfValues;
  typedef std::map<std::string, VectorOfValues> LinksType;
  LinksType Links;
  bool Updating;

  vtkInternals()
    : Updating(false)
  {
  }
};

vtkStandardNewMacro(vtkSMGlobalPropertiesProxy);
//----------------------------------------------------------------------------
vtkSMGlobalPropertiesProxy::vtkSMGlobalPropertiesProxy()
{
  this->Internals = new vtkSMGlobalPropertiesProxy::vtkInternals();
  this->SetLocation(0);
  this->PrototypeOn();
}

//----------------------------------------------------------------------------
vtkSMGlobalPropertiesProxy::~vtkSMGlobalPropertiesProxy()
{
  this->RemoveAllLinks();
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesProxy::RemoveAllLinks()
{
  vtkInternals::LinksType::iterator mapIter;
  for (mapIter = this->Internals->Links.begin(); mapIter != this->Internals->Links.end(); ++mapIter)
  {
    vtkInternals::VectorOfValues::iterator listIter;
    for (listIter = mapIter->second.begin(); listIter != mapIter->second.end(); ++listIter)
    {
      listIter->RemoveObserver();
    }
  }
  this->Internals->Links.clear();
}

//----------------------------------------------------------------------------
bool vtkSMGlobalPropertiesProxy::Link(
  const char* globalPropertyName, vtkSMProxy* proxy, const char* propname)
{
  if (!globalPropertyName || !proxy || !propname)
  {
    vtkErrorMacro("Incorrect arguments.");
    return false;
  }

  if (!proxy->GetProperty(propname))
  {
    vtkErrorMacro("Incorrect target property name: " << propname);
    return false;
  }

  if (!this->GetProperty(globalPropertyName))
  {
    vtkErrorMacro("Incorrect source property name:" << globalPropertyName);
    return false;
  }

  // avoid double linking.
  if (const char* oldname = this->GetLinkedPropertyName(proxy, propname))
  {
    if (strcmp(oldname, globalPropertyName) == 0)
    {
      return true;
    }
    this->Unlink(oldname, proxy, propname);
  }

  // Copy current value.
  vtkSMProperty* targetProp = proxy->GetProperty(propname);
  vtkSMProperty* globalProperty = this->GetProperty(globalPropertyName);
  targetProp->Copy(globalProperty);
  proxy->UpdateVTKObjects();

  vtkInternals::vtkValue value;
  value.Proxy = proxy;
  value.PropertyName = propname;
  value.ObserverId = targetProp->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkSMGlobalPropertiesProxy::TargetPropertyModified);
  this->Internals->Links[globalPropertyName].push_back(value);

  // FIXME: is this relevant?
  // if (proxy->GetObjectsCreated())
  //  {
  //  // This handles the case when the proxy hasn't been created yet (which
  //  // happens when reviving servermanager on the server side.
  //  proxy->UpdateVTKObjects();
  //  }

  // If we ever want to bring undo-redo support back.
  // ModifiedInfo info;
  // info.AddLink = true;
  // info.GlobalPropertyName = globalPropertyName;
  // info.PropertyName = propname;
  // info.Proxy = proxy;
  // this->InvokeEvent(vtkSMGlobalPropertiesProxy::GlobalPropertyLinkModified, &info);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMGlobalPropertiesProxy::Unlink(
  const char* globalPropertyName, vtkSMProxy* proxy, const char* propname)
{
  vtkInternals::VectorOfValues& values = this->Internals->Links[globalPropertyName];
  vtkInternals::VectorOfValues::iterator listIter;
  for (listIter = values.begin(); listIter != values.end(); ++listIter)
  {
    if (listIter->Proxy == proxy && listIter->PropertyName == propname)
    {
      listIter->RemoveObserver();
      values.erase(listIter);
      return true;
    }
  }
  return false;

  // If we ever want to bring undo-redo support back.
  // ModifiedInfo info;
  // info.AddLink = false;
  // info.GlobalPropertyName = globalPropertyName;
  // info.PropertyName = propname;
  // info.Proxy = proxy;
  // this->InvokeEvent(vtkSMGlobalPropertiesProxy::GlobalPropertyLinkModified, &info);
}

//----------------------------------------------------------------------------
const char* vtkSMGlobalPropertiesProxy::GetLinkedPropertyName(
  vtkSMProxy* proxy, const char* propname)
{
  vtkInternals::LinksType::iterator mapIter;
  for (mapIter = this->Internals->Links.begin(); mapIter != this->Internals->Links.end(); ++mapIter)
  {
    vtkInternals::VectorOfValues::iterator listIter;
    for (listIter = mapIter->second.begin(); listIter != mapIter->second.end(); ++listIter)
    {
      if (listIter->Proxy == proxy && listIter->PropertyName == propname)
      {
        return mapIter->first.c_str();
      }
    }
  }

  return NULL;
}
//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesProxy::SetPropertyModifiedFlag(const char* name, int flag)
{
  this->Superclass::SetPropertyModifiedFlag(name, flag);

  // Copy property value to all  linked properties.
  bool prev = this->Internals->Updating;
  this->Internals->Updating = true;
  vtkSMProperty* globalProperty = this->GetProperty(name);
  vtkInternals::VectorOfValues& values = this->Internals->Links[name];
  vtkInternals::VectorOfValues::iterator iter;
  for (iter = values.begin(); iter != values.end(); ++iter)
  {
    if (iter->Proxy.GetPointer() && iter->Proxy->GetProperty(iter->PropertyName.c_str()))
    {
      iter->Proxy->GetProperty(iter->PropertyName.c_str())->Copy(globalProperty);
      iter->Proxy->UpdateVTKObjects();
    }
  }
  this->Internals->Updating = prev;
}

//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesProxy::TargetPropertyModified(vtkObject* caller, unsigned long, void*)
{
  if (this->Internals->Updating)
  {
    return;
  }

  // target property was modified on its own volition. Unlink it.
  vtkSMProperty* target = vtkSMProperty::SafeDownCast(caller);

  for (vtkInternals::LinksType::iterator mapIter = this->Internals->Links.begin();
       mapIter != this->Internals->Links.end(); ++mapIter)
  {
    vtkInternals::VectorOfValues& values = mapIter->second;
    vtkInternals::VectorOfValues::iterator listIter;
    for (listIter = values.begin(); listIter != values.end(); ++listIter)
    {
      vtkInternals::vtkValue value = (*listIter);
      if (value.Proxy && (value.Proxy->GetProperty(value.PropertyName.c_str()) == target))
      {
        value.RemoveObserver();
        values.erase(listIter);
        return;
      }
    }
  }
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMGlobalPropertiesProxy::SaveXMLState(
  vtkPVXMLElement* root, vtkSMPropertyIterator* iter)
{
  (void)iter;

  if (!root)
  {
    return NULL;
  }

  vtkInternals::LinksType::iterator mapIter;
  for (mapIter = this->Internals->Links.begin(); mapIter != this->Internals->Links.end(); ++mapIter)
  {
    vtkInternals::VectorOfValues::iterator listIter;
    for (listIter = mapIter->second.begin(); listIter != mapIter->second.end(); ++listIter)
    {
      if (listIter->Proxy)
      {
        vtkPVXMLElement* linkElem = vtkPVXMLElement::New();
        linkElem->SetName("GlobalPropertyLink");
        linkElem->AddAttribute("global_name", mapIter->first.c_str());
        linkElem->AddAttribute("proxy", listIter->Proxy->GetGlobalIDAsString());
        linkElem->AddAttribute("property", listIter->PropertyName.c_str());
        root->AddNestedElement(linkElem);
        linkElem->Delete();
      }
    }
  }

  return root;
}

//----------------------------------------------------------------------------
int vtkSMGlobalPropertiesProxy::LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator)
{
  if (!locator)
  {
    return 1;
  }

  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    vtkPVXMLElement* child = element->GetNestedElement(cc);
    if (!child->GetName() || strcmp(child->GetName(), "GlobalPropertyLink") != 0)
    {
      continue;
    }
    std::string global_name = child->GetAttributeOrEmpty("global_name");
    std::string property = child->GetAttributeOrEmpty("property");
    int proxyid = 0;
    child->GetScalarAttribute("proxy", &proxyid);
    vtkSMProxy* proxy = locator->LocateProxy(proxyid);
    if (!global_name.empty() && !property.empty() && proxy)
    {
      this->Link(global_name.c_str(), proxy, property.c_str());
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
