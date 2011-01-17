/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoStackBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUndoStackBuilder.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMMessage.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyUndoElement.h"
#include "vtkSMRemoteObjectUpdateUndoElement.h"
#include "vtkSMSession.h"
#include "vtkSMUndoStack.h"
#include "vtkUndoElement.h"
#include "vtkUndoSet.h"
#include "vtkUndoStackInternal.h"

#include <vtksys/RegularExpression.hxx>
#include <vtkstd/map>


class vtkSMUndoStackBuilder::vtkInternals
{
public:
  // Return previous message and replace it with the new one.
  // If the previous message does not exist it has no global id
  vtkSMMessage ReplaceState(vtkTypeUInt32 globalId, const vtkSMMessage* state)
    {
    vtkSMMessage result;

    // Look if we have it
    vtkstd::map<vtkTypeUInt32, vtkSMMessage>::iterator iter;
    iter = this->StateMap.find(globalId);

    // We do find a previous state
    if(iter != this->StateMap.end())
      {
      result.CopyFrom(iter->second);
      if(!state)
        {
        // It means the object should be deleted
        this->StateMap.erase(iter);
        }
      }
    if(state)
      {
      this->StateMap[globalId].CopyFrom(*state);
      }
    return result;
    }

private:
  vtkstd::map<vtkTypeUInt32, vtkSMMessage> StateMap;
};
//*****************************************************************************
vtkStandardNewMacro(vtkSMUndoStackBuilder);
vtkCxxSetObjectMacro(vtkSMUndoStackBuilder, UndoStack, vtkSMUndoStack);
//-----------------------------------------------------------------------------
vtkSMUndoStackBuilder::vtkSMUndoStackBuilder()
{
  this->Internals = new vtkInternals();
  this->UndoStack = 0;
  this->UndoSet = vtkUndoSet::New();
  this->Label = NULL;
  this->EnableMonitoring = 0;
  this->IgnoreAllChanges = false;
}

//-----------------------------------------------------------------------------
vtkSMUndoStackBuilder::~vtkSMUndoStackBuilder()
{
  if (this->UndoSet)
    {
    this->UndoSet->Delete();
    this->UndoSet = NULL;
    }
  this->SetLabel(NULL);
  this->SetUndoStack(0);
  delete this->Internals;
}
//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::Begin(const char* label)
{
  if (!this->Label)
    {
    this->SetLabel(label);
    }

  ++this->EnableMonitoring;
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::End()
{
  if (this->EnableMonitoring == 0)
    {
    vtkWarningMacro("Unmatched End().");
    return;
    }
  this->EnableMonitoring--;

}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::PushToStack()
{
  cout << "Push stack " << this->UndoSet->GetNumberOfElements() << endl;
  if (this->UndoSet->GetNumberOfElements() > 0 && this->UndoStack)
    {
    this->UndoStack->Push( (this->Label? this->Label : "Changes"),
                           this->UndoSet);
    }
  this->InitializeUndoSet();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::Clear()
{
  this->InitializeUndoSet();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::InitializeUndoSet()
{
  this->SetLabel(NULL);
  this->UndoSet->RemoveAllElements();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::Add(vtkUndoElement* element)
{
  if (!element)
    {
    return;
    }

  this->UndoSet->AddElement(element);
}
//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnNewState( vtkSMSession* session,
                                        vtkTypeUInt32 globalId,
                                        const vtkSMMessage* newState)
{
  if (this->IgnoreAllChanges || !this->HandleChangeEvents() || !this->UndoStack)
    {
    return;
    }


  vtkSMMessage oldState = this->Internals->ReplaceState(globalId, newState);

  if(newState == NULL || !oldState.has_global_id())
    {
    // Create / Delete
    if(newState)
      {
      cout << "OnNewState CREATE " << globalId << endl;
      // Create
      // We only know how to deal with proxy
      if( newState->HasExtension(ProxyState::xml_group) )
        {
        vtkSMProxyUndoElement* undoElement = vtkSMProxyUndoElement::New();
        undoElement->SetSession(session);
        undoElement->SetCreateElement( true );
        undoElement->SetCreationState( newState );
        this->Add(undoElement);
        undoElement->Delete();
        }
      else if(newState->global_id() == 1)
        {
        // ServerManager state, just update, never create !!!
        vtkSMMessage origin;
        origin.CopyFrom(*newState);
        origin.ClearExtension(ProxyManagerState::registered_proxy);
        vtkSMRemoteObjectUpdateUndoElement* undoElement;
        undoElement = vtkSMRemoteObjectUpdateUndoElement::New();
        undoElement->SetSession(session);
        undoElement->SetUndoRedoState( &origin, newState);
        this->Add(undoElement);
        undoElement->Delete();
        }
      else
        {
        vtkWarningMacro("Try to register Creation Undo event based on a non Proxy object.\n"
                        << newState->DebugString());
        }
      }
    else
      {
      // Delete
      cout << "OnNewState DELETE " << globalId << endl;
      vtkSMProxyUndoElement* undoElement = vtkSMProxyUndoElement::New();
      undoElement->SetSession(session);
      undoElement->SetCreateElement( false );
      undoElement->SetCreationState( &oldState);
      this->Add(undoElement);
      undoElement->Delete();
      }
    }
  else
    {
    cout << "OnNewState UPDATE " << globalId << endl;
    // Just update
    vtkSMRemoteObjectUpdateUndoElement* undoElement;
    undoElement = vtkSMRemoteObjectUpdateUndoElement::New();
    undoElement->SetSession(session);
    undoElement->SetUndoRedoState( &oldState, newState);
    this->Add(undoElement);
    undoElement->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IgnoreAllChanges: " << this->IgnoreAllChanges << endl;
  os << indent << "UndoStack: " << this->UndoStack << endl;
}
