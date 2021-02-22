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

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyInternals.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <string>
#include <vector>

namespace vtkSMProxyListDomainNS
{
//---------------------------------------------------------------------------
class vtkSMLinkObserver : public vtkCommand
{
public:
  vtkWeakPointer<vtkSMProperty> Output;
  typedef vtkCommand Superclass;
  const char* GetClassNameInternal() const override { return "vtkSMLinkObserver"; }
  static vtkSMLinkObserver* New() { return new vtkSMLinkObserver(); }
  void Execute(vtkObject* caller, unsigned long event, void* calldata) override
  {
    (void)event;
    (void)calldata;
    vtkSMProperty* input = vtkSMProperty::SafeDownCast(caller);
    if (input && this->Output)
    {
      // this will copy both checked and unchecked property values.
      this->Output->Copy(input);
    }
    vtkSMProxy* parent = vtkSMProxy::SafeDownCast(caller);
    if (parent && this->Output)
    {
      // this will update the output object.
      this->Output->GetParent()->UpdateVTKObjects();
    }
  }
};
}

//-----------------------------------------------------------------------------
class vtkSMProxyInfo
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;
  std::vector<unsigned long> ObserverIds;

  // Proxies in proxy-list domains can have hints that are used to setup
  // property-links to ensure that those proxies get appropriate domains.
  void ProcessProxyListProxyHints(vtkSMProxy* parent)
  {
    vtkSMProxy* plproxy = this->Proxy;
    vtkPVXMLElement* proxyListElement =
      plproxy->GetHints() ? plproxy->GetHints()->FindNestedElementByName("ProxyList") : nullptr;
    if (!proxyListElement)
    {
      return;
    }
    for (unsigned int cc = 0, max = proxyListElement->GetNumberOfNestedElements(); cc < max; ++cc)
    {
      vtkPVXMLElement* child = proxyListElement->GetNestedElement(cc);
      if (child && child->GetName() && strcmp(child->GetName(), "Link") == 0)
      {
        const char* name = child->GetAttribute("name");
        const char* linked_with = child->GetAttribute("with_property");
        if (name && linked_with)
        {
          vtkSMProperty* input = parent->GetProperty(linked_with);
          vtkSMProperty* output = plproxy->GetProperty(name);
          if (input && output)
          {
            vtkSMProxyListDomainNS::vtkSMLinkObserver* observer =
              vtkSMProxyListDomainNS::vtkSMLinkObserver::New();
            observer->Output = output;
            this->ObserverIds.push_back(
              input->AddObserver(vtkCommand::PropertyModifiedEvent, observer));
            this->ObserverIds.push_back(
              input->AddObserver(vtkCommand::UncheckedPropertyModifiedEvent, observer));
            this->ObserverIds.push_back(
              parent->AddObserver(vtkCommand::UpdatePropertyEvent, observer));
            observer->FastDelete();
            output->Copy(input);
          }
        }
      }
      else if (child && child->GetName() && strcmp(child->GetName(), "ShareProperties") == 0)
      {
        const char* name = child->GetAttribute("parent_subproxy");
        if (!name || !name[0])
        {
          continue;
        }
        vtkSMProxy* src_subproxy = parent->GetSubProxy(name);
        if (!src_subproxy)
        {
          vtkErrorWithObjectMacro(parent, "Subproxy "
              << name << " must be defined before "
                         "its properties can be shared with another subproxy.");
          continue;
        }
        vtkNew<vtkSMProxyLink> sharingLink;
        sharingLink->PropagateUpdateVTKObjectsOff();

        // Read the exceptions.
        for (unsigned int j = 0; j < child->GetNumberOfNestedElements(); j++)
        {
          vtkPVXMLElement* exceptionProp = child->GetNestedElement(j);
          if (strcmp(exceptionProp->GetName(), "Exception") != 0)
          {
            continue;
          }
          const char* exp_name = exceptionProp->GetAttribute("name");
          if (!exp_name)
          {
            vtkErrorWithObjectMacro(parent, "Exception tag must have the attribute 'name'.");
            continue;
          }
          sharingLink->AddException(exp_name);
        }
        sharingLink->AddLinkedProxy(src_subproxy, vtkSMLink::INPUT);
        sharingLink->AddLinkedProxy(plproxy, vtkSMLink::OUTPUT);
        parent->Internals->SubProxyLinks.push_back(sharingLink.Get());
      }
    }
  }
  void RemoveObservers()
  {
    for (size_t cc = 0; cc < this->ObserverIds.size(); cc++)
    {
      this->Proxy->RemoveObserver(this->ObserverIds[cc]);
    }
    this->ObserverIds.clear();
  }
};

class vtkSMProxyListDomainInternals
{
public:
  typedef std::vector<vtkSMProxyInfo> VectorOfProxies;
  const VectorOfProxies& GetProxies() const { return this->ProxyList; }

  // Add a proxy to the internal Proxies list. This will also process any "ProxyList"
  // hints specified on the \c proxy and setup property links with the parent-proxy of
  // self.
  void AddProxy(vtkSMProxy* proxy, vtkSMProxyListDomain* self)
  {
    vtkSMProxyInfo info;
    info.Proxy = proxy;

    vtkSMProxy* parent = self->GetProperty() ? self->GetProperty()->GetParent() : nullptr;
    if (parent)
    {
      // Handle proxy-list hints.
      info.ProcessProxyListProxyHints(parent);
    }
    this->ProxyList.push_back(info);
  }

  // Removes a proxy from the Proxies list. Will also cleanup any "links" setup
  // during AddProxy().
  bool RemoveProxy(vtkSMProxy* proxy)
  {
    VectorOfProxies::iterator iter;
    for (iter = this->ProxyList.begin(); iter != this->ProxyList.end(); iter++)
    {
      if (iter->Proxy == proxy)
      {
        iter->RemoveObservers();
        this->ProxyList.erase(iter);
        return true;
      }
    }
    return false;
  }

  // Removes a proxy from the Proxies list. Will also cleanup any "links" setup
  // during AddProxy().
  bool RemoveProxy(unsigned int index)
  {
    if (index >= this->ProxyList.size())
    {
      return false;
    }

    VectorOfProxies::iterator iter;
    unsigned int cc = 0;
    for (iter = this->ProxyList.begin(); iter != this->ProxyList.end(); ++iter, ++cc)
    {
      if (cc == index)
      {
        iter->RemoveObservers();
        this->ProxyList.erase(iter);
        return true;
      }
    }
    return false;
  }

  // Removes all proxies from the Proxies list. Will also cleanup any "links" setup
  // during AddProxy() calls.
  void ClearProxies()
  {
    VectorOfProxies::iterator iter;
    for (iter = this->ProxyList.begin(); iter != this->ProxyList.end(); ++iter)
    {
      iter->RemoveObservers();
    }
    this->ProxyList.clear();
  }

  ~vtkSMProxyListDomainInternals() { this->ClearProxies(); }

public:
  std::vector<vtkSMProxyListDomain::ProxyType> ProxyTypeList;

private:
  VectorOfProxies ProxyList;
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
const std::vector<vtkSMProxyListDomain::ProxyType>& vtkSMProxyListDomain::GetProxyTypes() const
{
  return this->Internals->ProxyTypeList;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::CreateProxies(vtkSMSessionProxyManager* pxm)
{
  assert(pxm);
  for (const auto& apair : this->Internals->ProxyTypeList)
  {
    if (vtkSMProxy* proxy = pxm->NewProxy(apair.GroupName.c_str(), apair.ProxyName.c_str()))
    {
      this->Internals->AddProxy(proxy, this);
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
  this->Internals->ProxyTypeList.push_back(ProxyType(group, name));
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
    return nullptr;
  }

  return this->Internals->ProxyTypeList[cc].GroupName.c_str();
}

//-----------------------------------------------------------------------------
const char* vtkSMProxyListDomain::GetProxyName(unsigned int cc)
{
  if (this->GetNumberOfProxyTypes() <= cc)
  {
    vtkErrorMacro("Invalid index " << cc);
    return nullptr;
  }

  return this->Internals->ProxyTypeList[cc].ProxyName.c_str();
}

//-----------------------------------------------------------------------------
const char* vtkSMProxyListDomain::GetProxyName(vtkSMProxy* proxy)
{
  return proxy ? proxy->GetXMLName() : nullptr;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyListDomain::GetProxyWithName(const char* pname)
{
  vtkSMProxyListDomainInternals::VectorOfProxies::const_iterator iter;
  const vtkSMProxyListDomainInternals::VectorOfProxies& proxies = this->Internals->GetProxies();
  for (iter = proxies.begin(); pname != nullptr && iter != proxies.end(); iter++)
  {
    if (iter->Proxy && iter->Proxy->GetXMLName() && strcmp(iter->Proxy->GetXMLName(), pname) == 0)
    {
      return iter->Proxy;
    }
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(prop);
  if (pp && this->GetNumberOfProxies() > 0)
  {
    vtkSMPropertyHelper helper(prop);
    helper.SetUseUnchecked(use_unchecked_values);
    vtkSMProxy* values[1] = { this->GetProxy(this->GetDefaultIndex()) };
    helper.Set(values, 1);
    return 1;
  }

  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  unsigned int max = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < max; ++cc)
  {
    vtkPVXMLElement* proxyElement = element->GetNestedElement(cc);
    if (strcmp(proxyElement->GetName(), "Proxy") == 0)
    {
      const char* name = proxyElement->GetAttribute("name");
      const char* group = proxyElement->GetAttribute("group");
      if (name && group)
      {
        this->AddProxy(group, name);
      }
    }
    else if (strcmp(proxyElement->GetName(), "Group") == 0)
    {
      // Recover group name
      const char* name = proxyElement->GetAttribute("name");

      if (name)
      {
        // Browse group and recover each proxy type
        vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
        vtkSMProxyDefinitionManager* pxdm = pxm->GetProxyDefinitionManager();
        if (!pxdm)
        {
          vtkErrorMacro("No vtkSMProxyDefinitionManager available in vtkSMSessionProxyManager, "
                        "cannot generate proxy list for groups");
          continue;
        }
        else
        {
          vtkPVProxyDefinitionIterator* iter = pxdm->NewSingleGroupIterator(name);
          for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
          {
            this->AddProxy(name, iter->GetProxyName());
          }
          iter->Delete();
        }
      }
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::AddProxy(vtkSMProxy* proxy)
{
  this->Internals->AddProxy(proxy, this);
  this->DomainModified();
}

//-----------------------------------------------------------------------------
unsigned int vtkSMProxyListDomain::GetNumberOfProxies()
{
  return static_cast<unsigned int>(this->Internals->GetProxies().size());
}

//-----------------------------------------------------------------------------
bool vtkSMProxyListDomain::HasProxy(vtkSMProxy* proxy)
{
  const vtkSMProxyListDomainInternals::VectorOfProxies& proxies = this->Internals->GetProxies();
  vtkSMProxyListDomainInternals::VectorOfProxies::const_iterator iter;
  for (iter = proxies.begin(); iter != proxies.end(); iter++)
  {
    if (iter->Proxy == proxy)
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyListDomain::GetProxy(unsigned int index)
{
  const vtkSMProxyListDomainInternals::VectorOfProxies& proxies = this->Internals->GetProxies();
  if (index >= proxies.size())
  {
    vtkErrorMacro("Index " << index << " greater than max " << proxies.size());
    return nullptr;
  }
  return this->Internals->GetProxies()[index].Proxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyListDomain::FindProxy(const char* xmlgroup, const char* xmlname)
{
  vtkSMProxyListDomainInternals::VectorOfProxies::const_iterator iter;
  const vtkSMProxyListDomainInternals::VectorOfProxies& proxies = this->Internals->GetProxies();
  for (iter = proxies.begin(); xmlgroup != nullptr && xmlname != nullptr && iter != proxies.end();
       iter++)
  {
    if (iter->Proxy && iter->Proxy->GetXMLGroup() && iter->Proxy->GetXMLName() &&
      strcmp(iter->Proxy->GetXMLGroup(), xmlgroup) == 0 &&
      strcmp(iter->Proxy->GetXMLName(), xmlname) == 0)
    {
      return iter->Proxy;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::RemoveProxy(vtkSMProxy* proxy)
{
  if (this->Internals->RemoveProxy(proxy))
  {
    this->DomainModified();
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::RemoveProxy(unsigned int index)
{
  if (this->Internals->RemoveProxy(index))
  {
    this->DomainModified();
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::ChildSaveState(vtkPVXMLElement* element)
{
  this->Superclass::ChildSaveState(element);

  vtkSMProxyListDomainInternals::VectorOfProxies::const_iterator iter;
  const vtkSMProxyListDomainInternals::VectorOfProxies& proxies = this->Internals->GetProxies();
  for (iter = proxies.begin(); iter != proxies.end(); ++iter)
  {
    vtkSMProxy* proxy = iter->Proxy.GetPointer();
    vtkPVXMLElement* proxyElem = vtkPVXMLElement::New();
    proxyElem->SetName("Proxy");
    proxyElem->AddAttribute("value", static_cast<int>(proxy->GetGlobalID()));
    element->AddNestedElement(proxyElem);
    proxyElem->Delete();
  }
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader)
{
  this->Internals->ClearProxies();
  if (!this->Superclass::LoadState(element, loader))
  {
    return 0;
  }

  for (unsigned int cc = 0; cc < element->GetNumberOfNestedElements(); cc++)
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
          this->Internals->AddProxy(proxy, this);
        }
      }
    }
  }
  this->DomainModified();
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::SetProxies(vtkSMProxy** proxies, unsigned int count)
{
  this->Internals->ClearProxies();
  for (unsigned int cc = 0; cc < count; ++cc)
  {
    this->Internals->AddProxy(proxies[cc], this);
  }
  this->DomainModified();
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::SetLogName(const char* prefix)
{
  for (const auto& item : this->Internals->GetProxies())
  {
    if (item.Proxy != nullptr)
    {
      std::string name = std::string(prefix ? prefix : "") + "/" + item.Proxy->GetXMLName();
      item.Proxy->SetLogName(name.c_str());
    }
  }
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
