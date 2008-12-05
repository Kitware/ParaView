/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionLink.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelectionLink.h"

#include "vtkObjectFactory.h"
#include "vtkSMPropertyLink.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkSMSelectionLink);
vtkCxxRevisionMacro(vtkSMSelectionLink, "1.3");
//----------------------------------------------------------------------------
vtkSMSelectionLink::vtkSMSelectionLink()
{
  this->PropertyLink = vtkSMPropertyLink::New();
}

//----------------------------------------------------------------------------
vtkSMSelectionLink::~vtkSMSelectionLink()
{
  this->PropertyLink->Delete();
}

//----------------------------------------------------------------------------
void vtkSMSelectionLink::AddSelectionLink(vtkSMProxy* proxy, int updateDir,
  const char* pname)
{
  this->PropertyLink->AddLinkedProperty(proxy, pname, updateDir);
}

//----------------------------------------------------------------------------
void vtkSMSelectionLink::RemoveAllLinks()
{
  this->PropertyLink->RemoveAllLinks();
}

//----------------------------------------------------------------------------
void vtkSMSelectionLink::RemoveSelectionLink(vtkSMProxy* proxy,
  const char* pname)
{
  this->PropertyLink->RemoveLinkedProperty(proxy, pname);
}

//----------------------------------------------------------------------------
void vtkSMSelectionLink::SaveState(const char* linkname, vtkPVXMLElement* parent)
{
  // Simply save state from the internal property link.
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  this->PropertyLink->SaveState(linkname, root);

  vtkPVXMLElement* childRoot = root->GetNestedElement(0);
  childRoot->SetName("SelectionLink");
  parent->AddNestedElement(childRoot);
  root->Delete();
}

//----------------------------------------------------------------------------
int vtkSMSelectionLink::LoadState(vtkPVXMLElement* linkElement,
  vtkSMProxyLocator* locator)
{
  return this->PropertyLink->LoadState(linkElement, locator);
}

//----------------------------------------------------------------------------
void vtkSMSelectionLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


