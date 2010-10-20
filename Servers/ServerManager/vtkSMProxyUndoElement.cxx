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
#include "vtkCollection.h"

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
  vtkTypeUInt32 globalId = this->State.global_id();
  cout << "Create " << globalId << endl;
  if(this->Session->GetRemoteObject(globalId))
    {
    // A parent proxy already create it, so do nothing
    return 1;
    }

  //this->State.PrintDebugString();
  vtkSMProxy *proxy = this->Session->GetProxyManager()->NewProxy(&this->State);
  this->UndoSetWorkingContext->AddItem(proxy);
  cout << " - before delete " << globalId <<" in remote obj ? " << (this->Session->GetRemoteObject(globalId)!=0) << endl;
  proxy->Delete();
  cout << " - after delete " << globalId <<" in remote obj ? " << (this->Session->GetRemoteObject(globalId)!=0) << endl;
  return proxy ? 1 : 0;
}
//-----------------------------------------------------------------------------
int vtkSMProxyUndoElement::DeleteProxy()
{
  cout << "Delete " << this->State.global_id() << endl;
  this->Session->DeletePMObject(&this->State);
  vtkSMRemoteObject *obj = this->Session->GetRemoteObject(this->State.global_id());
  if(obj)
    {
    cout << "ERROR should have deleted Remote object as well "
         << this->State.global_id() <<  " ref count: " << obj->GetReferenceCount()
         << endl;
    }
  return 1; // OK
}
