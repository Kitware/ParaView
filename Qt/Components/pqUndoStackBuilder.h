// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqUndoStackBuilder_h
#define pqUndoStackBuilder_h

#include "pqComponentsModule.h"
#include "vtkSMMessageMinimal.h"
#include "vtkSMUndoStackBuilder.h"

class vtkCommand;

/**
 * pqUndoStackBuilder extends the vtkSMUndoStackBuilder as follows:
 * \li If properties on registered proxies are changed when the builder is not
 * with a BeginOrContinueUndoSet - EndUndoSet block, unless
 * IgnoreIsolatedChanges is true, it will create a UndoSet with that change
 * alone and push it on the stack. IgnoreIsolatedChanges does not have any
 * effect in a BeginOrContinueUndoSet-EndUndoSet block.
 * \li pqProxy objects can be added to be ignored for all changes. Thus,
 * any change event triggered from any of the proxies in the ignore list
 * don't result in updating of the undo stack even when those changes happen
 * with a BeginOrContinueUndoSet - EndUndoSet block (this feature isn't
 * implemented currently).
 * With these extensions, following are the points we must remember to ensure
 * that Undo/Redo works.
 * \li For any multi-change operations or proxy creation/registration
 *     operations such as creating of a source/reader/filter etc. we must
 *     explicitly use Begin() and End() blocks.
 * \li For modal dialogs which change a bunch of properties when the user
 *     hits Ok, such as Application Settings dialog, change input dialogs,
 *     we must use Begin() and End().
 * \li Any GUI wiget that is a single widget but changes multiple server
 *     manager properties or can lead to multiple undo steps should use
 *     Begin and End block eg. Accept button, widget to select scalar to color
 *     by etc.
 * \li GUI Widgets that are directly linked to a Server Manager property and
 *     don't need to be accepted, don't need to worry about Undo/Redo at all.
 *     The pqUndoStackBuilder will automatically create elements for such
 *     changes and even try to given them friendly labels eg. most Animation
 *     panel properties, display panel widgets etc.
 * Currently, this class automatically adds property modifications alone to
 * the stack. We may want to explore the possibility of automatically adding
 * all types of server manager modifications to the stack.
 */
class PQCOMPONENTS_EXPORT pqUndoStackBuilder : public vtkSMUndoStackBuilder
{
public:
  static pqUndoStackBuilder* New();
  vtkTypeMacro(pqUndoStackBuilder, vtkSMUndoStackBuilder);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set if all modifications triggered when not within a Begin/End
   * block should be added to the undo stack or not. On by default.
   * Must be set before performing changes to Server Manager which the
   * GUI is certain shouldn't be added to the Undo stack.
   */
  vtkGetMacro(IgnoreIsolatedChanges, bool);
  vtkSetMacro(IgnoreIsolatedChanges, bool);

  /**
   * Overridden to add observers to not record changes when the
   * stack is being undone/redone.
   */
  void SetUndoStack(vtkSMUndoStack*) override;

  /**
   * Overridden to filter unwanted event and manage auto undoset creation
   */
  void OnStateChange(vtkSMSession* session, vtkTypeUInt32 globalId,
    const vtkSMMessage* previousState, const vtkSMMessage* newState) override;

protected:
  pqUndoStackBuilder();
  ~pqUndoStackBuilder() override;

  /**
   * Return false if this state should be escaped.
   */
  bool Filter(vtkSMSession* session, vtkTypeUInt32 globalId);

  bool IgnoreIsolatedChanges;
  bool UndoRedoing;

private:
  pqUndoStackBuilder(const pqUndoStackBuilder&) = delete;
  void operator=(const pqUndoStackBuilder&) = delete;
};

#endif
