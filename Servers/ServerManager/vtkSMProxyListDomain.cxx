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
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkPVXMLElement.h"
#include "vtkSMStateLoader.h"

#include <vtkstd/string>
#include <vtkstd/vector>

//-----------------------------------------------------------------------------
class vtkSMProxyListDomainInternals
{
public:
  typedef vtkstd::vector<vtkSmartPointer<vtkSMProxy> > VectorOfProxies;
  VectorOfProxies ProxyList;

  struct ProxyInfo
    {
    vtkstd::string GroupName;
    vtkstd::string ProxyName;
    };

  typedef vtkstd::vector<ProxyInfo> VectorOfProxyInfo;
  VectorOfProxyInfo ProxyTypeList;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMProxyListDomain);
vtkCxxRevisionMacro(vtkSMProxyListDomain, "1.2");
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
int vtkSMProxyListDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
    {
    return 1;
    }

  if (!property)
    {
    return 0;
    }
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(property);
  if (pp)
    {
    unsigned int numProxies = pp->GetNumberOfUncheckedProxies();
    for (unsigned int i=0; i<numProxies; i++)
      {
      if (!this->IsInDomain(pp->GetUncheckedProxy(i)))
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::IsInDomain(vtkSMProxy* proxy)
{
  vtkSMProxyListDomainInternals::VectorOfProxies::iterator iter =
    this->Internals->ProxyList.begin();
  for (; iter != this->Internals->ProxyList.end(); ++iter)
    {
    if (iter->GetPointer() == proxy)
      {
      return 1;
      }
    }
  return 0;
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
void vtkSMProxyListDomain::CreateProxyList(vtkIdType connectionId)
{
  this->Internals->ProxyList.clear();

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  vtkSMProxyListDomainInternals::VectorOfProxyInfo::iterator iter =
    this->Internals->ProxyTypeList.begin();
  for (; iter != this->Internals->ProxyTypeList.end(); ++iter)
    {
    vtkSMProxy* proxy = pxm->NewProxy(iter->GroupName.c_str(), 
      iter->ProxyName.c_str());
    if (proxy)
      {
      proxy->SetConnectionID(connectionId);
      this->Internals->ProxyList.push_back(proxy);
      proxy->Delete();
      }
    }
}

//-----------------------------------------------------------------------------
unsigned int vtkSMProxyListDomain::GetNumberOfProxies()
{
  return this->Internals->ProxyList.size();
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyListDomain::GetProxy(unsigned int cc)
{
  if (this->GetNumberOfProxies() <= cc)
    {
    vtkErrorMacro("Invalid index " << cc);
    return NULL;
    }

  return this->Internals->ProxyList[cc];
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::GetIndex(vtkSMProxy* proxy)
{
  vtkSMProxyListDomainInternals::VectorOfProxies::iterator iter =
    this->Internals->ProxyList.begin();
  for (unsigned int cc=0; iter != this->Internals->ProxyList.end(); ++iter, cc++)
    {
    if (iter->GetPointer() == proxy)
      {
      return cc;
      }
    }
  return -1;
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
void vtkSMProxyListDomain::ChildSaveState(vtkPVXMLElement* domainElement)
{
  this->Superclass::ChildSaveState(domainElement);

  unsigned int size = this->GetNumberOfProxies();
  for (unsigned int cc=0; cc < size; cc++)
    {
    vtkPVXMLElement* element = vtkPVXMLElement::New();
    element->SetName("Proxy");
    element->AddAttribute("id", this->GetProxy(cc)->GetSelfIDAsString());
    domainElement->AddNestedElement(element);
    element->Delete();
    }
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::LoadState(vtkPVXMLElement* domainElement,
    vtkSMStateLoader* loader)
{
  this->Internals->ProxyList.clear();
  if (!this->Superclass::LoadState(domainElement, loader))
    {
    return 0;
    }

  unsigned int max = domainElement->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < max; cc++)
    {
    vtkPVXMLElement* child= domainElement->GetNestedElement(cc);
    if (strcmp(child->GetName(), "Proxy") == 0)
      {
      int id;
      if (!child->GetScalarAttribute("id", &id))
        {
        vtkWarningMacro("Proxy element missing required attribute id.");
        continue;
        }
      vtkSMProxy* proxy = loader->NewProxy(id);
      if (proxy)
        {
        this->Internals->ProxyList.push_back(proxy);
        proxy->Delete();
        }
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
