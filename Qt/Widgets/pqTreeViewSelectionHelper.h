/*=========================================================================

   Program: ParaView
   Module:    pqTreeViewSelectionHelper.h

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

  //@{
  /**
   * This property holds whether the tree view support filtering.
   *
   * By default, this property is true.
   */
  bool isFilterable() const { return this->Filterable; }
  void setFilterable(bool val) { this->Filterable = val; }
  //@}
protected Q_SLOTS:
  virtual void showContextMenu(int section, const QPoint&);

private:
  Q_DISABLE_COPY(pqTreeViewSelectionHelper);

  void setSelectedItemsCheckState(Qt::CheckState state);
  QAbstractItemView* TreeView;
  bool Filterable;
};

#endif
