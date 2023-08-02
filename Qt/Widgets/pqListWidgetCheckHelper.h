// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqListWidgetCheckHelper_h
#define pqListWidgetCheckHelper_h

#include "pqWidgetsModule.h"
#include <QObject>

class QListWidget;
class QListWidgetItem;

/**
 * If a QListWidget has checkable elements in any column, on needs
 * to explicitly click on the checkbox to change the check state.
 * However, sometimes we simply want the checkbox to be updated
 * when the user clicks on the entire row. For that
 * purpose, we use pqListWidgetCheckHelper. Simply create
 * and instance of pqListWidgetCheckHelper, and
 * set a List to use.
 */
class PQWIDGETS_EXPORT pqListWidgetCheckHelper : public QObject
{
  Q_OBJECT
public:
  // ListWidget :- the List widget managed by this helper.
  // checkableColumn :- column index for the checkable item.
  // parent :- QObject parent.
  pqListWidgetCheckHelper(QListWidget* ListWidget, QObject* parent);

protected Q_SLOTS:
  void onItemClicked(QListWidgetItem* item);
  void onItemPressed(QListWidgetItem* item);

private:
  Q_DISABLE_COPY(pqListWidgetCheckHelper)

  QListWidget* List;
  int PressState;
};

#endif
