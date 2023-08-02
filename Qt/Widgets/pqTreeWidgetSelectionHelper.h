// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTreeWidgetSelectionHelper_h
#define pqTreeWidgetSelectionHelper_h

#include "pqWidgetsModule.h"
#include <QItemSelection>
#include <QObject>

class QTreeWidget;
class QTreeWidgetItem;
/**
 * pqTreeWidgetSelectionHelper enables multiple element selection and the
 * toggling on then changing the checked state of the selected elements.
 * Hence, the user can do things like selecting a subset of nodes and then
 * (un)checking all of them etc. This cannot work in parallel with
 * pqTreeWidgetCheckHelper, hence only once of the two must be used on the same
 * tree widget.
 * CAVEATS: This helper currently assumes that the 0-th column is checkable (if
 * at all). This can be fixed if needed.
 */
class PQWIDGETS_EXPORT pqTreeWidgetSelectionHelper : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqTreeWidgetSelectionHelper(QTreeWidget* treeWidget);
  ~pqTreeWidgetSelectionHelper() override;

protected Q_SLOTS:
  void onItemPressed(QTreeWidgetItem* item, int column);
  void showContextMenu(const QPoint&);

private:
  Q_DISABLE_COPY(pqTreeWidgetSelectionHelper)

  void setSelectedItemsCheckState(Qt::CheckState state);

  QTreeWidget* TreeWidget;
  QItemSelection Selection;
  int PressState;
};

#endif
