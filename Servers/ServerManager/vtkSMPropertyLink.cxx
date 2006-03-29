/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyLink.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPropertyLink.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStateLoader.h"
#include "vtkStdString.h"

#include <vtkstd/list>

vtkStandardNewMacro(vtkSMPropertyLink);
vtkCxxRevisionMacro(vtkSMPropertyLink, "1.5");
//-----------------------------------------------------------------------------
struct vtkSMPropertyLinkInternals
{
public:
  struct LinkedProperty
    {
  public:
    LinkedProperty(vtkSMProxy* proxy, const char* pname, int updateDir) :
      Proxy(proxy), PropertyName(pname), UpdateDirection(updateDir), 
      Observer(0)
    {
    }
    ~LinkedProperty()
      {
      if (this->Observer && this->Proxy.GetPointer())
        {
        this->Proxy.GetPointer()->RemoveObserver(Observer);
        this->Observer = 0;
        }
      }

    vtkSmartPointer<vtkSMProxy> Proxy;
    vtkStdString PropertyName;
    int UpdateDirection;
    vtkCommand* Observer;
    };
  typedef vtkstd::list<LinkedProperty> LinkedPropertyType;
  LinkedPropertyType LinkedProperties;
};

//-----------------------------------------------------------------------------
vtkSMPropertyLink::vtkSMPropertyLink()
{
  this->Internals = new vtkSMPropertyLinkInternals;
}

//-----------------------------------------------------------------------------
vtkSMPropertyLink::~vtkSMPropertyLink()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::AddLinkedProperty(vtkSMProxy* proxy, const char* pname,
  int updateDir)
{
  int addToList = 1;
  int addObserver = updateDir & INPUT;
  
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if (iter->Proxy.GetPointer() == proxy && 
      iter->PropertyName == pname)
      {
      if (iter->UpdateDirection != updateDir)
        {
        iter->UpdateDirection = updateDir;
        if (addObserver)
          {
          iter->Observer = this->Observer;
          }
        }
      else
        {
        addObserver = 0;
        }
      addToList = 0;
      }
    }
  if (addToList)
    {
    vtkSMPropertyLinkInternals::LinkedProperty link(proxy, pname, updateDir);
    this->Internals->LinkedProperties.push_back(link);
    if (addObserver)
      {
      this->Internals->LinkedProperties.back().Observer = this->Observer;
      }
    }

  if (addObserver)
    {
    this->ObserveProxyUpdates(proxy);
    }
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::UpdateProperties(vtkSMProxy* fromProxy, const char* pname)
{
  if (!fromProxy)
    {
    return;
    }

  vtkSMProperty* fromProp = fromProxy->GetProperty(pname);
  if (!fromProp)
    {
    return;
    }

  // First verify that the property that triggerred this call is indeed
  // an input property.
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter;
  int propagate = 0;
  for (iter = this->Internals->LinkedProperties.begin(); 
    iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if (iter->UpdateDirection & INPUT && iter->Proxy.GetPointer() == fromProxy
      && iter->PropertyName == pname)
      {
      propagate = 1;
      break;
      }
    }

  if (!propagate)
    {
    return;
    }
 
  // Propagate the changes.
  for (iter = this->Internals->LinkedProperties.begin(); 
    iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if (iter->UpdateDirection & OUTPUT)
      {
      vtkSMProperty* toProp = iter->Proxy.GetPointer()->GetProperty(
        iter->PropertyName);
      if (toProp)
        {
        toProp->Copy(fromProp);
        toProp->Modified();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::UpdateVTKObjects(vtkSMProxy* caller)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for(; iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if ((iter->Proxy.GetPointer() != caller) && 
        (iter->UpdateDirection & OUTPUT))
      {
      iter->Proxy.GetPointer()->UpdateVTKObjects();
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::SaveState(const char* linkname, vtkPVXMLElement* parent)
{
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("PropertyLink");
  root->AddAttribute("name", linkname);
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for(; iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    vtkPVXMLElement* child = vtkPVXMLElement::New();
    child->SetName("Property");
    child->AddAttribute("id", iter->Proxy.GetPointer()->GetSelfIDAsString());
    child->AddAttribute("name", iter->PropertyName);
    child->AddAttribute("direction", ( (iter->UpdateDirection & INPUT)? 
        "input" : "output"));
    root->AddNestedElement(child);
    child->Delete();
    }
  parent->AddNestedElement(root);
  root->Delete();
}

//-----------------------------------------------------------------------------
int vtkSMPropertyLink::LoadState(vtkPVXMLElement* linkElement, 
  vtkSMStateLoader* loader)
{
  unsigned int numElems = linkElement->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* child = linkElement->GetNestedElement(cc);
    if (!child->GetName() || strcmp(child->GetName(), "Property") != 0)
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
    vtkSMProxy* proxy = loader->NewProxy(id); 
    if (!proxy)
      {
      vtkErrorMacro("Failed to locate proxy with ID: " << id);
      return 0;
      }

    const char* pname = child->GetAttribute("name");
    if (!pname)
      {
      vtkErrorMacro("State missing required attribute name.");
      return 0;
      }
    this->AddLinkedProperty(proxy, pname, idirection);
    proxy->Delete(); 
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
