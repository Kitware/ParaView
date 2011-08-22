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
#include "vtkSMProxyLocator.h"
#include "vtkStdString.h"

#include <vtkstd/list>

vtkStandardNewMacro(vtkSMPropertyLink);
//-----------------------------------------------------------------------------
class vtkSMPropertyLinkObserver : public vtkCommand
{
public:
  static vtkSMPropertyLinkObserver* New() 
    {
    return new vtkSMPropertyLinkObserver;
    }

  void SetTarget(vtkSMPropertyLink* t) 
    { 
    this->Target = t;
    }
  virtual void Execute(vtkObject *c, unsigned long , void* )
    {
    vtkSMProperty* caller = vtkSMProperty::SafeDownCast(c);
    if (this->Target && caller && this->Target->GetEnabled())
      {
      this->Target->PropertyModified(caller);
      }
    }

protected:
  vtkSMPropertyLinkObserver() 
    {
    this->Target = 0;
    }

  vtkSMPropertyLink* Target;
};

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

    LinkedProperty(vtkSMProperty* property, int updateDir) :
      Property(property), UpdateDirection(updateDir), 
      Observer(0)
    {
    }

    ~LinkedProperty()
      {
      if (this->Observer && this->Proxy.GetPointer())
        {
        this->Proxy.GetPointer()->RemoveObserver(this->Observer);
        }

      if (this->Observer && this->Property.GetPointer())
        {
        this->Property->RemoveObserver(this->Observer);
        }
      this->Observer = 0;

      }

    // Either (Proxy, PropertyName) pair is valid or (Property) is valid,
    // depending on the API used to add the link.
    vtkSmartPointer<vtkSMProxy> Proxy;
    vtkStdString PropertyName;
    vtkSmartPointer<vtkSMProperty> Property;

    int UpdateDirection;
    vtkCommand* Observer;
    };

  typedef vtkstd::list<LinkedProperty> LinkedPropertyType;
  LinkedPropertyType LinkedProperties;
  vtkSMPropertyLinkObserver* PropertyObserver;
};

//-----------------------------------------------------------------------------
vtkSMPropertyLink::vtkSMPropertyLink()
{
  this->Internals = new vtkSMPropertyLinkInternals;
  this->Internals->PropertyObserver = vtkSMPropertyLinkObserver::New();
  this->Internals->PropertyObserver->SetTarget(this);
  this->ModifyingProperty = false;
}

//-----------------------------------------------------------------------------
vtkSMPropertyLink::~vtkSMPropertyLink()
{
  this->Internals->PropertyObserver->SetTarget(0);
  this->Internals->PropertyObserver->Delete();
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::AddLinkedProperty(vtkSMProxy* proxy, const char* pname,
  int updateDir)
{
  int addToList = 1;
  int addObserver = updateDir & INPUT;
  bool input_exists = false;
  
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if (iter->Proxy.GetPointer() == proxy && 
      iter->PropertyName == pname && 
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

  if (addToList && input_exists && (updateDir & INPUT) )
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
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::AddLinkedProperty(vtkSMProperty* property, int updateDir)
{
  if (!property)
    {
    vtkErrorMacro("Cannot add link to a NULL property.");
    return;
    }
  int addToList = 1;
  int addObserver = updateDir & INPUT;

  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if (iter->Property == property && iter->UpdateDirection == updateDir)
      {
      addObserver = 0;
      addToList = 0;
      }
    }

  if (addToList)
    {
    vtkSMPropertyLinkInternals::LinkedProperty link(property, updateDir);
    this->Internals->LinkedProperties.push_back(link);
    if (addObserver)
      {
      this->Internals->LinkedProperties.back().Observer = 
        this->Internals->PropertyObserver;
      }
    }

  if (addObserver)
    {
    property->AddObserver(vtkCommand::ModifiedEvent, 
      this->Internals->PropertyObserver);
    }
  
  this->Synchronize();
  this->Modified();
}


//-----------------------------------------------------------------------------
// Find input property and update all output properties
// to match the value of the input property.
void vtkSMPropertyLink::Synchronize()
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for(; iter != this->Internals->LinkedProperties.end(); iter++)
    { 
    if (iter->UpdateDirection & INPUT)
      {
      if (iter->Property)
        {
        this->PropertyModified(iter->Property);
        }
      else if (iter->Proxy)
        {
        this->PropertyModified(iter->Proxy, iter->PropertyName);
        }
      break;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::RemoveAllLinks()
{
  this->Internals->LinkedProperties.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::RemoveLinkedProperty(vtkSMProperty* property)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if (iter->Property == property)
      {
      this->Internals->LinkedProperties.erase(iter);
      this->Modified();
      break;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::RemoveLinkedProperty(vtkSMProxy* proxy,
  const char* pname)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if (iter->Proxy == proxy  && iter->PropertyName == pname)
      {
      this->Internals->LinkedProperties.erase(iter);
      this->Modified();
      break;
      }
    }
}

//-----------------------------------------------------------------------------
unsigned int vtkSMPropertyLink::GetNumberOfLinkedProperties()
{
  return this->Internals->LinkedProperties.size();
}

//-----------------------------------------------------------------------------
vtkSMProperty* vtkSMPropertyLink::GetLinkedProperty(int index)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for(int i=0;
      i<index && iter != this->Internals->LinkedProperties.end();
      i++)
    { 
    iter++;
    }
  if(iter == this->Internals->LinkedProperties.end())
    {
    return NULL;
    }
  return iter->Property;
}

//-----------------------------------------------------------------------------
const char* vtkSMPropertyLink::GetLinkedPropertyName(int index)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for(int i=0;
      i<index && iter != this->Internals->LinkedProperties.end();
      i++)
    { 
    iter++;
    }
  if(iter == this->Internals->LinkedProperties.end())
    {
    return NULL;
    }
  return iter->PropertyName;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPropertyLink::GetLinkedProxy(int index)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for(int i=0;
      i<index && iter != this->Internals->LinkedProperties.end();
      i++)
    { 
    iter++;
    }
  if(iter == this->Internals->LinkedProperties.end())
    {
    return NULL;
    }
  return iter->Proxy;
}

//-----------------------------------------------------------------------------
int vtkSMPropertyLink::GetLinkedPropertyDirection(int index)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for(int i=0;
      i<index && iter != this->Internals->LinkedProperties.end();
      i++)
    { 
    iter++;
    }
  if(iter == this->Internals->LinkedProperties.end())
    {
    return NONE;
    }
  return iter->UpdateDirection;
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
        toProp = iter->Proxy.GetPointer()->GetProperty(iter->PropertyName);
        }
      else if (iter->Property.GetPointer())
        {
        toProp = iter->Property;
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
void vtkSMPropertyLink::PropertyModified(vtkSMProperty* fromProp)
{
  if (this->ModifyingProperty)
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
    if (iter->UpdateDirection & INPUT && iter->Property.GetPointer() == fromProp)
      {
      propagate = 1;
      break;
      }
    }

  if (!propagate)
    {
    return;
    }
 
  this->ModifyingProperty = true;
  // Propagate the changes.
  for (iter = this->Internals->LinkedProperties.begin(); 
    iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if (iter->UpdateDirection & OUTPUT)
      {
      vtkSMProperty* toProp = 0;
      if (iter->Proxy.GetPointer())
        {
        toProp = iter->Proxy.GetPointer()->GetProperty(iter->PropertyName);
        }
      else if (iter->Property.GetPointer())
        {
        toProp = iter->Property;
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
  for(; iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if ((iter->Proxy.GetPointer() == caller) && 
      (iter->PropertyName == pname) && (iter->UpdateDirection & INPUT))
      {
      is_input = true;
      break;
      }
    }

  // Update all output properties.
  if (is_input)
    {
    iter = this->Internals->LinkedProperties.begin();
    for(; iter != this->Internals->LinkedProperties.end(); ++iter)
      {
      if ((iter->Proxy.GetPointer() != caller) && 
        (iter->UpdateDirection & OUTPUT))
        {
        iter->Proxy.GetPointer()->UpdateProperty(
          iter->PropertyName.c_str());
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
    child->AddAttribute("id",
                        static_cast<unsigned int>(iter->Proxy.GetPointer()->GetGlobalID()));
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
  vtkSMProxyLocator* locator)
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
