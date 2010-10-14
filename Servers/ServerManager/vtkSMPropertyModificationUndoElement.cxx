/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyModificationUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPropertyModificationUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"

vtkStandardNewMacro(vtkSMPropertyModificationUndoElement);
//-----------------------------------------------------------------------------
vtkSMPropertyModificationUndoElement::vtkSMPropertyModificationUndoElement()
{
  this->SetMergeable(true);

}

//-----------------------------------------------------------------------------
vtkSMPropertyModificationUndoElement::~vtkSMPropertyModificationUndoElement()
{
}

//-----------------------------------------------------------------------------
int vtkSMPropertyModificationUndoElement::Undo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No State present to undo.");
    return 0;
    }
  int proxy_id;
  this->XMLElement->GetScalarAttribute("id", &proxy_id);
  const char* property_name = this->XMLElement->GetAttribute("name");

  vtkSMProxyLocator* locator = this->GetProxyLocator();
  if (!locator)
    {
    vtkErrorMacro("No locator set. Cannot Undo.");
    return 0;
    }
  vtkSMProxy* proxy = locator->LocateProxy(proxy_id);
  vtkSMProperty* property = (proxy? proxy->GetProperty(property_name): NULL);
  int ret = 0;
  if (property)
    {
    ret = property->LoadState(this->XMLElement->GetNestedElement(0),  
      locator, 1);
    proxy->UpdateProperty(property_name);
    }
  return ret;
}

//-----------------------------------------------------------------------------
int vtkSMPropertyModificationUndoElement::Redo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No State present to redo.");
    return 0;
    }
  int proxy_id;
  this->XMLElement->GetScalarAttribute("id", &proxy_id);
  const char* property_name = this->XMLElement->GetAttribute("name");

  vtkSMProxyLocator* locator = this->GetProxyLocator();
  if (!locator)
    {
    vtkErrorMacro("No locator set. Cannot Redo.");
    return 0;
    }
  vtkSMProxy* proxy = locator->LocateProxy(proxy_id);
  vtkSMProperty* property = (proxy? proxy->GetProperty(property_name): NULL);
  int ret = 0;
  if (property)
    {
    ret = property->LoadState(this->XMLElement->GetNestedElement(0),  
      locator, 0);
    proxy->UpdateProperty(property_name);
    }
  return ret;
}

//-----------------------------------------------------------------------------
bool vtkSMPropertyModificationUndoElement::CanLoadState(vtkPVXMLElement* elem)
{
  return (elem && elem->GetName() && 
    strcmp(elem->GetName(), "PropertyModification") == 0);
}

//-----------------------------------------------------------------------------
void vtkSMPropertyModificationUndoElement::ModifiedProperty(vtkSMProxy* proxy, 
  const char* propertyname)
{
  vtkSMProperty* property = proxy->GetProperty(propertyname);
  if (!property)
    {
    vtkErrorMacro("Failed to locate property with name : " << propertyname
      << " on the proxy. Cannot note its modification state for undo/redo.");
    return;
    }
  
  vtkPVXMLElement* pmElement = vtkPVXMLElement::New();
  pmElement->SetName("PropertyModification");
  pmElement->AddAttribute("id", proxy->GetSelfIDAsString());
  pmElement->AddAttribute("name", propertyname);

  property->SaveState(pmElement, propertyname, proxy->GetSelfIDAsString(),
    /*saveDomains=*/0, /*saveLastPushedValues=*/1);

  this->SetXMLElement(pmElement);
  pmElement->Delete();
}

//-----------------------------------------------------------------------------
bool vtkSMPropertyModificationUndoElement::Merge(vtkUndoElement* 
  vtkNotUsed(new_element))
{
  return false;

  // FIXME this doesn't work correctly...need to check.
#if 0
  vtkSMPropertyModificationUndoElement* toMerge = 
    vtkSMPropertyModificationUndoElement::SafeDownCast(new_element);
  if (!toMerge || !toMerge->XMLElement)
    {
    return false;
    }

  int proxy_id=0;
  this->XMLElement->GetScalarAttribute("id", &proxy_id);
  const char* property_name = this->XMLElement->GetAttribute("name");

  int other_id=0;
  toMerge->XMLElement->GetScalarAttribute("id", &other_id);
  const char* other_name = toMerge->XMLElement->GetAttribute("name");

  if (proxy_id != other_id && strcmp(property_name, other_name) != 0)
    {
    return false;
    }

  // Merge the XMLs.
  vtkPVXMLElement* last_pushed_values = 
    this->XMLElement->GetNestedElement(0)->
    FindNestedElementByName("LastPushedValues");
  vtkPVXMLElement* other_last_pushed_values = toMerge->XMLElement->
    GetNestedElement(0)->FindNestedElementByName("LastPushedValues");
  if (!last_pushed_values || !other_last_pushed_values)
    {
    return false;
    }

  toMerge->XMLElement->GetNestedElement(0)->RemoveNestedElement(
    other_last_pushed_values);
  toMerge->XMLElement->GetNestedElement(0)->AddNestedElement(
    last_pushed_values);

  this->SetXMLElement(toMerge->XMLElement);
  return true;
#endif
}

//-----------------------------------------------------------------------------
void vtkSMPropertyModificationUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
