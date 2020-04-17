/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef pqExpandableTableView_h
#define pqExpandableTableView_h

#include "pqTableView.h"
#include "pqWidgetsModule.h" // for export macro

/**
* pqExpandableTableView extends pqTableView to add support for the following
* features:
* \li Expand/Grow table: If the user edits past the last item
*     the view will fire a signal (editPastLastRow())
*     enabling observer to add new row to the table.
* \li Skip non-editable items: When editing, in a QTableView one can hit tab
*     to edit the next item. However, if the next item is not editable, the
*     editing is stopped. pqExpandableTableView makes it possible to skip
*     non-editable items and continue with the editing.
*/
class PQWIDGETS_EXPORT pqExpandableTableView : public pqTableView
{
  Q_OBJECT
  typedef pqTableView Superclass;

public:
  pqExpandableTableView(QWidget* parent = 0);
  ~pqExpandableTableView() override;

Q_SIGNALS:
  /**
  * signal fired when the user edits past the last row. Handlers can add a new
  * row to the table, if needed, to allow used to edit expandable tables with
  * ease.
  */
  void editPastLastRow();

protected:
  /**
  * Working together with logic in closeEditor(). This methods makes it
  * possible to skip past non-editable items.
  */
  QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;

  /**
  * Overridden to set MoveToNextEditableItem so that moveCursor() can skip
  * non-editable items. Also if moved past the last rows/last column, this
  * will fire the editPastLastRow() signal.
  */
  void closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint) override;

  /**
  * Overridden to capture Ctrl-V for pasting table data into the table.
  */
  void keyPressEvent(QKeyEvent* event) override;

private:
  Q_DISABLE_COPY(pqExpandableTableView)
  bool MoveToNextEditableItem;
};

#endif
