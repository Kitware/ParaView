// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqListWidgetItemObject.h"

pqListWidgetItemObject::pqListWidgetItemObject(const QString& t, QListWidget* p)
  : QListWidgetItem(t, p)
{
}

void pqListWidgetItemObject::setData(int role, const QVariant& v)
{
  if (Qt::CheckStateRole == role)
  {
    if (v != this->data(Qt::CheckStateRole))
    {
      QListWidgetItem::setData(role, v);
      Q_EMIT this->checkedStateChanged(Qt::Checked == v ? true : false);
    }
  }
  else
  {
    QListWidgetItem::setData(role, v);
  }
}

bool pqListWidgetItemObject::isChecked() const
{
  return Qt::Checked == this->checkState() ? true : false;
}

void pqListWidgetItemObject::setChecked(bool v)
{
  if (v)
  {
    this->setCheckState(Qt::Checked);
  }
  else
  {
    this->setCheckState(Qt::Unchecked);
  }
}
