/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRemoteObjectUpdateUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRemoteObjectUpdateUndoElement.h"

#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkSMMessage.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRemoteObject.h"
#include "vtkSMSession.h"
#include "vtkSMStateLocator.h"

#include <vtkNew.h>

vtkStandardNewMacro(vtkSMRemoteObjectUpdateUndoElement);
vtkSetObjectImplementationMacro(
  vtkSMRemoteObjectUpdateUndoElement, ProxyLocator, vtkSMProxyLocator);
//-----------------------------------------------------------------------------
vtkSMRemoteObjectUpdateUndoElement::vtkSMRemoteObjectUpdateUndoElement()
{
  this->ProxyLocator = nullptr;
  this->AfterState = new vtkSMMessage();
  this->BeforeState = new vtkSMMessage();
}

//-----------------------------------------------------------------------------
vtkSMRemoteObjectUpdateUndoElement::~vtkSMRemoteObjectUpdateUndoElement()
{
  delete this->AfterState;
  delete this->BeforeState;
  this->AfterState = nullptr;
  this->BeforeState = nullptr;

  this->SetProxyLocator(nullptr);
}

//-----------------------------------------------------------------------------
void vtkSMRemoteObjectUpdateUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GlobalId: " << this->GetGlobalId() << endl;
  os << indent << "Before state: " << endl;
  if (this->BeforeState)
    this->BeforeState->PrintDebugString();
  os << indent << "After state: " << endl;
  if (this->AfterState)
    this->AfterState->PrintDebugString();
}
//-----------------------------------------------------------------------------
int vtkSMRemoteObjectUpdateUndoElement::Undo()
{
  return this->UpdateState(this->BeforeState);
}

//-----------------------------------------------------------------------------
int vtkSMRemoteObjectUpdateUndoElement::Redo()
{
  return this->UpdateState(this->AfterState);
}

//-----------------------------------------------------------------------------
int vtkSMRemoteObjectUpdateUndoElement::UpdateState(const vtkSMMessage* state)
{
  if (this->Session && state && state->has_global_id())
  {
    // Creation or update
    vtkSMRemoteObject* remoteObj =
      vtkSMRemoteObject::SafeDownCast(this->Session->GetRemoteObject(state->global_id()));

    if (remoteObj)
    {
      // This prevent in-between object to be accenditaly removed
      this->Session->GetAllRemoteObjects(this->UndoSetWorkingContext);

      // Update
      if (this->ProxyLocator)
      {
        this->ProxyLocator->SetSession(this->Session);
        remoteObj->LoadState(state, this->ProxyLocator);
      }
      else
      {
        remoteObj->LoadState(state, this->Session->GetProxyLocator());
      }
    }
  }
  return 1; // OK, we say that everything is fine.
}

//-----------------------------------------------------------------------------
void vtkSMRemoteObjectUpdateUndoElement::SetUndoRedoState(
  const vtkSMMessage* before, const vtkSMMessage* after)
{
  this->BeforeState->Clear();
  this->AfterState->Clear();
  if (before && after)
  {
    this->BeforeState->CopyFrom(*before);
    this->AfterState->CopyFrom(*after);
  }
  else
  {
    vtkErrorMacro("Invalid SetUndoRedoState. "
      << "At least one of the provided states is NULL.");
  }
}
//-----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMRemoteObjectUpdateUndoElement::GetGlobalId()
{
  return this->BeforeState->global_id();
}
