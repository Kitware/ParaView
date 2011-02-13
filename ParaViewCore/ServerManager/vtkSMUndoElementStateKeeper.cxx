/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoElementStateKeeper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUndoElementStateKeeper.h"

#include "vtkObjectFactory.h"
#include "vtkSMMessage.h"

vtkStandardNewMacro(vtkSMUndoElementStateKeeper);
//-----------------------------------------------------------------------------
vtkSMUndoElementStateKeeper::vtkSMUndoElementStateKeeper()
{
  this->CreationState = new vtkSMMessage();
}

//-----------------------------------------------------------------------------
vtkSMUndoElementStateKeeper::~vtkSMUndoElementStateKeeper()
{
  delete this->CreationState;
}

//-----------------------------------------------------------------------------
void vtkSMUndoElementStateKeeper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMUndoElementStateKeeper::KeepCreationState(const vtkSMMessage* state)
{
  this->CreationState->CopyFrom(*state);
}
//----------------------------------------------------------------------------
vtkSMMessage* vtkSMUndoElementStateKeeper::GetCreationState() const
{
  return this->CreationState;
}
