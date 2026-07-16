// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAdjustToCurrentComboBox.h"

#include <QFontMetrics>
#include <QStyle>
#include <QStyleOptionComboBox>

//-----------------------------------------------------------------------------
pqAdjustToCurrentComboBox::pqAdjustToCurrentComboBox(QWidget* parentObject)
  : Superclass(parentObject)
{
  this->connect(
    this, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QWidget::updateGeometry);
}

//-----------------------------------------------------------------------------
QSize pqAdjustToCurrentComboBox::sizeHint() const
{
  this->ensurePolished();

  QStyleOptionComboBox opt;
  this->initStyleOption(&opt);

  const QFontMetrics fm = this->fontMetrics();
  const QString text = this->currentText();
  QSize contentSize = fm.size(Qt::TextSingleLine, text.isEmpty() ? QString(7, 'x') : text);

  if (!this->itemIcon(this->currentIndex()).isNull())
  {
    const QSize iconExtent = this->iconSize();
    contentSize.setWidth(contentSize.width() + iconExtent.width() + 4);
    contentSize.setHeight(qMax(contentSize.height(), iconExtent.height()));
  }

  return this->style()->sizeFromContents(QStyle::CT_ComboBox, &opt, contentSize, this);
}

//-----------------------------------------------------------------------------
QSize pqAdjustToCurrentComboBox::minimumSizeHint() const
{
  return this->sizeHint();
}
