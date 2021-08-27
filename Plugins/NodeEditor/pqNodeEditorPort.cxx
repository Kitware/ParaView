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

#include "pqNodeEditorPort.h"

#include "pqNodeEditorUtils.h"

// qt includes
#include <QApplication>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QPalette>
#include <QPen>

#include <iostream>

// ----------------------------------------------------------------------------
namespace details
{
/**
 * Custom class for handling the port label with the right font and mouse shape.
 */
class PortLabel : public QGraphicsTextItem
{
public:
  PortLabel(QString label, QGraphicsItem* parent)
    : QGraphicsTextItem(label, parent)
  {
    this->setCursor(Qt::PointingHandCursor);
  };

  ~PortLabel() override = default;

  void hoverEnterEvent(QGraphicsSceneHoverEvent* /*event*/) override
  {
    auto font = this->font();
    font.setBold(true);
    this->setFont(font);
  };

  void hoverLeaveEvent(QGraphicsSceneHoverEvent* /*event*/) override
  {
    auto font = this->font();
    font.setBold(false);
    this->setFont(font);
  };
};
}

// ----------------------------------------------------------------------------
pqNodeEditorPort::pqNodeEditorPort(Type type, QString name, QGraphicsItem* parent)
  : QGraphicsItem(parent)
{
  this->setZValue(pqNodeEditorUtils::CONSTS::PORT_LAYER);

  this->label = new details::PortLabel(name, this);

  // Set the port label at the right of the actual port for input ports and at the left
  // for output ports. Use an offset to not make the label too close of the node border.
  constexpr qreal LABEL_OFFSET = 3.0;

  qreal portRadius = pqNodeEditorUtils::CONSTS::PORT_RADIUS;

  const qreal xpos = (type == Type::INPUT)
    ? portRadius + LABEL_OFFSET
    : -portRadius - LABEL_OFFSET - this->label->boundingRect().width();
  this->label->setPos(xpos, -0.5 * this->label->boundingRect().height());

  this->disc =
    new QGraphicsEllipseItem(-portRadius, -portRadius, 2.0 * portRadius, 2.0 * portRadius, this);
  this->setMarkedAsSelected(false);
  this->setMarkedAsVisible(false);
}

// ----------------------------------------------------------------------------
int pqNodeEditorPort::setMarkedAsSelected(bool selected)
{
  this->disc->setPen(
    QPen(selected ? QApplication::palette().highlight() : QApplication::palette().light(),
      pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH));
  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorPort::setMarkedAsVisible(bool visible)
{
  this->disc->setBrush(
    visible ? pqNodeEditorUtils::CONSTS::COLOR_DARK_ORANGE : QApplication::palette().dark());
  return 1;
}

// ----------------------------------------------------------------------------
QRectF pqNodeEditorPort::boundingRect() const
{
  return QRectF(0, 0, 0, 0);
}
