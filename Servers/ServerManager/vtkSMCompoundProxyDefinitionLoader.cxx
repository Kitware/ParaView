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
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMCompoundProxyDefinitionLoader);

//---------------------------------------------------------------------------
vtkSMCompoundProxyDefinitionLoader::vtkSMCompoundProxyDefinitionLoader()
{
  this->RootElement = 0;
}

//---------------------------------------------------------------------------
vtkSMCompoundProxyDefinitionLoader::~vtkSMCompoundProxyDefinitionLoader()
{
  this->RootElement = 0;
}

//---------------------------------------------------------------------------
vtkSMCompoundSourceProxy* vtkSMCompoundProxyDefinitionLoader::HandleDefinition(
  vtkPVXMLElement* rootElement)
{
  vtkSMCompoundSourceProxy* result = vtkSMCompoundSourceProxy::New();

  this->RootElement = rootElement;
  vtkSMProxyLocator* locator = vtkSMProxyLocator::New();
  locator->SetDeserializer(this);
  int retVal = result->LoadDefinition(rootElement, locator);
  locator->SetDeserializer(0);
  locator->Delete();
  this->RootElement = 0;

  if (retVal)
    {
    return result;
    }
  result->Delete();
  return 0;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMCompoundProxyDefinitionLoader::LocateProxyElement(int id)
{
  if (!this->RootElement)
    {
    vtkErrorMacro("No root is defined. Cannot locate proxy element with id " 
      << id);
    return 0;
    }

  vtkPVXMLElement* root = this->RootElement;
  unsigned int numElems = root->GetNumberOfNestedElements();
  unsigned int i=0;
  for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    if (currentElement->GetName() &&
      strcmp(currentElement->GetName(), "Proxy") == 0)
      {
      int currentId;
      if (!currentElement->GetScalarAttribute("id", &currentId))
        {
        continue;
        }
      if (id != currentId)
        {
        continue;
        }
      return currentElement;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMCompoundSourceProxy* vtkSMCompoundProxyDefinitionLoader::LoadDefinition(
  vtkPVXMLElement* rootElement)
{
  vtkSMCompoundSourceProxy* result = 0;
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

  if (rootElement->GetName() &&
      strcmp(rootElement->GetName(), "CompoundSourceProxy") == 0)
    {
    result = this->HandleDefinition(rootElement);
    }

  return result;
}

//---------------------------------------------------------------------------
void vtkSMCompoundProxyDefinitionLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
