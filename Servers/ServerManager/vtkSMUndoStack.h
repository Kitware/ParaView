/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoStack.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUndoStack
// .SECTION Description
// This is the undo/redo stack for the Server Manager. This provides a 
// unified face for undo/redo irrespective of number of connections, their
// type etc etc.
// 
// On every undo/redo, it fetches the XML state change from the server.
// vtkSMUndoRedoStateLoader is used to generate a vtkUndoSet object from
// the XML. GUI can subclass vtkSMUndoRedoStateLoader to handle GUI specific
// XML elements. The loader instance must be set before performing the undo,
// otherwise vtkSMUndoRedoStateLoader is used.
//
// This class also provides API to push any vtkUndoSet instance on to a 
// server. GUI can use this to push its own changes that is undoable across
// connections.
// 
// .SECTION See Also
// vtkSMUndoStackBuilder


#ifndef __vtkSMUndoStack_h
#define __vtkSMUndoStack_h

#include "vtkUndoStack.h"

class vtkPVXMLElement;
class vtkSMUndoRedoStateLoader;
class vtkSMUndoStackObserver;
class vtkUndoSet;

class VTK_EXPORT vtkSMUndoStack : public vtkUndoStack
{
public:
  static vtkSMUndoStack* New();
  vtkTypeRevisionMacro(vtkSMUndoStack, vtkUndoStack);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // It is possible to push any instance of vtkUndoSet on to the server
  // for a specific connection. Note however that once it is pushed on the server
  // every connected client will be able to undo it (evetually atleast).
  void Push(vtkIdType connectionid, const char* label, vtkUndoSet* set);

  // Description:
  // Performs an Undo using the set on the top of the undo stack. The set is poped from
  // the undo stack and pushed at the top of the redo stack. 
  // Before undo begins, it fires vtkCommand::StartEvent and when undo completes,
  // it fires vtkCommand::EndEvent.
  // \returns the status of the operation.
  virtual int Undo();

  // Description:
  // Performs a Redo using the set on the top of the redo stack. The set is poped from
  // the redo stack and pushed at the top of the undo stack. 
  // Before redo begins, it fires vtkCommand::StartEvent and when redo completes,
  // it fires vtkCommand::EndEvent.
  // \returns the status of the operation.
  virtual int Redo();

  // Description:
  // Set the state loader to be used to load the undo/redo set states.
  // By default vtkSMUndoRedoStateLoader is used.
  void SetStateLoader(vtkSMUndoRedoStateLoader*);
  vtkGetObjectMacro(StateLoader, vtkSMUndoRedoStateLoader);

  // Description:
  // Typically undo stacks have their state saved on the server. This may not 
  // be necessary always. If this flag is set, the undo stack is kept only on
  // the client. Off by default.
  vtkSetMacro(ClientOnly, int);
  vtkGetMacro(ClientOnly, int);
  vtkBooleanMacro(ClientOnly, int);

//BTX

  vtkUndoSet *getLastUndoSet(); //vistrails

protected:
  vtkSMUndoStack();
  ~vtkSMUndoStack();

  // Description:
  // Don;t call Push directly on this class. Instead use 
  // BeginOrContinueUndoSet() and EndUndoSet().
  virtual void Push(const char* , vtkUndoSet* ) 
    {
    vtkErrorMacro(
      "vtkSMUndoStack does not support calling Push without connectionID.");
    return;
    }

  // Description:
  // The method updates the client side stack. Client side stack merely contains the labels
  // for the undo/redo states and which connection they are to be performed on.
  // TODO: Eventually this method will be called as an effect of the PM telling the client
  // that something has been pushed on the server side undo stack.
  // As a consequence each client will update their undo stack status. Note,
  // only the status is updated, the actual undo state is not sent to the client
  // until it requests it. Ofcourse, this part is still not implemnted. For now,
  // multiple clients are not supported.
  void PushUndoConnection(const char* label, vtkIdType id);

  friend class vtkSMUndoStackUndoSet;
  int ProcessUndo(vtkIdType id, vtkPVXMLElement* root);
  int ProcessRedo(vtkIdType id, vtkPVXMLElement* root);

  vtkSMUndoRedoStateLoader* StateLoader;
  void OnConnectionClosed(vtkIdType cid);

  friend class vtkSMUndoStackObserver;
  void ExecuteEvent(vtkObject* called, unsigned long eventid, void* data);

  int ClientOnly;

private:
  vtkSMUndoStack(const vtkSMUndoStack&); // Not implemented.
  void operator=(const vtkSMUndoStack&); // Not implemented.
  
  vtkSMUndoStackObserver* Observer;
//ETX
};


#endif

