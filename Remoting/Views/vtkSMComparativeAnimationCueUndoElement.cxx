/*=========================================================================

  Program:   ParaView
  Module:    vtkSMComparativeAnimationCueUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMComparativeAnimationCueUndoElement.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVComparativeAnimationCue.h"
#include "vtkPVXMLElement.h"
#include "vtkSMComparativeAnimationCueProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

vtkStandardNewMacro(vtkSMComparativeAnimationCueUndoElement);
//-----------------------------------------------------------------------------
vtkSMComparativeAnimationCueUndoElement::vtkSMComparativeAnimationCueUndoElement()
{
  this->ComparativeAnimationCueID = 0;
}

//-----------------------------------------------------------------------------
vtkSMComparativeAnimationCueUndoElement::~vtkSMComparativeAnimationCueUndoElement()
{
}

//-----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkSMComparativeAnimationCueUndoElement::Undo()
{
  if (this->ComparativeAnimationCueID &&
    this->Session->GetRemoteObject(this->ComparativeAnimationCueID) && this->BeforeState &&
    this->BeforeState->GetNestedElement(0))
  {
    vtkSMComparativeAnimationCueProxy* proxy = vtkSMComparativeAnimationCueProxy::SafeDownCast(
      this->Session->GetRemoteObject(this->ComparativeAnimationCueID));
    proxy->GetComparativeAnimationCue()->LoadCommandInfo(this->BeforeState->GetNestedElement(0));
    proxy->InvokeEvent(vtkCommand::ModifiedEvent); // Will update the UI
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSMComparativeAnimationCueUndoElement::Redo()
{
  if (this->ComparativeAnimationCueID && this->AfterState && this->AfterState->GetNestedElement(0))
  {
    // Make sure the proxy exist.
    // In the current undostack vtkSMComparativeAnimationCueUndoElement will
    // always occurs before the actual proxy will get registered therefore
    // when we redo from nothing we have no other choice than recreating that
    // proxy HERE which should ONLY be done inside the ProxyManager at the
    // registration time.
    if (!this->Session->GetRemoteObject(this->ComparativeAnimationCueID))
    {
      vtkSMProxy* proxy =
        this->Session->GetProxyLocator()->LocateProxy(this->ComparativeAnimationCueID);
      this->UndoSetWorkingContext->AddItem(proxy);
      proxy->LoadXMLState(this->AfterState->GetNestedElement(0), NULL);
      proxy->Delete();
    }
    else
    {
      vtkSMComparativeAnimationCueProxy* proxy = vtkSMComparativeAnimationCueProxy::SafeDownCast(
        this->Session->GetRemoteObject(this->ComparativeAnimationCueID));
      proxy->GetComparativeAnimationCue()->LoadCommandInfo(this->AfterState->GetNestedElement(0));
      proxy->InvokeEvent(vtkCommand::ModifiedEvent); // Will update the UI
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueUndoElement::SetXMLStates(
  vtkTypeUInt32 proxyID, vtkPVXMLElement* before, vtkPVXMLElement* after)
{
  this->ComparativeAnimationCueID = proxyID;
  if (before)
  {
    this->BeforeState = vtkSmartPointer<vtkPVXMLElement>::New();
    before->CopyTo(this->BeforeState);
  }
  else
  {
    this->BeforeState = NULL;
  }
  if (after)
  {
    this->AfterState = vtkSmartPointer<vtkPVXMLElement>::New();
    after->CopyTo(this->AfterState);
  }
  else
  {
    this->AfterState = NULL;
  }
}
