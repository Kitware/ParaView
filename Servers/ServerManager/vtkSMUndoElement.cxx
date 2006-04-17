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

vtkCxxRevisionMacro(vtkSMUndoElement, "1.2");
//-----------------------------------------------------------------------------
vtkSMUndoElement::vtkSMUndoElement()
{
  this->ConnectionID  = 
    vtkProcessModuleConnectionManager::GetNullConnectionID();
}

//-----------------------------------------------------------------------------
vtkSMUndoElement::~vtkSMUndoElement()
{

}

//-----------------------------------------------------------------------------
void vtkSMUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConnectionID: " << this->ConnectionID << endl;
}

