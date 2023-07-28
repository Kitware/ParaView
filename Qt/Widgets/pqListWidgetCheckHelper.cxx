// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqListWidgetCheckHelper.h"

#include <QListWidget>
#include <QListWidgetItem>

//-----------------------------------------------------------------------------
pqListWidgetCheckHelper::pqListWidgetCheckHelper(QListWidget* list, QObject* p)
  : QObject(p)
{
  this->List = list;
  QObject::connect(
    this->List, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(onItemClicked(QListWidgetItem*)));
  QObject::connect(
    this->List, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(onItemPressed(QListWidgetItem*)));
}

//-----------------------------------------------------------------------------
void pqListWidgetCheckHelper::onItemPressed(QListWidgetItem* item)
{
  this->PressState = item->checkState();
}

//-----------------------------------------------------------------------------
void pqListWidgetCheckHelper::onItemClicked(QListWidgetItem* item)
{
  Qt::CheckState state = item->checkState();
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
  item->setCheckState(state);
}

//-----------------------------------------------------------------------------
