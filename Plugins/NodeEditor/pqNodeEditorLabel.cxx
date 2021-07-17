/*=========================================================================

  Program:   ParaView
  Plugin:    NodeEditor

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  ParaViewPluginsNodeEditor - BSD 3-Clause License - Copyright (C) 2021 Jonas Lukasczyk

  See the Copyright.txt file provided
  with ParaViewPluginsNodeEditor for license information.
-------------------------------------------------------------------------*/

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
