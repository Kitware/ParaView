/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoStack.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUndoStack.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMDeserializerProtobuf.h"
#include "vtkSMMessage.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRemoteObjectUpdateUndoElement.h"
#include "vtkSMSession.h"
#include "vtkSMStateLocator.h"
#include "vtkSMUndoElement.h"
#include "vtkUndoSet.h"
#include "vtkUndoStackInternal.h"

#include "vtkNew.h"
#include <set>
#include <vtksys/RegularExpression.hxx>

//*****************************************************************************
class vtkSMUndoStack::vtkInternal
{
public:
  typedef std::set<vtkSmartPointer<vtkSMSession> > SessionSetType;
  SessionSetType Sessions;
  vtkNew<vtkSMProxyLocator> UndoSetProxyLocator;
  vtkNew<vtkSMDeserializerProtobuf> UndoSetProxyDeserializer;
  vtkNew<vtkSMStateLocator> UndoSetStateLocator;

  vtkInternal()
  {
    this->UndoSetProxyDeserializer->SetStateLocator(this->UndoSetStateLocator.GetPointer());
    this->UndoSetProxyLocator->SetDeserializer(this->UndoSetProxyDeserializer.GetPointer());
    this->UndoSetProxyLocator->UseSessionToLocateProxy(true);
  }

  void FillLocatorWithUndoStates(vtkUndoSet* undoSet, bool useBeforeState)
  {
    this->UndoSetStateLocator->UnRegisterAllStates(false);
    int max = undoSet->GetNumberOfElements();
    for (int cc = 0; cc < max; ++cc)
    {
      vtkSMRemoteObjectUpdateUndoElement* elem =
        vtkSMRemoteObjectUpdateUndoElement::SafeDownCast(undoSet->GetElement(cc));

      if (elem)
      {
        elem->SetProxyLocator(this->UndoSetProxyLocator.GetPointer());
        if (useBeforeState)
        {
          this->UndoSetStateLocator->RegisterState(elem->BeforeState);
        }
        else
        {
          this->UndoSetStateLocator->RegisterState(elem->AfterState);
        }
      }
    }
  }

  void UpdateSessions(vtkUndoSet* undoSet)
  {
    int max = undoSet->GetNumberOfElements();
    this->Sessions.clear();
    for (int cc = 0; cc < max; ++cc)
    {
      vtkSMUndoElement* elem = vtkSMUndoElement::SafeDownCast(undoSet->GetElement(cc));
      if (elem->GetSession())
      {
        this->Sessions.insert(elem->GetSession());
      }
    }

    assert("Undo element should not involve more than one session" && this->Sessions.size() < 2);
    if (this->Sessions.size() == 1)
    {
      this->UndoSetStateLocator->SetParentLocator(
        this->Sessions.begin()->GetPointer()->GetStateLocator());
    }
  }

  void FillSessionsRemoteObjects(vtkCollection* collection)
  {
    SessionSetType::iterator iter = this->Sessions.begin();
    while (iter != this->Sessions.end())
    {
      iter->GetPointer()->GetAllRemoteObjects(collection);
      iter++;
    }
  }

  void Clear()
  {
    this->ClearProxyLocators();
    this->Sessions.clear();
  }

  void ClearProxyLocators()
  {
    this->UndoSetProxyLocator->Clear();
    SessionSetType::iterator iter = this->Sessions.begin();
    while (iter != this->Sessions.end())
    {
      iter->GetPointer()->GetProxyLocator()->Clear();
      iter++;
    }
  }
};
//*****************************************************************************
vtkStandardNewMacro(vtkSMUndoStack);
//-----------------------------------------------------------------------------
vtkSMUndoStack::vtkSMUndoStack()
{
  this->Internal = new vtkInternal();
}

//-----------------------------------------------------------------------------
vtkSMUndoStack::~vtkSMUndoStack()
{
  delete this->Internal;
  this->Internal = NULL;
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::Push(const char* label, vtkUndoSet* changeSet)
{
  this->Superclass::Push(label, changeSet);
  this->InvokeEvent(PushUndoSetEvent, changeSet);
}

//-----------------------------------------------------------------------------
int vtkSMUndoStack::Undo()
{
  if (!this->CanUndo())
  {
    vtkErrorMacro("Cannot undo. Nothing on undo stack.");
    return 0;
  }

  // Hold remote objects refs while the UndoSet is processing
  vtkNew<vtkCollection> remoteObjectsCollection;
  this->FillWithRemoteObjects(this->GetNextUndoSet(), remoteObjectsCollection.GetPointer());
  this->Internal->FillLocatorWithUndoStates(this->GetNextUndoSet(), true);

  int retValue = this->Superclass::Undo();
  this->Internal->Clear();

  return retValue;
}

//-----------------------------------------------------------------------------
int vtkSMUndoStack::Redo()
{
  if (!this->CanRedo())
  {
    vtkErrorMacro("Cannot redo. Nothing on redo stack.");
    return 0;
  }

  // Hold remote objects refs while the UndoSet is processing
  vtkNew<vtkCollection> remoteObjectsCollection;
  this->FillWithRemoteObjects(this->GetNextRedoSet(), remoteObjectsCollection.GetPointer());
  this->Internal->FillLocatorWithUndoStates(this->GetNextRedoSet(), false);

  int retValue = this->Superclass::Redo();
  this->Internal->Clear();

  return retValue;
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::FillWithRemoteObjects(vtkUndoSet* undoSet, vtkCollection* collection)
{
  if (!undoSet || !collection)
    return;

  this->Internal->UpdateSessions(undoSet);
  this->Internal->FillSessionsRemoteObjects(collection);
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
