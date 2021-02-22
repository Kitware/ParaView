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
vtkCxxSetObjectMacro(vtkSMCompoundProxyDefinitionLoader, RootElement, vtkPVXMLElement);
//---------------------------------------------------------------------------
vtkSMCompoundProxyDefinitionLoader::vtkSMCompoundProxyDefinitionLoader()
{
  this->RootElement = nullptr;
}

//---------------------------------------------------------------------------
vtkSMCompoundProxyDefinitionLoader::~vtkSMCompoundProxyDefinitionLoader()
{
  this->SetRootElement(nullptr);
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMCompoundProxyDefinitionLoader::LocateProxyElement(vtkTypeUInt32 id_)
{
  if (!this->RootElement)
  {
    vtkErrorMacro("No root is defined. Cannot locate proxy element with id " << id_);
    return nullptr;
  }
  vtkIdType id = static_cast<vtkIdType>(id_);

  vtkPVXMLElement* root = this->RootElement;
  unsigned int numElems = root->GetNumberOfNestedElements();
  unsigned int i = 0;
  for (i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    if (currentElement->GetName() && strcmp(currentElement->GetName(), "Proxy") == 0)
    {
      vtkIdType currentId;
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
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMCompoundProxyDefinitionLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
