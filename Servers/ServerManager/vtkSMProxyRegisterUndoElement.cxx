/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyRegisterUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyRegisterUndoElement.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMProxyRegisterUndoElement);
//-----------------------------------------------------------------------------
vtkSMProxyRegisterUndoElement::vtkSMProxyRegisterUndoElement()
{
}

//-----------------------------------------------------------------------------
vtkSMProxyRegisterUndoElement::~vtkSMProxyRegisterUndoElement()
{
}

//-----------------------------------------------------------------------------
bool vtkSMProxyRegisterUndoElement::CanLoadState(vtkPVXMLElement* elem)
{
  return (elem && elem->GetName() && 
    strcmp(elem->GetName(), "ProxyRegister") == 0);
}

//-----------------------------------------------------------------------------
void vtkSMProxyRegisterUndoElement::ProxyToRegister(const char* groupname, 
  const char* proxyname,  vtkSMProxy* proxy)
{
  vtkPVXMLElement* pcElement = vtkPVXMLElement::New();
  pcElement->SetName("ProxyRegister");
 
  // This will save the state for the proxy properties as well, which is unncessary.
  // We only need the proxy state header. In future we may overload 
  // vtkSMProxy::SaveState (or something) to not save the property states.
  proxy->SaveState(pcElement);

  pcElement->AddAttribute("group_name", groupname);
  pcElement->AddAttribute("proxy_name", proxyname);
  pcElement->AddAttribute("id", proxy->GetSelfIDAsString());
    //ID is also saved as part of proxy->SaveState(), but we save it here to make
    //it easily accessible.
  
  this->SetXMLElement(pcElement);
  pcElement->Delete();
}

//-----------------------------------------------------------------------------
int vtkSMProxyRegisterUndoElement::Undo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No State present to undo.");
    return 0;
    }
  
  vtkPVXMLElement* element = this->XMLElement;
  const char* group_name = element->GetAttribute("group_name");
  const char* proxy_name = element->GetAttribute("proxy_name");
  int id = 0;
  element->GetScalarAttribute("id",&id);
  if (!id)
    {
    vtkErrorMacro("Failed to locate proxy id.");
    return 0;
    }

  vtkSMProxyLocator* locator = this->GetProxyLocator();
  if (!locator)
    {
    vtkErrorMacro("No locator set. Cannot Undo.");
    return 0;
    }

  vtkSMProxy* proxy = locator->LocateProxy(id);
  if (!proxy)
    {
    vtkErrorMacro("Failed to locate the proxy to register.");
    return 0;
    }
  
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  pxm->UnRegisterProxy(group_name, proxy_name, proxy);

  // Unregistering may trigger deletion of the proxy.
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSMProxyRegisterUndoElement::Redo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No State present to redo.");
    return 0;
    }
  
  if (this->XMLElement->GetNumberOfNestedElements() != 1)
    {
    vtkErrorMacro("Invalid child elements. Cannot redo.");
    return 0;
    }

  vtkPVXMLElement* element = this->XMLElement;
  const char* group_name = element->GetAttribute("group_name");
  const char* proxy_name = element->GetAttribute("proxy_name");
  int id = 0;
  element->GetScalarAttribute("id",&id);
  if (!id)
    {
    vtkErrorMacro("Failed to locate proxy id.");
    return 0;
    }

  vtkSMProxyLocator* locator = this->GetProxyLocator();
  if (!locator)
    {
    vtkErrorMacro("No locator set. Cannot Redo.");
    return 0;
    }

  vtkSMProxy* proxy = locator->LocateProxy(id);

  if (!proxy)
    {
    vtkErrorMacro("Failed to locate the proxy to register.");
    return 0;
    }
  // ensure that the proxy is alive and well before registering it.
  proxy->UpdateVTKObjects();

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  pxm->RegisterProxy(group_name, proxy_name, proxy);
  // We note that the proxy is registered after its state has been 
  // loaded as a result when the vtkSMUndoStack updates the modified proxies,
  // this proxy is not going to be updated. Hence we explicitly mark it 
  // for update. 
  proxy->InvokeEvent(vtkCommand::PropertyModifiedEvent, 0);
  return 1;
}


//-----------------------------------------------------------------------------
void vtkSMProxyRegisterUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

