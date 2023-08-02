// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  pqExpandableTableView(QWidget* parent = nullptr);
  ~pqExpandableTableView() override;

  /**
   * Enable pasting in table from clipboard.
   * Default is true.
   */
  void setPasteEnabled(bool enable);

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

  bool PasteEnabled;

private:
  Q_DISABLE_COPY(pqExpandableTableView)
  bool MoveToNextEditableItem;
};

#endif
