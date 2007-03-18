/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVXMLElement.h"
#include "vtkSMStateLoader.h"

vtkCxxRevisionMacro(vtkSMUndoElement, "1.3");
vtkCxxSetObjectMacro(vtkSMUndoElement, XMLElement, vtkPVXMLElement);
vtkCxxSetObjectMacro(vtkSMUndoElement, StateLoader, vtkSMStateLoader);
//-----------------------------------------------------------------------------
vtkSMUndoElement::vtkSMUndoElement()
{
  this->ConnectionID  = 
    vtkProcessModuleConnectionManager::GetNullConnectionID();
  this->XMLElement = 0;
  this->StateLoader = 0;
}

//-----------------------------------------------------------------------------
vtkSMUndoElement::~vtkSMUndoElement()
{
  this->SetXMLElement(0);
  this->SetStateLoader(0);
}

//-----------------------------------------------------------------------------
void vtkSMUndoElement::SaveStateInternal(vtkPVXMLElement* root)
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No state present to save.");
    }
  root->AddNestedElement(this->XMLElement);
}

//-----------------------------------------------------------------------------
void vtkSMUndoElement::LoadStateInternal(vtkPVXMLElement* element)
{
  this->SetXMLElement(element);
}

//-----------------------------------------------------------------------------
void vtkSMUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConnectionID: " << this->ConnectionID << endl;
  os << indent << "StateLoader: " << this->StateLoader << endl;
}

