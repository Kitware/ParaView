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
vtkCxxRevisionMacro(vtkSMCompoundProxyDefinitionLoader, "1.1");

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

  unsigned int numElems = rootElement->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "Proxy") == 0)
      {
      const char* compoundName = currentElement->GetAttribute(
        "compound_name");
      if (compoundName && compoundName[0] != '\0')
        {
        int currentId;
        if (!currentElement->GetScalarAttribute("id", &currentId))
          {
          continue;
          }
        vtkSMProxy* subProxy = this->NewProxyFromElement(
          currentElement, currentId);
        if (subProxy)
          {
          result->AddProxy(compoundName, subProxy);
          subProxy->Delete();
          }
        }
      }
    }

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

  unsigned int numElems = rootElement->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "CompoundProxy") == 0)
      {
      result = this->HandleDefinition(currentElement);
      break;
      }
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
