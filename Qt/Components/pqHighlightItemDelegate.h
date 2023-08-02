// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqHighlightItemDelegate_h
#define pqHighlightItemDelegate_h

#include "pqComponentsModule.h"
#include <QStyledItemDelegate>

/**
 * pqHighlightItemDelegate is a delegate used to highlight item views
 * It is currently used to highlight matching items found using the
 * pqItemViewSearchWidget. It works by repainting the item with a
 * colored background.
 */

class PQCOMPONENTS_EXPORT pqHighlightItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  /**
   * Construct the pqHighlightItemDelegate
   * The variable color is used to specify the highlight color,
   * defaults to QColor(Qt::white)
   */
  pqHighlightItemDelegate(QColor color = QColor(Qt::white), QObject* parentObject = nullptr)
    : QStyledItemDelegate(parentObject)
    , HighlightColor(color)
  {
  }

  void paint(
    QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
  QColor HighlightColor;
};

#endif
