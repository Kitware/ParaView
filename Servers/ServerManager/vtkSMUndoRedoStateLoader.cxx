/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoRedoStateLoader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUndoRedoStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyRegisterUndoElement.h"
#include "vtkSMProxyUnRegisterUndoElement.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkUndoSet.h"

vtkStandardNewMacro(vtkSMUndoRedoStateLoader);
vtkCxxRevisionMacro(vtkSMUndoRedoStateLoader, "1.1");
//-----------------------------------------------------------------------------
vtkSMUndoRedoStateLoader::vtkSMUndoRedoStateLoader()
{
  this->UndoSet = 0;
}

//-----------------------------------------------------------------------------
vtkSMUndoRedoStateLoader::~vtkSMUndoRedoStateLoader()
{
  this->UndoSet = 0;
}

//-----------------------------------------------------------------------------
vtkUndoSet* vtkSMUndoRedoStateLoader::LoadUndoRedoSet(
  vtkPVXMLElement* rootElement)
{
  if (!rootElement)
    {
    vtkErrorMacro("Cannot load state from (null) root element.");
    return 0;
    }

  if (!rootElement->GetName() || strcmp(rootElement->GetName(), "UndoSet") != 0)
    {
    vtkErrorMacro("Can only load state from root element with tag UndoSet.");
    return 0;
    }

  vtkUndoSet* undoSet = vtkUndoSet::New();
  this->UndoSet = undoSet;
  
  unsigned int numElems = rootElement->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(cc);
    const char* name = currentElement->GetName();
    if (!name)
      {
      continue;
      }
    this->HandleTag(name, currentElement);
    }
  
  this->UndoSet = 0;
  return undoSet;
}

//-----------------------------------------------------------------------------
void vtkSMUndoRedoStateLoader::HandleTag(const char* tagName, vtkPVXMLElement* root)
{
  if (!this->UndoSet)
    {
    return; //sanity check.
    }
  if (strcmp(tagName, "PropertyModification") == 0)
    {
    this->HandlePropertyModification(root);
    }
  else if (strcmp(tagName, "ProxyRegister") == 0)
    {
    this->HandleProxyRegister(root);
    }
  else if (strcmp(tagName, "ProxyUnRegister") == 0)
    {
    this->HandleProxyUnRegister(root);
    }
}

//-----------------------------------------------------------------------------
void vtkSMUndoRedoStateLoader::HandleProxyRegister(vtkPVXMLElement* root)
{
  vtkSMProxyRegisterUndoElement* elem = vtkSMProxyRegisterUndoElement::New();
  elem->SetConnectionID(this->ConnectionID);
  elem->LoadState(root);
  this->UndoSet->AddElement(elem);
  elem->Delete(); 
}

//-----------------------------------------------------------------------------
void vtkSMUndoRedoStateLoader::HandleProxyUnRegister(vtkPVXMLElement* root)
{
  vtkSMProxyUnRegisterUndoElement* elem = vtkSMProxyUnRegisterUndoElement::New();
  elem->SetConnectionID(this->ConnectionID);
  elem->LoadState(root);
  this->UndoSet->AddElement(elem);
  elem->Delete(); 
}


//-----------------------------------------------------------------------------
void vtkSMUndoRedoStateLoader::HandlePropertyModification(vtkPVXMLElement* root)
{
  vtkSMPropertyModificationUndoElement* elem = 
    vtkSMPropertyModificationUndoElement::New();
  elem->SetConnectionID(this->ConnectionID);
  elem->LoadState(root);
  this->UndoSet->AddElement(elem);
  elem->Delete(); 
}

//-----------------------------------------------------------------------------
void vtkSMUndoRedoStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
