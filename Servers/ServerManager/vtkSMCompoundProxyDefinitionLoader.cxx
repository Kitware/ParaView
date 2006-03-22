/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompoundProxyDefinitionLoader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompoundProxyDefinitionLoader.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMCompoundProxyDefinitionLoader);
vtkCxxRevisionMacro(vtkSMCompoundProxyDefinitionLoader, "1.2");

//---------------------------------------------------------------------------
vtkSMCompoundProxyDefinitionLoader::vtkSMCompoundProxyDefinitionLoader()
{
}

//---------------------------------------------------------------------------
vtkSMCompoundProxyDefinitionLoader::~vtkSMCompoundProxyDefinitionLoader()
{
}

//---------------------------------------------------------------------------
vtkSMCompoundProxy* vtkSMCompoundProxyDefinitionLoader::HandleDefinition(
  vtkPVXMLElement* rootElement)
{
  vtkSMCompoundProxy* result = vtkSMCompoundProxy::New();

  result->LoadState(rootElement, this);

  return result;
}

//---------------------------------------------------------------------------
vtkSMCompoundProxy* vtkSMCompoundProxyDefinitionLoader::LoadDefinition(
  vtkPVXMLElement* rootElement)
{
  vtkSMCompoundProxy* result = 0;
  if (!rootElement)
    {
    vtkErrorMacro("Cannot load state from (null) root element.");
    return result;
    }

  vtkSMProxyManager* pm = this->GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("Cannot load state without a proxy manager.");
    return result;
    }

  this->RootElement = rootElement;
  this->ClearCreatedProxies();

  if (rootElement->GetName() &&
      strcmp(rootElement->GetName(), "CompoundProxy") == 0)
    {
    result = this->HandleDefinition(rootElement);
    }

  this->ClearCreatedProxies();
  this->RootElement = 0;

  return result;
}

//---------------------------------------------------------------------------
void vtkSMCompoundProxyDefinitionLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
