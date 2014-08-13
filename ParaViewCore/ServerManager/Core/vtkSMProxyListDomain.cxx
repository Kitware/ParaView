/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyListDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyListDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"

#include <cassert>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
class vtkSMProxyListDomainInternals
{
public:
  typedef std::vector<vtkSmartPointer<vtkSMProxy> > VectorOfProxies;
  VectorOfProxies ProxyList;

  struct ProxyInfo
    {
    std::string GroupName;
    std::string ProxyName;
    };

  typedef std::vector<ProxyInfo> VectorOfProxyInfo;
  VectorOfProxyInfo ProxyTypeList;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMProxyListDomain);
//-----------------------------------------------------------------------------
vtkSMProxyListDomain::vtkSMProxyListDomain()
{
  this->Internals = new vtkSMProxyListDomainInternals;
}

//-----------------------------------------------------------------------------
vtkSMProxyListDomain::~vtkSMProxyListDomain()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::CreateProxies(vtkSMSessionProxyManager* pxm)
{
  assert(pxm);

  this->Internals->ProxyList.clear();
  for (vtkSMProxyListDomainInternals::VectorOfProxyInfo::iterator iter =
    this->Internals->ProxyTypeList.begin();
    iter != this->Internals->ProxyTypeList.end(); ++iter)
    {
    vtkSMProxy* proxy = pxm->NewProxy(
      iter->GroupName.c_str(), iter->ProxyName.c_str());
    if (proxy)
      {
      this->Internals->ProxyList.push_back(proxy);
      proxy->FastDelete();
      }
    }
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::IsInDomain(vtkSMProperty* vtkNotUsed(property))
{
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::AddProxy(const char* group, const char* name)
{
  vtkSMProxyListDomainInternals::ProxyInfo info;
  info.GroupName = group;
  info.ProxyName = name;
  this->Internals->ProxyTypeList.push_back(info);
}

//-----------------------------------------------------------------------------
unsigned int vtkSMProxyListDomain::GetNumberOfProxyTypes()
{
  return static_cast<unsigned int>(this->Internals->ProxyTypeList.size());
}

//-----------------------------------------------------------------------------
const char* vtkSMProxyListDomain::GetProxyGroup(unsigned int cc)
{
  if (this->GetNumberOfProxyTypes() <= cc)
    {
    vtkErrorMacro("Invalid index " << cc);
    return NULL;
    }

  return this->Internals->ProxyTypeList[cc].GroupName.c_str();
}

//-----------------------------------------------------------------------------
const char* vtkSMProxyListDomain::GetProxyName(unsigned int cc)
{
  if (this->GetNumberOfProxyTypes() <= cc)
    {
    vtkErrorMacro("Invalid index " << cc);
    return NULL;
    }

  return this->Internals->ProxyTypeList[cc].ProxyName.c_str();
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(prop);
  if (pp && this->GetNumberOfProxies() > 0)
    {
    vtkSMPropertyHelper helper(prop);
    helper.SetUseUnchecked(use_unchecked_values);
    vtkSMProxy *values[1] = {this->GetProxy(0)};
    helper.Set(values, 1);
    return 1;
    }

  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::ReadXMLAttributes(vtkSMProperty* prop, 
  vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
    {
    return 0;
    }

  int found = 0;
  unsigned int max = element->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc< max; ++cc)
    {
    vtkPVXMLElement* proxyElement = element->GetNestedElement(cc);
    if (strcmp(proxyElement->GetName(), "Proxy") == 0)
      {
      const char* name = proxyElement->GetAttribute("name");
      const char* group = proxyElement->GetAttribute("group");
      if (name && group)
        {
        this->AddProxy(group, name);
        found = 1;
        }
      }
    }

  if (!found)
    {
    vtkErrorMacro("Required element \"Proxy\" (with a 'name' and 'group' attribute) "
                  "was not found.");
    return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::AddProxy(vtkSMProxy* proxy)
{
  this->Internals->ProxyList.push_back(proxy);
}

//-----------------------------------------------------------------------------
unsigned int vtkSMProxyListDomain::GetNumberOfProxies()
{
  return static_cast<unsigned int>(this->Internals->ProxyList.size());
}

//-----------------------------------------------------------------------------
bool vtkSMProxyListDomain::HasProxy(vtkSMProxy* proxy)
{
  vtkSMProxyListDomainInternals::VectorOfProxies::iterator iter;
  for (iter = this->Internals->ProxyList.begin();
    iter != this->Internals->ProxyList.end(); iter++)
    {
    if (iter->GetPointer() == proxy)
      {
      return true;
      }
    }
  return false;
}


//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyListDomain::GetProxy(unsigned int index)
{
  if (index >= this->Internals->ProxyList.size())
    {
    vtkErrorMacro("Index " << index << " greater than max "
      << this->Internals->ProxyList.size());
    return 0;
    }
  return this->Internals->ProxyList[index].GetPointer();
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::RemoveProxy(vtkSMProxy* proxy)
{
  vtkSMProxyListDomainInternals::VectorOfProxies::iterator iter;
  for (iter = this->Internals->ProxyList.begin();
    iter != this->Internals->ProxyList.end(); iter++)
    {
    if (iter->GetPointer() == proxy)
      {
      this->Internals->ProxyList.erase(iter);
      return 1;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::RemoveProxy(unsigned int index)
{
  if (index >= this->Internals->ProxyList.size())
    {
    return 0;
    }

  vtkSMProxyListDomainInternals::VectorOfProxies::iterator iter;
  unsigned int cc=0; 
  for (iter = this->Internals->ProxyList.begin();
    iter != this->Internals->ProxyList.end(); ++iter, ++cc)
    {
    if (cc == index)
      {
      this->Internals->ProxyList.erase(iter);
      return 1;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::ChildSaveState(vtkPVXMLElement* element)
{
  this->Superclass::ChildSaveState(element);

  vtkSMProxyListDomainInternals::VectorOfProxies::iterator iter;
  for (iter = this->Internals->ProxyList.begin();
    iter != this->Internals->ProxyList.end(); ++iter)
    {
    vtkSMProxy* proxy = iter->GetPointer();
    vtkPVXMLElement* proxyElem = vtkPVXMLElement::New();
    proxyElem->SetName("Proxy");
    proxyElem->AddAttribute("value",
      static_cast<int>(proxy->GetGlobalID()));
    element->AddNestedElement(proxyElem);
    proxyElem->Delete();
    }
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::LoadState(vtkPVXMLElement* element,
  vtkSMProxyLocator* loader)
{
  this->Internals->ProxyList.clear();
  if (!this->Superclass::LoadState(element, loader))
    {
    return 0;
    }

  for (unsigned int cc=0; cc < element->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* proxyElem = element->GetNestedElement(cc);
    if (strcmp(proxyElem->GetName(), "Proxy") == 0)
      {
      int id;
      if (proxyElem->GetScalarAttribute("value", &id))
        {
        vtkSMProxy* proxy = loader->LocateProxy(id);
        if (proxy)
          {
          this->AddProxy(proxy);
          }
        }
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::SetProxies(vtkSMProxy** proxies, unsigned int count)
{
  vtkSMProxyListDomainInternals::VectorOfProxies newValues(proxies, proxies+count);
  if (this->Internals->ProxyList != newValues)
    {
    this->Internals->ProxyList = newValues;
    this->DomainModified();
    }
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
