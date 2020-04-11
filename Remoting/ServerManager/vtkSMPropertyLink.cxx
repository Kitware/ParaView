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
#include "vtkSMMessage.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkWeakPointer.h"

#include <list>

vtkStandardNewMacro(vtkSMPropertyLink);
//-----------------------------------------------------------------------------
class vtkSMPropertyLinkInternals
{
public:
  struct LinkedProperty
  {
  public:
    LinkedProperty(vtkSMProxy* proxy, const char* pname, int updateDir)
      : Proxy(proxy)
      , PropertyName(pname)
      , UpdateDirection(updateDir)
      , Observer(nullptr)
    {
    }

    ~LinkedProperty()
    {
      if (this->Observer && this->Proxy && this->Proxy.GetPointer())
      {
        this->Proxy.GetPointer()->RemoveObserver(this->Observer);
      }
      this->Observer = nullptr;
    }

    // Either (Proxy, PropertyName) pair is valid,
    // depending on the API used to add the link.
    vtkWeakPointer<vtkSMProxy> Proxy;
    std::string PropertyName;

    int UpdateDirection;
    vtkWeakPointer<vtkCommand> Observer;
  };

  typedef std::list<LinkedProperty> LinkedPropertyType;
  LinkedPropertyType LinkedProperties;
};

//-----------------------------------------------------------------------------
vtkSMPropertyLink::vtkSMPropertyLink()
{
  this->Internals = new vtkSMPropertyLinkInternals;
  this->ModifyingProperty = false;
}

//-----------------------------------------------------------------------------
vtkSMPropertyLink::~vtkSMPropertyLink()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::AddLinkedProperty(vtkSMProxy* proxy, const char* pname, int updateDir)
{
  int addToList = 1;
  int addObserver = updateDir & INPUT;
  bool input_exists = false;

  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
  {
    if (iter->Proxy.GetPointer() == proxy && iter->PropertyName == pname &&
      iter->UpdateDirection == updateDir)
    {
      addObserver = 0;
      addToList = 0;
    }
    if (iter->UpdateDirection & INPUT)
    {
      input_exists = true;
    }
  }

  if (addToList && input_exists && (updateDir & INPUT))
  {
    /* For now, allow multiple inputs. This strategy needs some evaluation.
    vtkErrorMacro("Only one input can be added at a time.");
    return;
    */
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

  this->Synchronize();

  this->Modified();

  // Update state and push it to share
  this->UpdateState();
  this->PushStateToSession();
}

//-----------------------------------------------------------------------------
// Find input property and update all output properties
// to match the value of the input property.
void vtkSMPropertyLink::Synchronize()
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); iter++)
  {
    if (iter->UpdateDirection & INPUT)
    {
      if (iter->Proxy)
      {
        this->PropertyModified(iter->Proxy, iter->PropertyName.c_str());
      }
      break;
    }
  }
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::RemoveAllLinks()
{
  this->Internals->LinkedProperties.clear();
  this->State->ClearExtension(LinkState::link);
  this->Modified();

  // Update state and push it to share
  this->UpdateState();
  this->PushStateToSession();
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::RemoveLinkedProperty(vtkSMProxy* proxy, const char* pname)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
  {
    if (iter->Proxy == proxy && iter->PropertyName == pname)
    {
      this->Internals->LinkedProperties.erase(iter);
      this->Modified();

      // Update state and push it to share
      this->UpdateState();
      this->PushStateToSession();

      break;
    }
  }
}

//-----------------------------------------------------------------------------
unsigned int vtkSMPropertyLink::GetNumberOfLinkedObjects()
{
  return static_cast<unsigned int>(this->Internals->LinkedProperties.size());
}

//-----------------------------------------------------------------------------
unsigned int vtkSMPropertyLink::GetNumberOfLinkedProperties()
{
  vtkWarningMacro("GetNumberOfLinkedProperties() is deprecated, "
                  "please use GetNumberOfLinkedObjects() instead");
  return this->GetNumberOfLinkedObjects();
}

//-----------------------------------------------------------------------------
const char* vtkSMPropertyLink::GetLinkedPropertyName(int index)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (int i = 0; i < index && iter != this->Internals->LinkedProperties.end(); i++)
  {
    iter++;
  }
  if (iter == this->Internals->LinkedProperties.end())
  {
    return NULL;
  }
  return iter->PropertyName.c_str();
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPropertyLink::GetLinkedProxy(int index)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (int i = 0; i < index && iter != this->Internals->LinkedProperties.end(); i++)
  {
    iter++;
  }
  if (iter == this->Internals->LinkedProperties.end())
  {
    return NULL;
  }
  return iter->Proxy;
}

//-----------------------------------------------------------------------------
int vtkSMPropertyLink::GetLinkedObjectDirection(int index)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (int i = 0; i < index && iter != this->Internals->LinkedProperties.end(); i++)
  {
    iter++;
  }
  if (iter == this->Internals->LinkedProperties.end())
  {
    return NONE;
  }
  return iter->UpdateDirection;
}

//-----------------------------------------------------------------------------
int vtkSMPropertyLink::GetLinkedPropertyDirection(int index)
{
  vtkWarningMacro("GetLinkedPropertyDirection(int index) is deprecated, "
                  "please use GetLinkedObjectDirection(int index) instead");
  return this->GetLinkedObjectDirection(index);
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::PropertyModified(vtkSMProxy* fromProxy, const char* pname)
{
  if (this->ModifyingProperty)
  {
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

  this->ModifyingProperty = true;
  // First verify that the property that triggered this call is indeed
  // an input property.
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter;
  int propagate = 0;
  for (iter = this->Internals->LinkedProperties.begin();
       iter != this->Internals->LinkedProperties.end(); ++iter)
  {
    if (iter->UpdateDirection & INPUT && iter->Proxy.GetPointer() == fromProxy &&
      iter->PropertyName == pname)
    {
      propagate = 1;
      break;
    }
  }

  if (!propagate)
  {
    this->ModifyingProperty = false;
    return;
  }

  // Propagate the changes.
  for (iter = this->Internals->LinkedProperties.begin();
       iter != this->Internals->LinkedProperties.end(); ++iter)
  {
    if (iter->UpdateDirection & OUTPUT)
    {
      vtkSMProperty* toProp = 0;
      if (iter->Proxy.GetPointer())
      {
        toProp = iter->Proxy.GetPointer()->GetProperty(iter->PropertyName.c_str());
      }
      if (toProp && (toProp != fromProp))
      {
        toProp->Copy(fromProp);
      }
    }
  }
  this->ModifyingProperty = false;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::UpdateProperty(vtkSMProxy* caller, const char* pname)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter;

  // First check if pname is indeed an input property.
  bool is_input = false;
  iter = this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
  {
    if ((iter->Proxy.GetPointer() == caller) && (iter->PropertyName == pname) &&
      (iter->UpdateDirection & INPUT))
    {
      is_input = true;
      break;
    }
  }

  // Update all output properties.
  if (is_input)
  {
    iter = this->Internals->LinkedProperties.begin();
    for (; iter != this->Internals->LinkedProperties.end(); ++iter)
    {
      if ((iter->Proxy.GetPointer() != caller) && (iter->UpdateDirection & OUTPUT))
      {
        iter->Proxy.GetPointer()->UpdateProperty(iter->PropertyName.c_str());
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::UpdateVTKObjects(vtkSMProxy* caller)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
  {
    if ((iter->Proxy.GetPointer() != caller) && (iter->UpdateDirection & OUTPUT))
    {
      iter->Proxy.GetPointer()->UpdateVTKObjects();
    }
  }
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::SaveXMLState(const char* linkname, vtkPVXMLElement* parent)
{
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("PropertyLink");
  root->AddAttribute("name", linkname);
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
  {
    vtkPVXMLElement* child = vtkPVXMLElement::New();
    child->SetName("Property");
    child->AddAttribute("id", static_cast<unsigned int>(iter->Proxy.GetPointer()->GetGlobalID()));
    child->AddAttribute("name", iter->PropertyName.c_str());
    child->AddAttribute("direction", ((iter->UpdateDirection & INPUT) ? "input" : "output"));
    root->AddNestedElement(child);
    child->Delete();
  }
  parent->AddNestedElement(root);
  root->Delete();
}

//-----------------------------------------------------------------------------
int vtkSMPropertyLink::LoadXMLState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator)
{
  unsigned int numElems = linkElement->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
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
    vtkSMProxy* proxy = locator->LocateProxy(id);
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
  }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//-----------------------------------------------------------------------------
void vtkSMPropertyLink::LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator)
{
  this->Superclass::LoadState(msg, locator);

  // Reset old state
  this->Internals->LinkedProperties.clear();

  // Load Property Links
  int numberOfLinks = msg->ExtensionSize(LinkState::link);
  for (int i = 0; i < numberOfLinks; i++)
  {
    const LinkState_LinkDescription* link = &msg->GetExtension(LinkState::link, i);
    vtkSMProxy* proxy = locator->LocateProxy(link->proxy());

    assert("property name must be set for PropertyLink" && link->has_property_name());

    if (proxy)
    {
      switch (link->direction())
      {
        case LinkState_LinkDescription::NONE:
          this->AddLinkedProperty(proxy, link->property_name().c_str(), vtkSMLink::NONE);
          break;
        case LinkState_LinkDescription::INPUT:
          this->AddLinkedProperty(proxy, link->property_name().c_str(), vtkSMLink::INPUT);
          break;
        case LinkState_LinkDescription::OUTPUT:
          this->AddLinkedProperty(proxy, link->property_name().c_str(), vtkSMLink::OUTPUT);
          break;
      }
    }
    else
    {
      vtkDebugMacro("Proxy not found with ID: " << link->proxy());
    }
  }
}
//-----------------------------------------------------------------------------
void vtkSMPropertyLink::UpdateState()
{
  if (this->Session == NULL)
  {
    return;
  }

  this->State->ClearExtension(LinkState::link);
  this->State->ClearExtension(LinkState::exception_property);

  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
  {
    if (!iter->Proxy.GetPointer())
    {
      continue;
    }
    LinkState_LinkDescription* link = this->State->AddExtension(LinkState::link);
    link->set_proxy(iter->Proxy.GetPointer()->GetGlobalID());
    switch (iter->UpdateDirection)
    {
      case vtkSMLink::NONE:
        link->set_direction(LinkState_LinkDescription::NONE);
        break;
      case vtkSMLink::INPUT:
        link->set_direction(LinkState_LinkDescription::INPUT);
        break;
      case vtkSMLink::OUTPUT:
        link->set_direction(LinkState_LinkDescription::OUTPUT);
        break;
      default:
        vtkErrorMacro("Invalid Link direction");
        break;
    }
    link->set_property_name(iter->PropertyName.c_str());
  }
}
