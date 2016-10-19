/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInsituStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkSMInsituStateLoader);
//----------------------------------------------------------------------------
vtkSMInsituStateLoader::vtkSMInsituStateLoader()
{
}

//----------------------------------------------------------------------------
vtkSMInsituStateLoader::~vtkSMInsituStateLoader()
{
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMInsituStateLoader::NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator)
{
  vtkPVXMLElement* elem = this->LocateProxyElement(id);
  if (elem)
  {
    vtkSMProxy* proxy = this->LocateExistingProxyUsingRegistrationName(id);
    if (proxy)
    {
      proxy->Register(this);
      if (!this->LoadProxyState(elem, proxy, locator))
      {
        vtkErrorMacro("Failed to load state correctly.");
        proxy->Delete();
        return 0;
      }
      this->CreatedNewProxy(id, proxy);
      return proxy;
    }
  }

  return this->Superclass::NewProxy(id, locator);
}

//----------------------------------------------------------------------------
void vtkSMInsituStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
