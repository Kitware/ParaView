/*=========================================================================

   Program: ParaView
   Module:    pqUndoStackBuilder.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
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
