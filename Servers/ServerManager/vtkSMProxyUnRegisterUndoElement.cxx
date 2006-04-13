/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyUnRegisterUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyUnRegisterUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDefaultStateLoader.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMProxyUnRegisterUndoElement);
vtkCxxRevisionMacro(vtkSMProxyUnRegisterUndoElement, "1.1");
vtkCxxSetObjectMacro(vtkSMProxyUnRegisterUndoElement, XMLElement, vtkPVXMLElement);
//-----------------------------------------------------------------------------
vtkSMProxyUnRegisterUndoElement::vtkSMProxyUnRegisterUndoElement()
{
  this->XMLElement = NULL;
}

//-----------------------------------------------------------------------------
vtkSMProxyUnRegisterUndoElement::~vtkSMProxyUnRegisterUndoElement()
{
  this->SetXMLElement(0);
}

//-----------------------------------------------------------------------------
int vtkSMProxyUnRegisterUndoElement::Undo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No proxy state available to undo deletion.");
    return 0;
    }
 
  vtkPVXMLElement* element = this->XMLElement;
  const char* group_name = element->GetAttribute("group_name");
  const char* proxy_name = element->GetAttribute("proxy_name");
 
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
    vtkSMDefaultStateLoader* loader = vtkSMDefaultStateLoader::New();
    proxy = loader->NewProxyFromElement(this->XMLElement, 0);
    created = 1;
    }
  
  if (!proxy)
    {
    vtkErrorMacro("Failed to locate proxy to register.");
    return 0;
    }
  
  pxm->RegisterProxy(group_name, proxy_name, proxy);
  if (created)
    {
    proxy->Delete();
    }
  return 1;  
}

//-----------------------------------------------------------------------------
int vtkSMProxyUnRegisterUndoElement::Redo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No State present to redo.");
    return 0;
    }
  vtkPVXMLElement* element = this->XMLElement;
  const char* group_name = element->GetAttribute("group_name");
  const char* proxy_name = element->GetAttribute("proxy_name");

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  pxm->UnRegisterProxy(group_name, proxy_name);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyUnRegisterUndoElement::ProxyToUnRegister(const char* groupname,
  const char* proxyname, vtkSMProxy* proxy)
{
  if (!proxy)
    {
    vtkErrorMacro("Proxy cannot be NULL.");
    return;
    }
  
  vtkPVXMLElement* pdElement = vtkPVXMLElement::New();
  pdElement->SetName("ProxyUnRegister");
  pdElement->AddAttribute("group_name", proxyname);
  pdElement->AddAttribute("proxy_name", groupname);
  pdElement->AddAttribute("id", proxy->GetSelfIDAsString());

  vtkPVXMLElement* proxyState = proxy->SaveState(NULL);
  pdElement->AddNestedElement(proxyState);
  proxyState->Delete();
  
  this->SetXMLElement(pdElement);
  pdElement->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMProxyUnRegisterUndoElement::SaveStateInternal(vtkPVXMLElement* root)
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No state present to save.");
    }
  root->AddNestedElement(this->XMLElement);
}

//-----------------------------------------------------------------------------
void vtkSMProxyUnRegisterUndoElement::LoadStateInternal(vtkPVXMLElement* element)
{
  if (strcmp(element->GetName(), "ProxyUnRegister") != 0)
    {
    vtkErrorMacro("Invalid element name: " << element->GetName()
      << ". Must be ProxyUnRegister.");
    return;
    }
  
  this->SetXMLElement(element);
}

//-----------------------------------------------------------------------------
void vtkSMProxyUnRegisterUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
