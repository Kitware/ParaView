/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPQStateLoader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPQStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkSMMultiViewRenderModuleProxy.h"
#include "vtkSMProxy.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkSMPQStateLoader);
vtkCxxRevisionMacro(vtkSMPQStateLoader, "1.1");
vtkCxxSetObjectMacro(vtkSMPQStateLoader, MultiViewRenderModuleProxy, 
  vtkSMMultiViewRenderModuleProxy);
//-----------------------------------------------------------------------------
vtkSMPQStateLoader::vtkSMPQStateLoader()
{
  this->MultiViewRenderModuleProxy = 0;
}

//-----------------------------------------------------------------------------
vtkSMPQStateLoader::~vtkSMPQStateLoader()
{
  this->SetMultiViewRenderModuleProxy(0);
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPQStateLoader::NewProxyFromElement(
  vtkPVXMLElement* proxyElement, int id)
{
  vtkSMProxy* proxy = this->GetCreatedProxy(id);
  if (proxy)
    {
    proxy->Register(this);
    return proxy;
    }

  // Check if the proxy requested is a render module.
  const char* xml_name = proxyElement->GetAttribute("type");
  const char* xml_group = proxyElement->GetAttribute("group");
  if (xml_group && xml_name && strcmp(proxyElement->GetName(), "Proxy") == 0 
    && strcmp(xml_group, "rendermodules") == 0)
    {
    if (strcmp(xml_name, "MultiViewRenderModule") == 0)
      {
      if (this->MultiViewRenderModuleProxy)
        {
        return this->MultiViewRenderModuleProxy;
        }
      vtkWarningMacro("MultiViewRenderModuleProxy is not set. "
        "Creating MultiViewRenderModuleProxy from the state.");
      }
    else
      {
      // Create a rendermodule.
      if (this->MultiViewRenderModuleProxy)
        {
        vtkSMProxy* proxy = 
          this->MultiViewRenderModuleProxy->NewRenderModule();
        if (proxy)
          {
          this->AddCreatedProxy(id, proxy);
          if (!this->LoadProxyState(proxyElement, proxy))
            {
            this->RemoveCreatedProxy(id);
            vtkErrorMacro("Failed to load proxy state.");
            proxy->Delete();
            return 0;
            }
          }
        return proxy;
        }
      else
        {
        vtkWarningMacro("MultiViewRenderModuleProxy is not set. "
          "Creating MultiViewRenderModuleProxy from the state.");
        }
      }
    }
  return this->Superclass::NewProxyFromElement(proxyElement, id);
}

//-----------------------------------------------------------------------------
void vtkSMPQStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MultiViewRenderModuleProxy: " 
    << this->MultiViewRenderModuleProxy << endl;
}
