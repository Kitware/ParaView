// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTreeWidgetSelectionHelper.h"

// Server Manager Includes.

// Qt Includes.
#include <QMenu>
#include <QTreeWidget>
#include <QtDebug>
// ParaView Includes.

//-----------------------------------------------------------------------------
pqTreeWidgetSelectionHelper::pqTreeWidgetSelectionHelper(QTreeWidget* tree)
  : Superclass(tree)
{
  this->TreeWidget = tree;
  tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tree->setContextMenuPolicy(Qt::CustomContextMenu);

  QObject::connect(tree, SIGNAL(itemPressed(QTreeWidgetItem*, int)), this,
    SLOT(onItemPressed(QTreeWidgetItem*, int)));
  QObject::connect(tree, SIGNAL(customContextMenuRequested(const QPoint&)), this,
    SLOT(showContextMenu(const QPoint&)));
}

//-----------------------------------------------------------------------------
pqTreeWidgetSelectionHelper::~pqTreeWidgetSelectionHelper() = default;

//-----------------------------------------------------------------------------
void pqTreeWidgetSelectionHelper::onItemPressed(QTreeWidgetItem* item, int)
{
  this->PressState = -1;
  if ((item->flags() & Qt::ItemIsUserCheckable) == Qt::ItemIsUserCheckable)
  {
    this->PressState = item->checkState(0);
    this->Selection = this->TreeWidget->selectionModel()->selection();
  }
}

//-----------------------------------------------------------------------------
void pqTreeWidgetSelectionHelper::setSelectedItemsCheckState(Qt::CheckState state)
{
  // Change all checkable items in the this->Selection to match the new
  // check state.
  this->TreeWidget->selectionModel()->select(this->Selection, QItemSelectionModel::ClearAndSelect);

  QList<QTreeWidgetItem*> items = this->TreeWidget->selectedItems();
  Q_FOREACH (QTreeWidgetItem* curitem, items)
  {
    if ((curitem->flags() & Qt::ItemIsUserCheckable) == Qt::ItemIsUserCheckable)
    {
      curitem->setCheckState(/*column*/ 0, state);
    }
  }
}

//-----------------------------------------------------------------------------
void pqTreeWidgetSelectionHelper::showContextMenu(const QPoint& pos)
{
  if (!this->TreeWidget->selectionModel()->selectedIndexes().empty())
  {
    QMenu menu;
    menu.setObjectName("TreeWidgetCheckMenu");
    QAction* check = new QAction(tr("Check"), &menu);
    QAction* uncheck = new QAction(tr("Uncheck"), &menu);
    menu.addAction(check);
    menu.addAction(uncheck);
    QAction* result = menu.exec(this->TreeWidget->mapToGlobal(pos));
    if (result == check)
    {
      this->setSelectedItemsCheckState(Qt::Checked);
    }
    else if (result == uncheck)
    {
      this->setSelectedItemsCheckState(Qt::Unchecked);
    }
  }
}
