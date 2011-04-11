/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGlobalPropertiesManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMGlobalPropertiesManager.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkWeakPointer.h"

#include <vtkstd/map>
#include <vtkstd/list>
#include <vtkstd/string>

class vtkSMGlobalPropertiesManager::vtkInternals
{
public:
  class vtkValue
    {
  public:
    vtkWeakPointer<vtkSMProxy> Proxy;
    vtkstd::string PropertyName;
    };

  typedef vtkstd::list<vtkValue> VectorOfValues;
  typedef vtkstd::map<vtkstd::string, VectorOfValues> LinksType;
  LinksType Links;
};


vtkStandardNewMacro(vtkSMGlobalPropertiesManager);
//----------------------------------------------------------------------------
vtkSMGlobalPropertiesManager::vtkSMGlobalPropertiesManager()
{
  this->Internals = new vtkSMGlobalPropertiesManager::vtkInternals();
  this->SetLocation(0);
}

//----------------------------------------------------------------------------
vtkSMGlobalPropertiesManager::~vtkSMGlobalPropertiesManager()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
bool vtkSMGlobalPropertiesManager::InitializeProperties(
  const char* xmlgroup, const char* xmlname)
{
  if (this->XMLName && strcmp(this->XMLName, xmlname) != 0 &&
    this->XMLGroup && strcmp(this->XMLGroup, xmlgroup) != 0)
    {
    vtkErrorMacro("Manager already initialized");
    return false;
    }

  if (this->XMLName && this->XMLGroup)
    {
    return false;
    }
  vtkPVXMLElement* elem = this->GetProxyManager()->GetProxyElement(xmlgroup, xmlname);
  if (!elem)
    {
    return false;
    }

  this->ReadXMLAttributes(this->GetProxyManager(), elem);
  this->SetXMLName(xmlname);
  this->SetXMLGroup(xmlgroup);
  return true;
}

//----------------------------------------------------------------------------
const char* vtkSMGlobalPropertiesManager::GetGlobalPropertyName(
  vtkSMProxy* proxy, const char* propname)
{
  vtkInternals::LinksType::iterator mapIter;
  for (mapIter = this->Internals->Links.begin();
    mapIter!= this->Internals->Links.end(); ++mapIter)
    {
    vtkInternals::VectorOfValues::iterator listIter;
    for (listIter = mapIter->second.begin();
      listIter != mapIter->second.end();
      ++listIter)
      {
      if (listIter->Proxy == proxy &&
        listIter->PropertyName == propname)
        {
        return mapIter->first.c_str();
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesManager::RemoveGlobalPropertyLink(
  const char* globalPropertyName, vtkSMProxy* proxy, const char* propname)
{
  vtkInternals::VectorOfValues& values =
    this->Internals->Links[globalPropertyName];
  vtkInternals::VectorOfValues::iterator listIter;
  for (listIter = values.begin(); listIter != values.end(); ++listIter)
    {
    if (listIter->Proxy == proxy &&
      listIter->PropertyName == propname)
      {
      values.erase(listIter);
      break;
      }
    }

  ModifiedInfo info;
  info.AddLink = false;
  info.GlobalPropertyName = globalPropertyName;
  info.PropertyName = propname;
  info.Proxy = proxy;
  this->InvokeEvent(vtkSMGlobalPropertiesManager::GlobalPropertyLinkModified, &info);
}

//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesManager::SetGlobalPropertyLink(
  const char* globalPropertyName, vtkSMProxy* proxy, const char* propname)
{
  if (!globalPropertyName || !proxy || !propname ||
    !proxy->GetProperty(propname))
    {
    return;
    }

  const char* oldGlobalName = this->GetGlobalPropertyName(proxy, propname);
  if (oldGlobalName)
    {
    if (strcmp(oldGlobalName, globalPropertyName) == 0)
      {
      return;//nothing to do.
      }
    this->RemoveGlobalPropertyLink(oldGlobalName, proxy, propname);
    }
  vtkInternals::vtkValue value;
  value.Proxy = proxy;
  value.PropertyName = propname;
  this->Internals->Links[globalPropertyName].push_back(value);
  proxy->GetProperty(propname)->Copy(
    this->GetProperty(globalPropertyName));
  if (proxy->GetObjectsCreated())
    {
    // This handles the case when the proxy hasn't been created yet (which
    // happens when reviving servermanager on the server side.
    proxy->UpdateVTKObjects();
    }

  ModifiedInfo info;
  info.AddLink = true;
  info.GlobalPropertyName = globalPropertyName;
  info.PropertyName = propname;
  info.Proxy = proxy;
  this->InvokeEvent(vtkSMGlobalPropertiesManager::GlobalPropertyLinkModified, &info);
}

//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesManager::SetPropertyModifiedFlag(const char* name,
  int flag)
{
  vtkSMProperty* globalProperty = this->GetProperty(name);
  vtkInternals::VectorOfValues& values = this->Internals->Links[name];
  vtkInternals::VectorOfValues::iterator iter;
  for (iter = values.begin(); iter != values.end(); ++iter)
    {
    if (iter->Proxy.GetPointer() && iter->Proxy->GetProperty(
        iter->PropertyName.c_str()))
      {
      iter->Proxy->GetProperty(iter->PropertyName.c_str())->Copy(
        globalProperty);
      iter->Proxy->UpdateVTKObjects();
      }
    }

  // there's no need to call this really, but no harm either.
  this->Superclass::SetPropertyModifiedFlag(name, flag);
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMGlobalPropertiesManager::SaveLinkState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* elem = vtkPVXMLElement::New();
  elem->SetName("GlobalPropertiesManager");
  elem->AddAttribute("group", this->XMLGroup);
  elem->AddAttribute("type", this->XMLName);

  vtkInternals::LinksType::iterator mapIter;
  for (mapIter = this->Internals->Links.begin();
    mapIter!= this->Internals->Links.end(); ++mapIter)
    {
    vtkInternals::VectorOfValues::iterator listIter;
    for (listIter = mapIter->second.begin();
      listIter != mapIter->second.end();
      ++listIter)
      {
      if (listIter->Proxy)
        {
        vtkPVXMLElement* linkElem = vtkPVXMLElement::New();
        linkElem->SetName("Link");
        linkElem->AddAttribute("global_name", mapIter->first.c_str());
        linkElem->AddAttribute( "proxy",
                                static_cast<unsigned int>(listIter->Proxy->GetGlobalID()));
        linkElem->AddAttribute("property", listIter->PropertyName.c_str());
        elem->AddNestedElement(linkElem);
        linkElem->Delete();
        }
      }
    }

  if (root)
    {
    root->AddNestedElement(elem);
    elem->Delete();
    }
  return elem;
}

//----------------------------------------------------------------------------
int vtkSMGlobalPropertiesManager::LoadLinkState(
  vtkPVXMLElement* elem, vtkSMProxyLocator* locator)
{
  unsigned int numElems = elem->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* child = elem->GetNestedElement(cc);
    if (!child->GetName() || strcmp(child->GetName(), "Link") != 0)
      {
      vtkWarningMacro("Invalid element in global link state. Ignoring.");
      continue;
      }
    vtkstd::string global_name = child->GetAttributeOrEmpty("global_name");
    vtkstd::string property = child->GetAttributeOrEmpty("property");
    int proxyid = 0;
    child->GetScalarAttribute("proxy", &proxyid);
    vtkSMProxy* proxy = locator->LocateProxy(proxyid);
    if (!global_name.empty() && !property.empty() && proxy)
      {
      this->SetGlobalPropertyLink(global_name.c_str(), proxy, property.c_str());
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
