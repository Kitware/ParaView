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
#include "vtkSmartPointer.h"
#include "vtkSMGlobalPropertiesLinkUndoElement.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyRegisterUndoElement.h"
#include "vtkSMProxyStateChangedUndoElement.h"
#include "vtkSMProxyUnRegisterUndoElement.h"
#include "vtkSMUpdateInformationUndoElement.h"
#include "vtkUndoSet.h"

#include <vtkstd/vector>

class vtkSMUndoRedoStateLoaderVector : 
  public vtkstd::vector<vtkSmartPointer<vtkSMUndoElement> >
{
};


vtkStandardNewMacro(vtkSMUndoRedoStateLoader);
vtkCxxRevisionMacro(vtkSMUndoRedoStateLoader, "1.6");
vtkCxxSetObjectMacro(vtkSMUndoRedoStateLoader, RootElement, vtkPVXMLElement);
//-----------------------------------------------------------------------------
vtkSMUndoRedoStateLoader::vtkSMUndoRedoStateLoader()
{
  this->RegisteredElements = new vtkSMUndoRedoStateLoaderVector;
  this->RootElement = 0;
  this->ProxyLocator = 0;

  vtkSMUndoElement* elem = vtkSMProxyRegisterUndoElement::New();
  this->RegisterElement(elem);
  elem->Delete();

  elem = vtkSMProxyUnRegisterUndoElement::New();
  this->RegisterElement(elem);
  elem->Delete();

  elem = vtkSMPropertyModificationUndoElement::New();
  this->RegisterElement(elem);
  elem->Delete();

  elem = vtkSMProxyStateChangedUndoElement::New();
  this->RegisterElement(elem);
  elem->Delete();

  elem = vtkSMUpdateInformationUndoElement::New();
  this->RegisterElement(elem);
  elem->Delete();

  elem = vtkSMGlobalPropertiesLinkUndoElement::New();
  this->RegisterElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
vtkSMUndoRedoStateLoader::~vtkSMUndoRedoStateLoader()
{
  delete this->RegisteredElements;
  this->SetRootElement(0);
}

//-----------------------------------------------------------------------------
unsigned int vtkSMUndoRedoStateLoader::RegisterElement(vtkSMUndoElement* elem)
{
  this->RegisteredElements->push_back(elem);
  return static_cast<unsigned int>(this->RegisteredElements->size()-1);
}

//-----------------------------------------------------------------------------
void vtkSMUndoRedoStateLoader::UnRegisterElement(unsigned int index)
{
  if (index >= this->RegisteredElements->size())
    {
    vtkErrorMacro("Invalid index " << index);
    return;
    }

  vtkSMUndoRedoStateLoaderVector::iterator iter = 
    this->RegisteredElements->begin();

  for (unsigned int cc=0; iter != this->RegisteredElements->end();  iter++, cc++)
    {
    if (cc == index)
      {
      this->RegisteredElements->erase(iter);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
vtkSMUndoElement* vtkSMUndoRedoStateLoader::GetRegisteredElement(unsigned int index)
{
  if (index >= this->RegisteredElements->size())
    {
    vtkErrorMacro("Invalid index " << index);
    return 0;
    }

  return (*this->RegisteredElements)[index];
}

//-----------------------------------------------------------------------------
unsigned int vtkSMUndoRedoStateLoader::GetNumberOfRegisteredElements()
{
  return this->RegisteredElements->size();
}

//-----------------------------------------------------------------------------
vtkUndoSet* vtkSMUndoRedoStateLoader::LoadUndoRedoSet(
  vtkPVXMLElement* rootElement,
  vtkSMProxyLocator* locator)
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

  this->SetRootElement(rootElement);
  this->ProxyLocator = locator;

  vtkUndoSet* undoSet = vtkUndoSet::New();
  unsigned int numElems = rootElement->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(cc);
    const char* name = currentElement->GetName();
    if (!name)
      {
      continue;
      }
    vtkUndoElement* elem = this->HandleTag(currentElement);
    if (elem)
      {
      undoSet->AddElement(elem);
      elem->Delete();
      }
    }
  this->ProxyLocator = 0;
  return undoSet;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMUndoRedoStateLoader::LocateProxyElement(int id)
{
  vtkPVXMLElement* root = this->RootElement;

  // When control reaches here, we are assured that a proxy with the given
  // id doesn't already exist, so we try to locate an element
  // with the state for the proxy and create the new proxy.
  unsigned int numElems = root->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    unsigned int child_numElems = currentElement->GetNumberOfNestedElements();
    for (unsigned int cc=0; cc < child_numElems; cc++)
      {
      vtkPVXMLElement* child = currentElement->GetNestedElement(cc);
      int child_id=0;
      if (child->GetName() && 
        strcmp(child->GetName(), "Proxy") == 0 &&
        child->GetAttribute("group") &&
        child->GetAttribute("type") &&
        child->GetScalarAttribute("id", &child_id))
        {
        if (child_id == id)
          {
          // We found an element that defines this unknown proxy.
          // we create this new proxy and return it.
          return child;
          }
        }
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
vtkUndoElement* vtkSMUndoRedoStateLoader::HandleTag(vtkPVXMLElement* root)
{
  vtkSMUndoRedoStateLoaderVector::reverse_iterator iter = 
    this->RegisteredElements->rbegin();

  for (; iter != this->RegisteredElements->rend();  iter++)
    {
    if ((*iter)->CanLoadState(root))
      {
      vtkSMUndoElement* elem = (*iter)->NewInstance();
      elem->SetProxyLocator(this->ProxyLocator);
      elem->LoadState(root);
      return elem;
      }
    }
  vtkWarningMacro("Cannot handle element : " << root->GetName());
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSMUndoRedoStateLoader::CreatedNewProxy(int id, vtkSMProxy* proxy)
{
  vtkClientServerID csid;
  csid.ID = static_cast<vtkTypeUInt32>(id);
  proxy->SetSelfID(csid);
}

//-----------------------------------------------------------------------------
void vtkSMUndoRedoStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
