// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqClickableLabel.h"

#include <QMouseEvent>

//-----------------------------------------------------------------------------
pqClickableLabel::pqClickableLabel(QWidget* widget, const QString& text, const QString& tooltip,
  const QString& statusTip, QPixmap* pixmap, QWidget* parent)
  : QLabel(parent)
  , Widget(widget)
{
  this->setText(text);
  this->setToolTip(tooltip);
  this->setStatusTip(statusTip);
  if (pixmap)
  {
    this->setPixmap(*pixmap);
  }
}

//-----------------------------------------------------------------------------
void pqClickableLabel::mousePressEvent(QMouseEvent* event)
{
  Q_EMIT this->onClicked(this->Widget);
  event->accept();
}
