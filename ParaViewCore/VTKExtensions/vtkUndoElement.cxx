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
void vtkUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mergeable: " << this->Mergeable << endl;
}
