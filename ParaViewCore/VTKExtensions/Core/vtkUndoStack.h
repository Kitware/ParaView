/*=========================================================================

  Program:   ParaView
  Module:    vtkUndoStack.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUndoStack
 * @brief   undo/redo stack.
 *
 * This an undo stack. Each undo/redo-able operation is a vtkUndoSet object.
 * This class fires a vtkCommand::ModifiedEvent when  the undo/redo stack
 * changes.
 *
 * On Undo, vtkUndoSet::Undo is called on the vtkUndoSet at the top of the
 * undo stack and the set is pushed onto the top of the redo stack.
 * On Redo, vtkUndoSet::Redo is called on the vtkUndoSet at the top of the
 * redo stack and the set is pushed onto the top of the undo stack.
 * When a vtkUndoSet is pushed on the undo stack, the redo stack is
 * cleared.
 *
 * Each undo set are assigned user-readable labels providing information about
 * the operation(s) that will be undone/redone.
 *
 * vtkUndoElement, vtkUndoSet and vtkUndoStack form the undo/redo framework core.
 * @sa
 * vtkUndoSet vtkUndoElement
*/

#ifndef vtkUndoStack_h
#define vtkUndoStack_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro

class vtkUndoStackInternal;
class vtkUndoSet;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkUndoStack : public vtkObject
{
public:
  enum EventIds
  {
    UndoSetRemovedEvent = 1989,
    UndoSetClearedEvent = 1990
  };

  static vtkUndoStack* New();
  vtkTypeMacro(vtkUndoStack, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Push an undo set on the Undo stack. This will clear
   * any sets in the Redo stack.
   */
  virtual void Push(const char* label, vtkUndoSet* changeSet);

  /**
   * Returns the label for the set at the given Undo position.
   * 0 is the current undo set, 1 is the one preceding to the current one
   * and so on.
   * \returns NULL is no set exists at the given index, otherwise the label
   * for the change set.
   */
  const char* GetUndoSetLabel(unsigned int position);

  /**
   * Returns the label for the set at the given Redo position.
   * 0 is the next set to redo, 1 is the one after the next one
   * and so on.
   * \returns NULL is no set exists at the given index, otherwise the label
   * for the change set.
   */
  const char* GetRedoSetLabel(unsigned int position);

  /**
   * Returns the number of sets on the undo stack.
   */
  unsigned int GetNumberOfUndoSets();

  /**
   * Returns the number of sets on the undo stack.
   */
  unsigned int GetNumberOfRedoSets();

  /**
   * Returns if undo operation can be performed.
   */
  int CanUndo() { return (this->GetNumberOfUndoSets() > 0); }

  /**
   * Returns if redo operation can be performed.
   */
  int CanRedo() { return (this->GetNumberOfRedoSets() > 0); }

  /**
   * Get the UndoSet on the top of the Undo stack, if any.
   */
  virtual vtkUndoSet* GetNextUndoSet();

  /**
   * Get the UndoSet on the top of the Redo stack, if any.
   */
  virtual vtkUndoSet* GetNextRedoSet();

  /**
   * Performs an Undo using the set on the top of the undo stack. The set is poped from
   * the undo stack and pushed at the top of the redo stack.
   * Before undo begins, it fires vtkCommand::StartEvent and when undo completes,
   * it fires vtkCommand::EndEvent.
   * \returns the status of the operation.
   */
  virtual int Undo();

  /**
   * Performs a Redo using the set on the top of the redo stack. The set is poped from
   * the redo stack and pushed at the top of the undo stack.
   * Before redo begins, it fires vtkCommand::StartEvent and when redo completes,
   * it fires vtkCommand::EndEvent.
   * \returns the status of the operation.
   */
  virtual int Redo();

  /**
   * Pop the undo stack. The UndoElement on the top of the undo stack is popped from the
   * undo stack and pushed on the redo stack. This is same as Undo() except that the
   * vtkUndoElement::Undo() is not invoked.
   */
  void PopUndoStack();

  /**
   * Pop the redo stack. The UndoElement on the top of the redo stack is popped and then
   * pushed on the undo stack. This is same as Redo() except that vtkUndoElement::Redo()
   * is not invoked.
   */
  void PopRedoStack();

  /**
   * Clears all the undo/redo elements from the stack.
   */
  void Clear();

  //@{
  /**
   * Returns if the stack is currently being undone.
   */
  vtkGetMacro(InUndo, bool);
  //@}

  //@{
  /**
   * Returns if the stack is currently being redone.
   */
  vtkGetMacro(InRedo, bool);
  //@}

  //@{
  /**
   * Get set the maximum stack depth. As more entries are pushed on the stack,
   * if its size exceeds this limit then old entries will be removed.
   * Default is 10.
   */
  vtkSetClampMacro(StackDepth, int, 1, 100);
  vtkGetMacro(StackDepth, int);

protected:
  vtkUndoStack();
  ~vtkUndoStack() override;
  //@}

  vtkUndoStackInternal* Internal;
  int StackDepth;

private:
  vtkUndoStack(const vtkUndoStack&) = delete;
  void operator=(const vtkUndoStack&) = delete;

  bool InUndo;
  bool InRedo;
};

#endif
