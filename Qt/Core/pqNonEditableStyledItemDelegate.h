// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqNonEditableStyledItemDelegate_h
#define pqNonEditableStyledItemDelegate_h

#include "pqCoreModule.h"

#include <QStyledItemDelegate>

/**
 * pqNonEditableStyledItemDelegate() can be used inside Table or TreeView as
 * ItemDelegate to make them Copy/Paste friendly. Basically it will allow
 * the user to enter in edit mode but without having the option to change the
 * content so the content can only be selected and Copy to the clipboard.
 */

class PQCORE_EXPORT pqNonEditableStyledItemDelegate : public QStyledItemDelegate
{
  typedef QStyledItemDelegate Superclass;

  Q_OBJECT

public:
  pqNonEditableStyledItemDelegate(QObject* parent = nullptr);
  QWidget* createEditor(
    QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
  Q_DISABLE_COPY(pqNonEditableStyledItemDelegate)
};

#endif
