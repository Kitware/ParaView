// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqNonEditableStyledItemDelegate.h"

#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QWidget>

//-----------------------------------------------------------------------------
pqNonEditableStyledItemDelegate::pqNonEditableStyledItemDelegate(QObject* p)
  : QStyledItemDelegate(p)
{
}

//-----------------------------------------------------------------------------
QWidget* pqNonEditableStyledItemDelegate::createEditor(
  QWidget* parentObject, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  Q_UNUSED(option);
  Q_UNUSED(index);

  QLineEdit* lineEdit = new QLineEdit(parentObject);
  lineEdit->setReadOnly(true);

  return lineEdit;
}
