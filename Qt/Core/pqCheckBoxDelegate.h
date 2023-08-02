// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqCoreModule.h"
#include <QStyledItemDelegate>

/**
 * Delegate for QTableView to draw a checkbox as an left-right (unchecked)
 * and top-bottom (checked) arrow.
 * The checkbox has an extra state for unchecked disabled.
 * Based on a Stack overflow answer:
 * http://stackoverflow.com/questions/3363190/qt-qtableview-how-to-have-a-checkbox-only-column
 */
class PQCORE_EXPORT pqCheckBoxDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  enum CheckBoxValues
  {
    NOT_EXPANDED,
    EXPANDED,
    NOT_EXPANDED_DISABLED
  };

  pqCheckBoxDelegate(QObject* parent);
  ~pqCheckBoxDelegate() override;

  void paint(
    QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
    const QModelIndex& index) override;

private:
  Q_DISABLE_COPY(pqCheckBoxDelegate)
  struct pqInternals;
  pqInternals* Internals;
};
