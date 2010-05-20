/*=========================================================================

  Program:   ParaView
  Module:    vtkUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

//-----------------------------------------------------------------------------
vtkUndoElement::vtkUndoElement()
{
  this->Mergeable = false;
}

//-----------------------------------------------------------------------------
vtkUndoElement::~vtkUndoElement()
{
}

//-----------------------------------------------------------------------------
void vtkUndoElement::SaveState(vtkPVXMLElement* root)
{
  if (!root)
    {
    vtkErrorMacro("Root element must be specified to save the state.");
    return;
    }
  this->SaveStateInternal(root);
}

//-----------------------------------------------------------------------------
void vtkUndoElement::LoadState(vtkPVXMLElement* element)
{
  if (!element)
    {
    vtkErrorMacro("Element must be specified to load the state.");
    return;
    }
  this->LoadStateInternal(element);
}

//-----------------------------------------------------------------------------
void vtkUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mergeable: " << this->Mergeable << endl;
}
