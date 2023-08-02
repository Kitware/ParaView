// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTreeViewExpandState_h
#define pqTreeViewExpandState_h

#include "pqWidgetsModule.h" // for exports
#include <QObject>
#include <QScopedPointer> // for ivar
class QTreeView;

/**
 * @class pqTreeViewExpandState
 * @brief save/restore expand-state for items in a tree view.
 *
 * pqTreeViewExpandState is a helper class that can be used to save/restore
 * expansion state for nodes in the tree view even after the view's model has been
 * reset or severely modified.
 *
 * Example usage:
 * @code{cpp}
 *
 *  pqTreeViewExpandState helper;
 *  helper.save(treeView);
 *
 *  treeView->model()->reset();
 *  // do other changes to model.
 *
 *  // optional default expand state.
 *  treeView->expandToDepth(1);
 *
 *  helper.restore(treeView);
 *
 * @endcode
 *
 * Applications often want to have a default expand state for tree views. To do
 * that, simply call `QTreeView::expand`, `QTreeView::expandAll`, or
 * `QTreeView::expandToDepth` before calling `restore`. pqTreeViewExpandState
 * ensures that it doesn't not save (and hence restore) state for the tree if
 * the tree has only 1 or less nodes when `save` is called.
 *
 */
class PQWIDGETS_EXPORT pqTreeViewExpandState : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqTreeViewExpandState(QObject* parent = nullptr);
  ~pqTreeViewExpandState() override;

  void save(QTreeView* treeView);
  void restore(QTreeView* treeView);

private:
  Q_DISABLE_COPY(pqTreeViewExpandState);
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
