/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDeserializerXML.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDeserializerXML.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkSMDeserializerXML);
//----------------------------------------------------------------------------
vtkSMDeserializerXML::vtkSMDeserializerXML() = default;

//----------------------------------------------------------------------------
vtkSMDeserializerXML::~vtkSMDeserializerXML() = default;

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMDeserializerXML::NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator)
{
  vtkPVXMLElement* elem = this->LocateProxyElement(id);
  if (!elem)
  {
    return nullptr;
  }

  const char* group = elem->GetAttribute("group");
  const char* type = elem->GetAttribute("type");
  if (!type)
  {
    vtkErrorMacro("Could not create proxy from element, missing 'type'.");
    return nullptr;
  }

  vtkSMProxy* proxy;
  proxy = this->CreateProxy(group, type);
  if (!proxy)
  {
    vtkErrorMacro(
      "Could not create a proxy of group: " << (group ? group : "(null)") << " type: " << type);
    return nullptr;
  }

  if (!this->LoadProxyState(elem, proxy, locator))
  {
    vtkErrorMacro("Failed to load state correctly.");
    proxy->Delete();
    return nullptr;
  }

  this->CreatedNewProxy(id, proxy);
  return proxy;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMDeserializerXML::CreateProxy(
  const char* xmlgroup, const char* xmlname, const char* subProxyName /*=nullptr*/)
{
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  vtkSMProxy* proxy = pxm->NewProxy(xmlgroup, xmlname, subProxyName);
  return proxy;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMDeserializerXML::LocateProxyElement(vtkTypeUInt32 vtkNotUsed(id))
{
  return nullptr;
}

//----------------------------------------------------------------------------
int vtkSMDeserializerXML::LoadProxyState(
  vtkPVXMLElement* element, vtkSMProxy* proxy, vtkSMProxyLocator* locator)
{
  return proxy->LoadXMLState(element, locator);
}

//----------------------------------------------------------------------------
void vtkSMDeserializerXML::CreatedNewProxy(
  vtkTypeUInt32 vtkNotUsed(id), vtkSMProxy* vtkNotUsed(proxy))
{
}

//----------------------------------------------------------------------------
void vtkSMDeserializerXML::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
