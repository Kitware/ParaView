// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMInsituStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkSMInsituStateLoader);
//----------------------------------------------------------------------------
vtkSMInsituStateLoader::vtkSMInsituStateLoader() = default;

//----------------------------------------------------------------------------
vtkSMInsituStateLoader::~vtkSMInsituStateLoader() = default;

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
        return nullptr;
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
