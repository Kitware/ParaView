/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkSMSession.h"
#include "vtkSMMessage.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMProxyUndoElement);
//-----------------------------------------------------------------------------
vtkSMProxyUndoElement::vtkSMProxyUndoElement()
{
  this->CreateElement = true;
}

//-----------------------------------------------------------------------------
vtkSMProxyUndoElement::~vtkSMProxyUndoElement()
{
}

//-----------------------------------------------------------------------------
void vtkSMProxyUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//-----------------------------------------------------------------------------
int vtkSMProxyUndoElement::Undo()
{
  if(this->CreateElement)
    {
    // undo create => DELETE
    return this->DeleteProxy();
    }
  else
    {
    return this->CreateProxy();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSMProxyUndoElement::Redo()
{
  if(this->CreateElement)
    {
    // redo create => CREATE
    return this->CreateProxy();
    }
  else
    {
    return this->DeleteProxy();
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyUndoElement::SetCreationState(const vtkSMMessage* state)
{
  this->State.Clear();
  if(!state)
    {
    vtkErrorMacro("No state provided in SetCreationState");
    }
  else
    {
    this->State.CopyFrom(*state);
    }
}
//-----------------------------------------------------------------------------
int vtkSMProxyUndoElement::CreateProxy()
{
  cout << "Create " << this->State.global_id() << endl;
  this->Session->GetProxyManager()->NewProxy(&this->State);
  return 1; // OK
}
//-----------------------------------------------------------------------------
int vtkSMProxyUndoElement::DeleteProxy()
{
    cout << "Delete " << this->State.global_id() << endl;
  this->Session->DeletePMObject(&this->State);
  return 1; // OK
}
