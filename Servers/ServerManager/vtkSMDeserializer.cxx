/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDeserializer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDeserializer.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMDeserializer);
//----------------------------------------------------------------------------
vtkSMDeserializer::vtkSMDeserializer()
{
}

//----------------------------------------------------------------------------
vtkSMDeserializer::~vtkSMDeserializer()
{
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMDeserializer::NewProxy(int id, vtkSMProxyLocator* locator)
{
  vtkPVXMLElement* elem = this->LocateProxyElement(id);
  if (!elem)
    {
    return 0;
    }

  const char* group = elem->GetAttribute("group");
  const char* type = elem->GetAttribute("type");
  if (!type)
    {
    vtkErrorMacro("Could not create proxy from element, missing 'type'.");
    return 0;
    }

  vtkSMProxy* proxy;
  proxy = this->CreateProxy(group, type, locator->GetConnectionID());
  if (!proxy)
    {
    vtkErrorMacro("Could not create a proxy of group: "
      << (group? group : "(null)")
      << " type: " << type);
    return 0;
    }

  if (!this->LoadProxyState(elem, proxy, locator))
    {
    vtkErrorMacro("Failed to load state correctly.");
    proxy->Delete();
    return 0;
    }

  this->CreatedNewProxy(id, proxy);
  return proxy;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMDeserializer::CreateProxy(
  const char* xmlgroup, const char* xmlname, vtkIdType connectionId)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* proxy = pxm->NewProxy(xmlgroup, xmlname);
  if (proxy)
    {
    proxy->SetConnectionID(connectionId);
    }
  return proxy;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMDeserializer::LocateProxyElement(int vtkNotUsed(id))
{
  return NULL;
}

//----------------------------------------------------------------------------
int vtkSMDeserializer::LoadProxyState(
  vtkPVXMLElement* element, vtkSMProxy* proxy,
  vtkSMProxyLocator* locator)
{
  return proxy->LoadState(element, locator);
}


//----------------------------------------------------------------------------
void vtkSMDeserializer::CreatedNewProxy(int vtkNotUsed(id),
  vtkSMProxy* vtkNotUsed(proxy))
{
}

//----------------------------------------------------------------------------
void vtkSMDeserializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


