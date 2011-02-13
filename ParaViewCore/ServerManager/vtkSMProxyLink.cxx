/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyLink.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyLink.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"

#include <vtkstd/list>
#include <vtkstd/set>
#include <vtkstd/string>

vtkStandardNewMacro(vtkSMProxyLink);

//---------------------------------------------------------------------------
struct vtkSMProxyLinkInternals
{
  struct LinkedProxy
  {
    LinkedProxy(vtkSMProxy* proxy, int updateDir) : 
      Proxy(proxy), UpdateDirection(updateDir), Observer(0)
      {
      };
    ~LinkedProxy()
      {
        if (this->Observer && this->Proxy.GetPointer())
          {
          this->Proxy.GetPointer()->RemoveObserver(Observer);
          this->Observer = 0;
          }
      }
    vtkSmartPointer<vtkSMProxy> Proxy;
    int UpdateDirection;
    vtkCommand* Observer;
  };

  typedef vtkstd::list<LinkedProxy> LinkedProxiesType;
  LinkedProxiesType LinkedProxies;

  typedef vtkstd::set<vtkstd::string> ExceptionPropertiesType;
  ExceptionPropertiesType ExceptionProperties;
};

//---------------------------------------------------------------------------
vtkSMProxyLink::vtkSMProxyLink()
{
  this->Internals = new vtkSMProxyLinkInternals;
}

//---------------------------------------------------------------------------
vtkSMProxyLink::~vtkSMProxyLink()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::AddLinkedProxy(vtkSMProxy* proxy, int updateDir)
{
  int addToList = 1;
  int addObserver = updateDir & INPUT;

  vtkSMProxyLinkInternals::LinkedProxiesType::iterator iter =
    this->Internals->LinkedProxies.begin();
  for(; iter != this->Internals->LinkedProxies.end(); iter++)
    {
    if (iter->Proxy == proxy && iter->UpdateDirection == updateDir)
      {
      addObserver = 0;
      addToList = 0;
      }
    }

  if (addToList)
    {
    vtkSMProxyLinkInternals::LinkedProxy link(proxy, updateDir);
    this->Internals->LinkedProxies.push_back(link);
    if (addObserver)
      {
      this->Internals->LinkedProxies.back().Observer = this->Observer;
      }
    }

  if (addObserver)
    {
    this->ObserveProxyUpdates(proxy);
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSMProxyLink::RemoveAllLinks()
{
  this->Internals->LinkedProxies.clear();
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::RemoveLinkedProxy(vtkSMProxy* proxy)
{
  vtkSMProxyLinkInternals::LinkedProxiesType::iterator iter =
    this->Internals->LinkedProxies.begin();
  for(; iter != this->Internals->LinkedProxies.end(); iter++)
    {
    if (iter->Proxy == proxy)
      {
      this->Internals->LinkedProxies.erase(iter);
      this->Modified();
      break;
      }
    }
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyLink::GetNumberOfLinkedProxies()
{
  return static_cast<unsigned int>(this->Internals->LinkedProxies.size());
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyLink::GetLinkedProxy(int index)
{
  vtkSMProxyLinkInternals::LinkedProxiesType::iterator iter =
    this->Internals->LinkedProxies.begin();
  for(int i=0;
      i<index && iter != this->Internals->LinkedProxies.end();
      i++)
    {
    iter++;
    }
  if(iter == this->Internals->LinkedProxies.end())
    {
    return NULL;
    }
  return iter->Proxy;
}

//---------------------------------------------------------------------------
int vtkSMProxyLink::GetLinkedProxyDirection(int index)
{
  vtkSMProxyLinkInternals::LinkedProxiesType::iterator iter =
    this->Internals->LinkedProxies.begin();
  for(int i=0;
      i<index && iter != this->Internals->LinkedProxies.end();
      i++)
    {
    iter++;
    }
  if(iter == this->Internals->LinkedProxies.end())
    {
    return NONE;
    }
  return iter->UpdateDirection;
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::AddException(const char* propertyname)
{
  this->Internals->ExceptionProperties.insert(propertyname);
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::RemoveException(const char* propertyname)
{
  vtkSMProxyLinkInternals::ExceptionPropertiesType::iterator iter
    = this->Internals->ExceptionProperties.find(propertyname);
  if (iter != this->Internals->ExceptionProperties.end())
    {
    this->Internals->ExceptionProperties.erase(iter);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::PropertyModified(vtkSMProxy* fromProxy, const char* pname)
{
  if (pname && this->Internals->ExceptionProperties.find(pname) !=
    this->Internals->ExceptionProperties.end())
    {
    // Property is in exception list.
    return;
    }

  if (!fromProxy)
    {
    return;
    }
  vtkSMProperty* fromProp = fromProxy->GetProperty(pname);
  if (!fromProp)
    {
    return;
    }

  vtkSMProxyLinkInternals::LinkedProxiesType::iterator iter =
    this->Internals->LinkedProxies.begin();
  for(; iter != this->Internals->LinkedProxies.end(); iter++)
    {
    if ((iter->Proxy.GetPointer() != fromProxy) && 
        (iter->UpdateDirection & OUTPUT))
      {
      vtkSMProperty* toProp = iter->Proxy->GetProperty(pname);
      if (toProp)
        {
        toProp->Copy(fromProp);
        }
      }
    }
}


//---------------------------------------------------------------------------
void vtkSMProxyLink::UpdateProperty(vtkSMProxy* caller, const char* pname)
{
  if (pname && this->Internals->ExceptionProperties.find(pname) !=
    this->Internals->ExceptionProperties.end())
    {
    // Property is in exception list.
    return;
    }

  vtkSMProxyLinkInternals::LinkedProxiesType::iterator iter =
    this->Internals->LinkedProxies.begin();
  for(; iter != this->Internals->LinkedProxies.end(); iter++)
    {
    if ((iter->Proxy.GetPointer() != caller) && 
        (iter->UpdateDirection & OUTPUT))
      {
      iter->Proxy->UpdateProperty(pname);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::UpdateVTKObjects(vtkSMProxy* caller)
{
  vtkSMProxyLinkInternals::LinkedProxiesType::iterator iter =
    this->Internals->LinkedProxies.begin();
  for(; iter != this->Internals->LinkedProxies.end(); iter++)
    {
    if ((iter->Proxy.GetPointer() != caller) && 
        (iter->UpdateDirection & OUTPUT))
      {
      iter->Proxy->UpdateVTKObjects();
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::SaveState(const char* linkname, vtkPVXMLElement* parent)
{
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("ProxyLink");
  root->AddAttribute("name", linkname);
  vtkSMProxyLinkInternals::LinkedProxiesType::iterator iter =
    this->Internals->LinkedProxies.begin();
  for(; iter != this->Internals->LinkedProxies.end(); ++iter)
    {
    vtkPVXMLElement* child = vtkPVXMLElement::New();
    child->SetName("Proxy");
    child->AddAttribute("direction", (iter->UpdateDirection == INPUT? "input" : "output"));
    child->AddAttribute( "id",
                         static_cast<unsigned int>(iter->Proxy.GetPointer()->GetGlobalID()));
    root->AddNestedElement(child);
    child->Delete();
    }
  parent->AddNestedElement(root);
  root->Delete();
}

//---------------------------------------------------------------------------
int vtkSMProxyLink::LoadState(
  vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator)
{
  unsigned int numElems = linkElement->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* child = linkElement->GetNestedElement(cc);
    if (!child->GetName() || strcmp(child->GetName(), "Proxy") != 0)
      {
      vtkWarningMacro("Invalid element in link state. Ignoring.");
      continue;
      }
    const char* direction = child->GetAttribute("direction");
    if (!direction)
      {
      vtkErrorMacro("State missing required attribute direction.");
      return 0;
      }
    int idirection;
    if (strcmp(direction, "input") == 0)
      {
      idirection = INPUT;
      }
    else if (strcmp(direction, "output") == 0)
      {
      idirection = OUTPUT;
      }
    else
      {
      vtkErrorMacro("Invalid value for direction: " << direction);
      return 0;
      }
    int id;
    if (!child->GetScalarAttribute("id", &id))
      {
      vtkErrorMacro("State missing required attribute id.");
      return 0;
      }
    vtkSMProxy* proxy = locator->LocateProxy(id); 
    if (!proxy)
      {
      vtkErrorMacro("Failed to locate proxy with ID: " << id);
      return 0;
      }
    this->AddLinkedProxy(proxy, idirection);
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


