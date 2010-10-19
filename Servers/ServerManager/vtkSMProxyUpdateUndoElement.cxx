/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyUpdateUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyUpdateUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkSMSession.h"
#include "vtkSMMessage.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMProxyUpdateUndoElement);
//-----------------------------------------------------------------------------
vtkSMProxyUpdateUndoElement::vtkSMProxyUpdateUndoElement()
{
}

//-----------------------------------------------------------------------------
vtkSMProxyUpdateUndoElement::~vtkSMProxyUpdateUndoElement()
{
}

//-----------------------------------------------------------------------------
void vtkSMProxyUpdateUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//-----------------------------------------------------------------------------
int vtkSMProxyUpdateUndoElement::Undo()
{
  cout << "undo..." << endl;
  return this->UpdateProxyState(&this->BeforeState);
}

//-----------------------------------------------------------------------------
int vtkSMProxyUpdateUndoElement::Redo()
{
  cout << "redo..." << endl;
  return this->UpdateProxyState(&this->AfterState);
}

//-----------------------------------------------------------------------------
int vtkSMProxyUpdateUndoElement::UpdateProxyState(const vtkSMMessage* state)
{
  cout << "Update proxy state " << state->global_id() << endl;
  if(this->Session && state->has_global_id())
    {
    // Creation or update
    vtkSMProxy* proxy =
        vtkSMProxy::SafeDownCast(
            this->Session->GetRemoteObject(state->global_id()));
    if(proxy)
      {
      // Update
      proxy->LoadState(state);
      return 1; // OK
      }
    }
  return 0; // ERROR
}

//-----------------------------------------------------------------------------
void vtkSMProxyUpdateUndoElement::SetUndoRedoState(const vtkSMMessage* before,
                                             const vtkSMMessage* after)
{
  this->BeforeState.Clear();
  this->AfterState.Clear();
  if(before && after)
    {
    this->BeforeState.CopyFrom(*before);
    this->AfterState.CopyFrom(*after);
    }
  else
    {
    vtkErrorMacro("Invalid SetUndoRedoState. At least one of the provided states is NULL.");
    }
}
