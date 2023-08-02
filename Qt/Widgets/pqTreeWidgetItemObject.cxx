// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqTreeWidgetItemObject.h"

pqTreeWidgetItemObject::pqTreeWidgetItemObject(const QStringList& t, int item_type)
  : QTreeWidgetItem(t, item_type)
{
}

pqTreeWidgetItemObject::pqTreeWidgetItemObject(QTreeWidget* p, const QStringList& t, int item_type)
  : QTreeWidgetItem(p, t, item_type)
{
}

pqTreeWidgetItemObject::pqTreeWidgetItemObject(
  QTreeWidgetItem* p, const QStringList& t, int item_type)
  : QTreeWidgetItem(p, t, item_type)
{
}

void pqTreeWidgetItemObject::setData(int column, int role, const QVariant& v)
{
  if (Qt::CheckStateRole == role)
  {
    if (v != this->data(column, Qt::CheckStateRole))
    {
      QTreeWidgetItem::setData(column, role, v);
      Q_EMIT this->checkedStateChanged(Qt::Checked == v ? true : false);
    }
  }
  else
  {
    QTreeWidgetItem::setData(column, role, v);
  }
  Q_EMIT this->modified();
}

bool pqTreeWidgetItemObject::isChecked() const
{
  return Qt::Checked == this->checkState(0) ? true : false;
}

void pqTreeWidgetItemObject::setChecked(bool v)
{
  if (v)
  {
    this->setCheckState(0, Qt::Checked);
  }
  else
  {
    this->setCheckState(0, Qt::Unchecked);
  }
}
