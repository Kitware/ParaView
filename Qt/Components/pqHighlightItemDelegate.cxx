// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/// \file pqHighlightItemDelegate.cxx
/// \date 02/20/2014

#include "pqHighlightItemDelegate.h"
#include <QPainter>

void pqHighlightItemDelegate::paint(
  QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  if (index.data().isValid())
  {
    QStyleOptionViewItem viewOption(option);
    painter->save();
    painter->fillRect(option.rect, this->HighlightColor);
    painter->restore();
    QStyledItemDelegate::paint(painter, viewOption, index);
  }
  else
  {
    QStyledItemDelegate::paint(painter, option, index);
  }
}
