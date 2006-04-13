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

vtkCxxRevisionMacro(vtkSMUndoElement, "1.1");
//-----------------------------------------------------------------------------
vtkSMUndoElement::vtkSMUndoElement()
{
  this->ConnectionID.ID  = 0;
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

