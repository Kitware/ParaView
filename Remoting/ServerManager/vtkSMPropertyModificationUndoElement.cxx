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
#include "vtkSMMessage.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"

vtkStandardNewMacro(vtkSMPropertyModificationUndoElement);
//-----------------------------------------------------------------------------
vtkSMPropertyModificationUndoElement::vtkSMPropertyModificationUndoElement()
{
  this->SetMergeable(true);
  this->PropertyName = nullptr;
  this->ProxyGlobalID = 0;
  this->PropertyState = new vtkSMMessage();
}

//-----------------------------------------------------------------------------
vtkSMPropertyModificationUndoElement::~vtkSMPropertyModificationUndoElement()
{
  this->SetPropertyName(nullptr);
  delete this->PropertyState;
}

//-----------------------------------------------------------------------------
int vtkSMPropertyModificationUndoElement::Undo()
{
  return this->RevertToState();
}
//-----------------------------------------------------------------------------
int vtkSMPropertyModificationUndoElement::Redo()
{
  return this->RevertToState();
}

//-----------------------------------------------------------------------------
int vtkSMPropertyModificationUndoElement::RevertToState()
{
  if (this->ProxyGlobalID == 0)
  {
    vtkErrorMacro("Invalid State.");
    return 0;
  }
  if (!this->Session)
  {
    vtkErrorMacro("No session set. Cannot Revert to state.");
    return 0;
  }
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(this->Session->GetRemoteObject(this->ProxyGlobalID));
  vtkSMProperty* property = (proxy ? proxy->GetProperty(this->PropertyName) : nullptr);
  if (property)
  {
    property->ReadFrom(this->PropertyState, 0, nullptr); // 0 because only one
    proxy->UpdateProperty(this->PropertyName);
  }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyModificationUndoElement::ModifiedProperty(
  vtkSMProxy* proxy, const char* propertyname)
{
  vtkSMProperty* property = proxy->GetProperty(propertyname);
  if (!property)
  {
    vtkErrorMacro("Failed to locate property with name : "
      << propertyname << " on the proxy. Cannot note its modification state for undo/redo.");
    return;
  }

  this->SetSession(proxy->GetSession());
  this->ProxyGlobalID = proxy->GetGlobalID();
  this->SetPropertyName(propertyname);

  this->PropertyState->Clear();
  property->WriteTo(this->PropertyState);
}

//-----------------------------------------------------------------------------
bool vtkSMPropertyModificationUndoElement::Merge(vtkUndoElement* vtkNotUsed(new_element))
{
  return false;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyModificationUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
