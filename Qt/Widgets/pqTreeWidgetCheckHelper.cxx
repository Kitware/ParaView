// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqTreeWidgetCheckHelper.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>

//-----------------------------------------------------------------------------
pqTreeWidgetCheckHelper::pqTreeWidgetCheckHelper(QTreeWidget* tree, int checkableColumn, QObject* p)
  : QObject(p)
{
  this->Mode = CLICK_IN_ROW;
  this->Tree = tree;
  this->CheckableColumn = checkableColumn;
  this->PressState = -1;
  QObject::connect(this->Tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this,
    SLOT(onItemClicked(QTreeWidgetItem*, int)));
  QObject::connect(this->Tree, SIGNAL(itemPressed(QTreeWidgetItem*, int)), this,
    SLOT(onItemPressed(QTreeWidgetItem*, int)));
}

//-----------------------------------------------------------------------------
void pqTreeWidgetCheckHelper::onItemPressed(QTreeWidgetItem* item, int /*column*/)
{
  this->PressState = item->checkState(this->CheckableColumn);
}

//-----------------------------------------------------------------------------
void pqTreeWidgetCheckHelper::onItemClicked(QTreeWidgetItem* item, int column)
{
  if (this->Mode == CLICK_IN_COLUMN && column != this->CheckableColumn)
  {
    return;
  }
  Qt::CheckState state = item->checkState(this->CheckableColumn);
  if (this->PressState != state)
  {
    // the click was on the check box itself, hence the state is already toggled.
    return;
  }
  if (state == Qt::Unchecked)
  {
    state = Qt::Checked;
  }
  else if (state == Qt::Checked)
  {
    state = Qt::Unchecked;
  }
  item->setCheckState(this->CheckableColumn, state);
  this->PressState = -1;
}

//-----------------------------------------------------------------------------
