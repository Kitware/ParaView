// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2021 Jonas Lukasczyk
// SPDX-License-Identifier: BSD-3-Clause

#include "pqNodeEditorLabel.h"

#include <QCursor>
#include <QString>

// ----------------------------------------------------------------------------
pqNodeEditorLabel::pqNodeEditorLabel(QString label, QGraphicsItem* parent, bool cursor)
  : QGraphicsTextItem(label, parent)
{
  if (cursor)
  {
    this->setCursor(Qt::PointingHandCursor);
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorLabel::setMousePressEventCallback(
  const std::function<void(QGraphicsSceneMouseEvent*)>& callback)
{
  this->mousePressCallback = callback;
}

// ----------------------------------------------------------------------------
void pqNodeEditorLabel::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  // Let the superclass also handle how the mouse interact with the label
  // (allows drag-dropping nodes with the mouse on the label for example)
  this->QGraphicsTextItem::mousePressEvent(event);

  if (this->mousePressCallback)
  {
    this->mousePressCallback(event);
  }
}
