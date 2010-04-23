/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyStateChangedUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyStateChangedUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"


vtkStandardNewMacro(vtkSMProxyStateChangedUndoElement);
//----------------------------------------------------------------------------
vtkSMProxyStateChangedUndoElement::vtkSMProxyStateChangedUndoElement()
{
}

//----------------------------------------------------------------------------
vtkSMProxyStateChangedUndoElement::~vtkSMProxyStateChangedUndoElement()
{
}

//----------------------------------------------------------------------------
bool vtkSMProxyStateChangedUndoElement::CanLoadState(vtkPVXMLElement* elem)
{
  return (elem && elem->GetName() && 
    strcmp(elem->GetName(), "ProxyStateChanged") == 0);
}

//----------------------------------------------------------------------------
void vtkSMProxyStateChangedUndoElement::StateChanged(
  vtkSMProxy* proxy, vtkPVXMLElement* elem)
{
  vtkPVXMLElement* element = vtkPVXMLElement::New();
  element->SetName("ProxyStateChanged");
  element->AddAttribute("id", proxy->GetSelfIDAsString());
  element->AddNestedElement(elem);
  this->SetXMLElement(element);
  element->Delete();
}

//----------------------------------------------------------------------------
int vtkSMProxyStateChangedUndoElement::UndoRedo(bool redo)
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No state present to undo/redo.");
    return 0;
    }
  vtkSMProxyLocator* locator = this->GetProxyLocator();
  if (!locator)
    {
    vtkErrorMacro("No locator set. Cannot undo/redo.");
    return 0;
    }

  int proxy_id;
  this->XMLElement->GetScalarAttribute("id", &proxy_id);
  vtkSMProxy* proxy = locator->LocateProxy(proxy_id);
  if (!proxy)
    {
    vtkErrorMacro("Failed to locate proxy with id: " << proxy_id);
    return 0;
    }

  vtkPVXMLElement* stateXML = this->XMLElement->GetNestedElement(0);
  return redo?  proxy->LoadState(stateXML, locator) :
    proxy->RevertState(stateXML, locator);
}

//----------------------------------------------------------------------------
void vtkSMProxyStateChangedUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


