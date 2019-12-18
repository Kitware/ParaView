/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoStackBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMUndoStackBuilder
 * @brief   builds server manager undo sets and
 * pushes them on the undo stack.
 *
 * vtkSMUndoStackBuilder records Server Manager changes that are undo/redo
 * able and collects them. To begin recording such changes one must call
 * Begin(). To end recording use End(). One can have multiple blocks
 * of Begin-End before pushing the changes on the Undo Stack. To push all
 * collected changes onto the Undo Stack as a single undoable step,
 * use PushToStack().
 * Applications can subclass vtkSMUndoStackBuilder to record GUI related
 * changes and add them to the undo stack.
*/

#ifndef vtkSMUndoStackBuilder_h
#define vtkSMUndoStackBuilder_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMMessageMinimal.h"            // needed for vtkSMMessage
#include "vtkSMObject.h"

class vtkSMProxy;
class vtkSMUndoStack;
class vtkUndoElement;
class vtkUndoSet;
class vtkSMSession;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMUndoStackBuilder : public vtkSMObject
{
public:
  static vtkSMUndoStackBuilder* New();
  vtkTypeMacro(vtkSMUndoStackBuilder, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Begins monitoring of the vtkSMProxyManager for undoable operations.
   * All noted actions are converted to UndoElements and collected.
   * One vtkUndoElement is created per action. All undo elements
   * become a part of a vtkUndoSet which is pushed on to
   * the Undo Stack on PushToStack().
   * \c label is a suggestion for the UndoSet that will be built. If the
   * UndoSet already has elements implying it hasn't been pushed to the stack
   * then the label is ignored.
   */
  virtual void Begin(const char* label);

  /**
   * Stops monitoring of the vtkSMProxyManager for undoable operations.
   * Any changes made to the proxy manager will not be converted
   * to UndoElements. This method does not push the vtkUndoSet of
   * undo elements built. One must call PushToStack() to push
   * the UndoSet to the Undo stack. Alternatively, one can use the
   * EndAndPushToStack() method which combines End() and PushToStack().
   */
  virtual void End();

  //@{
  /**
   * Convenience method call End(); PushToStack(); in that order.
   */
  void EndAndPushToStack()
  {
    this->End();
    this->PushToStack();
  }
  //@}

  /**
   * If any undoable changes were recorded by the builder, this will push
   * the vtkUndoSet formed on to the UndoStack. The UndoStack which the builder
   * is building must be set by using SetUndoStack(). If the UndoSet
   * is empty, it is not pushed on the stack. After pushing, the UndoSet is cleared
   * so the builder is ready to collect new modifications.
   */
  virtual void PushToStack();

  /**
   * Discard all recorded changes that haven't been pushed on the UndoStack.
   */
  virtual void Clear();

  /**
   * One can add arbitrary elements to the active undo set.
   * It is essential that the StateLoader on the UndoStack can handle the
   * arbitrary undo elements.
   * If that element has been escaped for any reason, the method will return false;
   */
  virtual bool Add(vtkUndoElement* element);

  //@{
  /**
   * Get/Set the undo stack that this builder will build.
   */
  vtkGetObjectMacro(UndoStack, vtkSMUndoStack);
  virtual void SetUndoStack(vtkSMUndoStack*);
  //@}

  //@{
  /**
   * If IgnoreAllChanges is true, any server manager changes will be
   * ignored even if the changes happened within a Begin()-End() call.
   * This provides a mechanism for the application to perform non-undoable
   * operations irrespective of whether a undo set if being built.
   * By default, it is set to false.
   */
  vtkSetMacro(IgnoreAllChanges, bool);
  vtkGetMacro(IgnoreAllChanges, bool);
  //@}

  // Record a state change on a RemoteObject
  virtual void OnStateChange(vtkSMSession* session, vtkTypeUInt32 globalId,
    const vtkSMMessage* previousState, const vtkSMMessage* newState);

  // Indicate that a new object was created.
  // Simply fires the vtkSMUndoStack::ObjectCreationEvent from the undo-stack.
  virtual void OnCreateObject(vtkSMSession* session, vtkSMMessage* newState);

protected:
  vtkSMUndoStackBuilder();
  ~vtkSMUndoStackBuilder() override;

  vtkSMUndoStack* UndoStack;
  vtkUndoSet* UndoSet;
  char* Label;
  vtkSetStringMacro(Label);

  /**
   * Returns if the event raised by the proxy manager should be
   * converted to undo elements.
   */
  virtual bool HandleChangeEvents() { return (this->EnableMonitoring > 0); }

  void InitializeUndoSet();

  // used to count Begin/End call to make sure they stay consistent
  // and make sure that a begin occurs before recording any event
  int EnableMonitoring;
  bool IgnoreAllChanges;

private:
  vtkSMUndoStackBuilder(const vtkSMUndoStackBuilder&) = delete;
  void operator=(const vtkSMUndoStackBuilder&) = delete;
};

#endif
