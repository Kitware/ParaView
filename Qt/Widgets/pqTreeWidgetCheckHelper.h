// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqTreeWidgetCheckHelper_h
#define pqTreeWidgetCheckHelper_h

#include "pqWidgetsModule.h"
#include <QObject>

class QTreeWidget;
class QTreeWidgetItem;

/**
 * If a QTreeWidget has checkable elements in any column, one needs
 * to explicitly click on the checkbox to change the check state.
 * However, sometimes we simply want the checkbox to be updated
 * when the user clicks on the entire row. For that
 * purpose, we use pqTreeWidgetCheckHelper. Simply create
 * and instance of pqTreeWidgetCheckHelper, and set a tree to use.
 */
class PQWIDGETS_EXPORT pqTreeWidgetCheckHelper : public QObject
{
  Q_OBJECT
public:
  // treeWidget :- the tree widget managed by this helper.
  // checkableColumn :- column index for the checkable item.
  // parent :- QObject parent.
  pqTreeWidgetCheckHelper(QTreeWidget* treeWidget, int checkableColumn, QObject* parent);

  enum CheckMode
  {
    CLICK_IN_COLUMN, // toggle check state when clicked in the column
                     // with the checkable item.
    CLICK_IN_ROW,    // toggle check state when clicked in the row
                     // with the checkable item (default).
  };

  // Check Mode controls whether the user must click in the column
  // with the checkable item or any column in the same row.
  void setCheckMode(CheckMode mode) { this->Mode = mode; }
  CheckMode checkMode() const { return this->Mode; }

protected Q_SLOTS:
  void onItemClicked(QTreeWidgetItem* item, int column);
  void onItemPressed(QTreeWidgetItem* item, int column);

private:
  Q_DISABLE_COPY(pqTreeWidgetCheckHelper)

  QTreeWidget* Tree;
  int CheckableColumn;
  int PressState;
  CheckMode Mode;
};

#endif
