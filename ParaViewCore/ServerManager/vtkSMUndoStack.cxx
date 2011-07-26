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
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMUndoElement.h"
#include "vtkUndoSet.h"
#include "vtkUndoStackInternal.h"
#include "vtkSMStateLocator.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"

#include <vtksys/RegularExpression.hxx>
#include <vtkstd/set>
#include "vtkNew.h"

//*****************************************************************************
class vtkSMUndoStack::vtkInternal
{
public:
  typedef  vtkstd::set<vtkSmartPointer<vtkSMSession> >   SessionSetType;
  SessionSetType  Sessions;

  void UpdateSessions(vtkUndoSet* undoSet)
    {
    int max = undoSet->GetNumberOfElements();
    this->Sessions.clear();
    for (int cc=0; cc < max; ++cc)
      {
      vtkSMUndoElement* elem =
          vtkSMUndoElement::SafeDownCast(undoSet->GetElement(cc));
      if(elem->GetSession())
        {
        this->Sessions.insert(elem->GetSession());
        }
      }
    }

  void FillSessionsRemoteObjects(vtkCollection* collection)
    {
    SessionSetType::iterator iter = this->Sessions.begin();
    while(iter != this->Sessions.end())
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
    SessionSetType::iterator iter = this->Sessions.begin();
    while(iter != this->Sessions.end())
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
  delete this->Internal; this->Internal = NULL;
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

  int retValue = this->Superclass::Redo();
  this->Internal->Clear();

  return retValue;
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::FillWithRemoteObjects( vtkUndoSet *undoSet,
                                            vtkCollection *collection)
{
  if(!undoSet || !collection)
    return;

  this->Internal->UpdateSessions(undoSet);
  this->Internal->FillSessionsRemoteObjects(collection);
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
