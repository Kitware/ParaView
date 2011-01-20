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

vtkStandardNewMacro(vtkSMUndoStackBuilder);
vtkCxxSetObjectMacro(vtkSMUndoStackBuilder, UndoStack, vtkSMUndoStack);
//-----------------------------------------------------------------------------
vtkSMUndoStackBuilder::vtkSMUndoStackBuilder()
{
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
  if(this->EnableMonitoring > 0)
    {
    return; // Only push the whole set when the first begin/end has been reached
    }

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
void vtkSMUndoStackBuilder::OnCreation( vtkSMSession *session,
                                        vtkTypeUInt32 globalId,
                                        const vtkSMMessage *creationState)
{
  if (this->IgnoreAllChanges || !this->HandleChangeEvents() || !this->UndoStack)
    {
    return;
    }

  // No creation event for special remote object (i.e.: PipelineState...)
  if(globalId < 10)
    {
    this->OnUpdate(session, globalId, creationState, creationState);
    }
  else
    {
    if( creationState->HasExtension(ProxyState::xml_group) )
      {
      vtkSMProxyUndoElement* undoElement = vtkSMProxyUndoElement::New();
      undoElement->SetSession(session);
      undoElement->SetCreateElement( true );
      undoElement->SetCreationState( creationState );
      this->Add(undoElement);
      undoElement->Delete();
      }
    else
      {
      vtkWarningMacro("Try to register Creation Undo event based on a non Proxy object.\n"
                      << creationState->DebugString().c_str());
      }
    }
}
//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnUpdate( vtkSMSession *session,
                                      vtkTypeUInt32 globalId,
                                      const vtkSMMessage *previousState,
                                      const vtkSMMessage *newState)
{
  if (this->IgnoreAllChanges || !this->HandleChangeEvents() || !this->UndoStack)
    {
    return;
    }

  vtkSMRemoteObjectUpdateUndoElement* undoElement;
  undoElement = vtkSMRemoteObjectUpdateUndoElement::New();
  undoElement->SetSession(session);
  undoElement->SetUndoRedoState( previousState, newState );
  this->Add(undoElement);
  undoElement->Delete();
}
//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::OnDeletion( vtkSMSession *session,
                                        vtkTypeUInt32 globalId,
                                        const vtkSMMessage *previousState)
{
  if (this->IgnoreAllChanges || !this->HandleChangeEvents() || !this->UndoStack)
    {
    return;
    }

  vtkSMProxyUndoElement* undoElement = vtkSMProxyUndoElement::New();
  undoElement->SetSession(session);
  undoElement->SetCreateElement( false );
  undoElement->SetCreationState( previousState );
  this->Add(undoElement);
  undoElement->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStackBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IgnoreAllChanges: " << this->IgnoreAllChanges << endl;
  os << indent << "UndoStack: " << this->UndoStack << endl;
}
