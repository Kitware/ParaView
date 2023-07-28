// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2021 Jonas Lukasczyk
// SPDX-License-Identifier: BSD-3-Clause

#include "pqNodeEditorPort.h"

#include "pqNodeEditorLabel.h"
#include "pqNodeEditorScene.h"
#include "pqNodeEditorUtils.h"

// qt includes
#include <QApplication>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QPalette>
#include <QPen>

#include <cassert>
#include <iostream>

// ----------------------------------------------------------------------------
namespace details
{
/**
 * Custom class for handling the port label with the right font and mouse shape.
 */
class PortLabel : public pqNodeEditorLabel
{
public:
  PortLabel(QString label, QGraphicsItem* parent)
    : pqNodeEditorLabel(label, parent, true){};

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

class PortDisc : public QGraphicsEllipseItem
{
public:
  PortDisc(qreal x, qreal y, qreal w, qreal h, QGraphicsItem* parent)
    : QGraphicsEllipseItem(x, y, w, h, parent)
  {
    this->setCursor(Qt::PointingHandCursor);
  }

  ~PortDisc() override { this->deleteLine(); }

protected:
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override
  {
    this->deleteLine();

    QPointF p0 = this->boundingRect().center();
    QPointF p1 = event->pos();
    currentLine = new QGraphicsLineItem(p0.x(), p0.y(), p1.x(), p1.y(), this);
    currentLine->setPen(
      QPen(QApplication::palette().highlight(), pqNodeEditorUtils::CONSTS::EDGE_WIDTH));

    currentType = dynamic_cast<pqNodeEditorPort*>(this->parentItem())->getPortType();
  }

  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
  {
    if (this->currentLine)
    {
      auto prev = currentLine->line().p1();
      const QPointF& curr = event->pos();
      currentLine->setLine(prev.x(), prev.y(), curr.x(), curr.y());
    }
  }

  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
  {
    this->deleteLine();

    auto* scene = dynamic_cast<pqNodeEditorScene*>(this->scene());
    assert(scene);
    auto items = scene->items(event->scenePos());
    for (QGraphicsItem* item : items)
    {
      if (auto* port = dynamic_cast<PortDisc*>(item))
      {
        auto* fromPort = dynamic_cast<pqNodeEditorPort*>(this->parentItem());
        auto* toPort = dynamic_cast<pqNodeEditorPort*>(port->parentItem());
        // XXX: Here we prevent a filter to connect its output to its own input.
        // This is actually a bit more complex than that ... we want to prevent any kind
        // of cycles in our graph. This has to be improved at some point. See #20726
        if (fromPort->getProxyId() == toPort->getProxyId())
        {
          continue;
        }

        if (toPort->getPortType() == pqNodeEditorPort::Type::INPUT &&
          fromPort->getPortType() == pqNodeEditorPort::Type::OUTPUT)
        {
          Q_EMIT scene->edgeDragAndDropRelease(fromPort->getProxyId(), fromPort->getPortNumber(),
            toPort->getProxyId(), toPort->getPortNumber());
        }
        else if (toPort->getPortType() == pqNodeEditorPort::Type::OUTPUT &&
          fromPort->getPortType() == pqNodeEditorPort::Type::INPUT)
        {
          Q_EMIT scene->edgeDragAndDropRelease(toPort->getProxyId(), toPort->getPortNumber(),
            fromPort->getProxyId(), fromPort->getPortNumber());
        }
        break;
      }
    }
  }

private:
  void deleteLine()
  {
    if (this->currentLine)
    {
      this->scene()->removeItem(this->currentLine);
      delete this->currentLine;
      this->currentLine = nullptr;
    }
  }

  QGraphicsLineItem* currentLine = nullptr;
  pqNodeEditorPort::Type currentType;
};
}

// ----------------------------------------------------------------------------
using namespace pqNodeEditorUtils::CONSTS;
pqNodeEditorPort::pqNodeEditorPort(
  Type type, vtkIdType id, int port, QString name, QGraphicsItem* parent)
  : QGraphicsItem(parent)
  , disc(
      new details::PortDisc(-PORT_RADIUS, -PORT_RADIUS, 2.0 * PORT_RADIUS, 2.0 * PORT_RADIUS, this))
  , label(new details::PortLabel(name, this))
  , proxyId(id)
  , portNumber(port)
  , portType(type)
{
  // Set the port label at the right of the actual port for input ports and at the left
  // for output ports. Use an offset to not make the label too close of the node border.
  qreal xpos = PORT_RADIUS + PORT_LABEL_OFFSET;
  if (type == Type::OUTPUT)
  {
    xpos = -xpos - this->label->boundingRect().width();
  }
  this->label->setPos(xpos, -0.5 * this->label->boundingRect().height());
  this->disc->setBrush(pqNodeEditorUtils::CONSTS::COLOR_BASE_DEEP);

  this->setMarkedAsSelected(false);
  this->setMarkedAsVisible(false);
}

// ----------------------------------------------------------------------------
int pqNodeEditorPort::setMarkedAsSelected(bool selected)
{
  const QBrush& brush = selected ? pqNodeEditorUtils::CONSTS::COLOR_HIGHLIGHT
                                 : pqNodeEditorUtils::CONSTS::COLOR_CONSTRAST;
  this->disc->setPen(QPen(brush, pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH));
  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorPort::setMarkedAsVisible(bool visible)
{
  this->disc->setBrush(visible ? pqNodeEditorUtils::CONSTS::COLOR_BASE_ORANGE
                               : pqNodeEditorUtils::CONSTS::COLOR_BASE_DEEP);
  return 1;
}

// ----------------------------------------------------------------------------
QPointF pqNodeEditorPort::getConnectionPoint(QGraphicsItem* item) const
{
  return this->disc->mapToItem(item, QPointF(0, 0));
}

// ----------------------------------------------------------------------------
QRectF pqNodeEditorPort::boundingRect() const
{
  return QRectF(0, 0, 0, 0);
}
