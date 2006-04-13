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

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMProxyRegisterUndoElement);
vtkCxxRevisionMacro(vtkSMProxyRegisterUndoElement, "1.1");
vtkCxxSetObjectMacro(vtkSMProxyRegisterUndoElement, XMLElement,
  vtkPVXMLElement);
//-----------------------------------------------------------------------------
vtkSMProxyRegisterUndoElement::vtkSMProxyRegisterUndoElement()
{
  this->XMLElement = 0;
}

//-----------------------------------------------------------------------------
vtkSMProxyRegisterUndoElement::~vtkSMProxyRegisterUndoElement()
{
  this->SetXMLElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMProxyRegisterUndoElement::ProxyToRegister(const char* groupname, 
  const char* proxyname,  vtkSMProxy* proxy)
{
  vtkPVXMLElement* pcElement = vtkPVXMLElement::New();
  pcElement->SetName("ProxyRegister");
  pcElement->AddAttribute("xml_group_name", proxy->GetXMLGroup());
  pcElement->AddAttribute("xml_proxy_name", proxy->GetXMLName());
  pcElement->AddAttribute("group_name", groupname);
  pcElement->AddAttribute("proxy_name", proxyname);
  pcElement->AddAttribute("id", proxy->GetSelfIDAsString());
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
  
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  pxm->UnRegisterProxy(group_name, proxy_name);

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
  vtkPVXMLElement* element = this->XMLElement;
  const char* group_name = element->GetAttribute("group_name");
  const char* proxy_name = element->GetAttribute("proxy_name");
  const char* xml_group_name = element->GetAttribute("xml_group_name");
  const char* xml_proxy_name = element->GetAttribute("xml_proxy_name");
 
  int int_id = 0;
  element->GetScalarAttribute("id", &int_id);
  vtkClientServerID proxyID;
  proxyID.ID = static_cast<vtkTypeUInt32>(int_id);

  if (!proxyID.ID)
    {
    vtkErrorMacro("No valid proxy ID present.");
    return 0;
    }
  
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(pm->GetObjectFromID(proxyID));

  int created = 0;
  if (!proxy)
    {
    // Create a new proxy.
    proxy = pxm->NewProxy(xml_group_name, xml_proxy_name);
    if (!proxy)
      {
      vtkErrorMacro("Failed to create proxy: " 
        << (xml_group_name? xml_group_name: "(none)") << ", "
        << (xml_proxy_name? xml_proxy_name: "(none)"));
      return 0;
      }
    created = 1;
    }
  pxm->RegisterProxy(group_name, proxy_name, proxy);
  if (created)
    {
    proxy->Delete();
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyRegisterUndoElement::SaveStateInternal(vtkPVXMLElement* root)
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No state present to save.");
    }
  root->AddNestedElement(this->XMLElement);
}

//-----------------------------------------------------------------------------
void vtkSMProxyRegisterUndoElement::LoadStateInternal(vtkPVXMLElement* element)
{
  if (strcmp(element->GetName(), "ProxyRegister") != 0)
    {
    vtkErrorMacro("Invalid element name: " 
      << element->GetName() << ". Must be ProxyRegister.");
    return;
    }

  this->SetXMLElement(element);
}

//-----------------------------------------------------------------------------
void vtkSMProxyRegisterUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

