// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTreeViewSelectionHelper_h
#define pqTreeViewSelectionHelper_h

#include "pqWidgetsModule.h"
#include <QAbstractItemView>
#include <QItemSelection>
#include <QObject>

/**
 * @class pqTreeViewSelectionHelper
 * @brief helper class to add selection/sort/filter context menu to QAbstractItemView.
 *
 * pqTreeViewSelectionHelper is used to add a custom context menu to QAbstractItemView
 * to do common actions such as checking/unchecking highlighted items,
 * sorting, and filtering.
 *
 * If the QAbstractItemView has a pqHeaderView as the header then this class calls
 * `pqHeaderView::setCustomIndicatorIcon` and also shows the context menu when the
 * custom indicator icon is clicked in the header.
 *
 * Sorting and filtering options are added to the context menu only if the model
 * associated with the tree view is `QSortFilterProxyModel` or subclass.
 *
 * Filtering can be enabled/disabled using the `filterable` property. Default is
 * enabled. When enabling filtering for a tree view where the tree is more than
 * 1 level deep, keep in mind that you may need to enable
 * `recursiveFilteringEnabled` on QSortFilterProxyModel (supported in Qt 5.10
 * and later) to ensure that the filtering correctly traverses the tree. For
 * older Qt versions, it's recommended that you disable filtering support for
 * non-flat trees using `pqTreeViewSelectionHelper::setFilterable(false)`.
 *
 */
class PQWIDGETS_EXPORT pqTreeViewSelectionHelper : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
  Q_PROPERTY(bool filterable READ isFilterable WRITE setFilterable);

public:
  pqTreeViewSelectionHelper(QAbstractItemView* view, bool customIndicator = true);
  ~pqTreeViewSelectionHelper() override;

  ///@{
  /**
   * This property holds whether the tree view support filtering.
   *
   * By default, this property is true.
   */
  bool isFilterable() const { return this->Filterable; }
  void setFilterable(bool val) { this->Filterable = val; }
  //@}

protected:
  virtual void buildupMenu(QMenu& menu, int section, const QPoint& pos);
  QAbstractItemView* TreeView;

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  virtual void showContextMenu(int section, const QPoint&);

private:
  Q_DISABLE_COPY(pqTreeViewSelectionHelper);

  void setSelectedItemsCheckState(Qt::CheckState state);
  bool Filterable;
};

#endif
