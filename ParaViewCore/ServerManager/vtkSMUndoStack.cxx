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
#include "vtkSMProxyManager.h"

#include <vtksys/RegularExpression.hxx>
#include <vtkstd/set>


//*****************************************************************************
// FIXME I broke VisTrail !!!
//*****************************************************************************

vtkStandardNewMacro(vtkSMUndoStack);
//-----------------------------------------------------------------------------
vtkSMUndoStack::vtkSMUndoStack()
{
}

//-----------------------------------------------------------------------------
vtkSMUndoStack::~vtkSMUndoStack()
{
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
  vtkSmartPointer<vtkCollection> remoteObjectsCollection;
  remoteObjectsCollection = vtkSmartPointer<vtkCollection>::New();
  this->FillWithRemoteObjects(this->GetNextUndoSet(), remoteObjectsCollection.GetPointer());

  int retValue = this->Superclass::Undo();

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
  vtkSmartPointer<vtkCollection> remoteObjectsCollection;
  remoteObjectsCollection = vtkSmartPointer<vtkCollection>::New();
  this->FillWithRemoteObjects(this->GetNextRedoSet(), remoteObjectsCollection.GetPointer());

  int retValue = this->Superclass::Redo();

  return retValue;
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::FillWithRemoteObjects( vtkUndoSet *undoSet,
                                            vtkCollection *collection)
{
  if(!undoSet || !collection)
    return;

  int max = undoSet->GetNumberOfElements();
  vtkstd::set<vtkSmartPointer<vtkSMSession> > sessions;
  for (int cc=0; cc < max; ++cc)
    {
    vtkSMUndoElement* elem =
        vtkSMUndoElement::SafeDownCast(undoSet->GetElement(cc));
    if(elem->GetSession())
      {
      sessions.insert(elem->GetSession());
      }
    }
  vtkstd::set<vtkSmartPointer<vtkSMSession> >::iterator iter = sessions.begin();
  while(iter != sessions.end())
    {
    iter->GetPointer()->GetAllRemoteObjects(collection);
    iter++;
    }
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
