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
/**
 * @class   vtkSMUndoStack
 *
 * This is the undo/redo stack for the Server Manager. This provides a
 * unified face for undo/redo irrespective of number of connections, their
 * type etc etc.
 *
 * On every undo/redo, it fetches the XML state change from the server.
 * vtkSMUndoRedoStateLoader is used to generate a vtkUndoSet object from
 * the XML. GUI can subclass vtkSMUndoRedoStateLoader to handle GUI specific
 * XML elements. The loader instance must be set before performing the undo,
 * otherwise vtkSMUndoRedoStateLoader is used.
 *
 * This class also provides API to push any vtkUndoSet instance on to a
 * server. GUI can use this to push its own changes that is undoable across
 * connections.
 *
 * @sa
 * vtkSMUndoStackBuilder
*/

#ifndef vtkSMUndoStack_h
#define vtkSMUndoStack_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkUndoStack.h"

class vtkSMUndoRedoStateLoader;
class vtkSMUndoStackObserver;
class vtkUndoSet;
class vtkCollection;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMUndoStack : public vtkUndoStack
{
public:
  static vtkSMUndoStack* New();
  vtkTypeMacro(vtkSMUndoStack, vtkUndoStack);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Push an undo set on the Undo stack. This will clear
   * any sets in the Redo stack.
   */
  void Push(const char* label, vtkUndoSet* changeSet) override;

  /**
   * Performs an Undo using the set on the top of the undo stack. The set is poped from
   * the undo stack and pushed at the top of the redo stack.
   * Before undo begins, it fires vtkCommand::StartEvent and when undo completes,
   * it fires vtkCommand::EndEvent.
   * \returns the status of the operation.
   */
  int Undo() override;

  /**
   * Performs a Redo using the set on the top of the redo stack. The set is poped from
   * the redo stack and pushed at the top of the undo stack.
   * Before redo begins, it fires vtkCommand::StartEvent and when redo completes,
   * it fires vtkCommand::EndEvent.
   * \returns the status of the operation.
   */
  int Redo() override;

  enum EventIds
  {
    PushUndoSetEvent = 1987,
    ObjectCreationEvent = 1988

  };

protected:
  vtkSMUndoStack();
  ~vtkSMUndoStack() override;

  // Helper method used to push all vtkSMRemoteObject to the collection of
  // all the sessions that have been used across that undoset.
  // (It is more than likely that only one session will be find but in case of
  // collaboration, we might want to support a set of sessions.)
  // This is useful when we execute the undoset to prevent automatic
  // object deletion between 2 undo element calls when a proxy registration
  // is supposed to happen.
  void FillWithRemoteObjects(vtkUndoSet* undoSet, vtkCollection* collection);

private:
  vtkSMUndoStack(const vtkSMUndoStack&) = delete;
  void operator=(const vtkSMUndoStack&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
