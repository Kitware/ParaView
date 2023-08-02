// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2021 Jonas Lukasczyk
// SPDX-License-Identifier: BSD-3-Clause

#include "pqNodeEditorEdge.h"

#include "pqNodeEditorNode.h"
#include "pqNodeEditorPort.h"
#include "pqNodeEditorUtils.h"

#include <pqPipelineSource.h>

#include <vtkSMProxy.h>

#include <QApplication>
#include <QGraphicsPathItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <cmath>
#include <sstream>

// -----------------------------------------------------------------------------
pqNodeEditorEdge::pqNodeEditorEdge(pqNodeEditorNode* producerNode, int outputPortIdx,
  pqNodeEditorNode* consumerNode, int inputPortIdx, Type edgeType, QGraphicsItem* parent)
  : QGraphicsItem(parent)
  , type(edgeType)
  , edgeOverlay(new QGraphicsPathItem(parent))
  , producer(producerNode)
  , producerOutputPortIdx(outputPortIdx)
  , consumer(consumerNode)
  , consumerInputPortIdx(inputPortIdx)
{
  this->connect(
    this->producer, &pqNodeEditorNode::nodeMoved, this, &pqNodeEditorEdge::updatePoints);
  this->connect(
    this->consumer, &pqNodeEditorNode::nodeMoved, this, &pqNodeEditorEdge::updatePoints);
  this->connect(
    this->producer, &pqNodeEditorNode::nodeResized, this, &pqNodeEditorEdge::updatePoints);
  this->connect(
    this->consumer, &pqNodeEditorNode::nodeResized, this, &pqNodeEditorEdge::updatePoints);

  this->setZValue(pqNodeEditorUtils::CONSTS::EDGE_LAYER);
  this->setAcceptedMouseButtons(Qt::NoButton);
  this->edgeOverlay->setVisible(this->isVisible());
  this->edgeOverlay->setPath(this->path);
  this->edgeOverlay->setZValue(pqNodeEditorUtils::CONSTS::MAX_LAYER);
  this->edgeOverlay->setOpacity(0.15);
  this->edgeOverlay->setAcceptedMouseButtons(Qt::NoButton);

  this->updatePoints();
}

// -----------------------------------------------------------------------------
pqNodeEditorEdge::~pqNodeEditorEdge()
{
  if (this->edgeOverlay->parentItem() == nullptr)
  {
    delete this->edgeOverlay;
  }
}

// -----------------------------------------------------------------------------
void pqNodeEditorEdge::setType(Type _type)
{
  this->type = _type;
  this->updatePoints();
}

// -----------------------------------------------------------------------------
std::string pqNodeEditorEdge::toString()
{
  std::stringstream ss;

  ss << pqNodeEditorUtils::getLabel(this->producer->getProxy()) << "["
     << this->producerOutputPortIdx << "]"
     << " -> " << pqNodeEditorUtils::getLabel(this->consumer->getProxy()) << "["
     << this->consumerInputPortIdx << "]";
  return ss.str();
}

// -----------------------------------------------------------------------------
QRectF pqNodeEditorEdge::boundingRect() const
{
  constexpr qreal BB_MARGIN =
    pqNodeEditorUtils::CONSTS::EDGE_WIDTH + pqNodeEditorUtils::CONSTS::EDGE_OUTLINE;

  return this->path.boundingRect().adjusted(-BB_MARGIN, -BB_MARGIN, BB_MARGIN, BB_MARGIN);
}

// ----------------------------------------------------------------------------
QVariant pqNodeEditorEdge::itemChange(GraphicsItemChange change, const QVariant& value)
{
  this->edgeOverlay->setVisible(this->isVisible());
  return QGraphicsItem::itemChange(change, value);
}

// -----------------------------------------------------------------------------
int pqNodeEditorEdge::updatePoints()
{
  this->prepareGeometryChange();

  // clear path
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
  this->path = QPainterPath();
#else
  this->path.clear();
#endif

  // compute path
  const auto oPoint =
    this->producer->getOutputPorts()[this->producerOutputPortIdx]->getConnectionPoint(this);
  const auto iPoint =
    this->consumer->getInputPorts()[this->consumerInputPortIdx]->getConnectionPoint(this);
  const auto xo = oPoint.x();
  const auto yo = oPoint.y();
  const auto xi = iPoint.x();
  const auto yi = iPoint.y();
  path.moveTo(oPoint);
  if (this->type == Type::PIPELINE)
  {
    const auto dx = std::abs(xi - xo) * 0.5;
    path.cubicTo(xo + dx, yo, xi - dx, yi, xi, yi);
  }
  else
  {
    const auto dy = std::abs(yi - yo) * 0.5;
    path.cubicTo(xo, yo + dy, xi, yi - dy, xi, yi);
  }

  this->edgeOverlay->setPath(this->path);
  this->edgeOverlay->update();

  return 1;
}

// -----------------------------------------------------------------------------
void pqNodeEditorEdge::paint(
  QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  // Pre-allocate pens for faster rendering
  static const QPen edgePipelinePen(pqNodeEditorUtils::CONSTS::COLOR_HIGHLIGHT,
    pqNodeEditorUtils::CONSTS::EDGE_WIDTH, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  static const QPen activeViewPen(pqNodeEditorUtils::CONSTS::COLOR_BASE_ORANGE,
    pqNodeEditorUtils::CONSTS::EDGE_WIDTH, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
  static const QPen unfocusedPen(pqNodeEditorUtils::CONSTS::COLOR_DULL_ORANGE,
    pqNodeEditorUtils::CONSTS::EDGE_WIDTH, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);

  if (this->type == Type::PIPELINE)
  {
    this->edgeOverlay->setPen(edgePipelinePen);
    painter->setPen(edgePipelinePen);
  }
  else if (this->consumer->isNodeActive())
  {
    this->edgeOverlay->setPen(activeViewPen);
    painter->setPen(activeViewPen);
  }
  else
  {
    this->edgeOverlay->setPen(unfocusedPen);
    painter->setPen(unfocusedPen);
  }

  painter->drawPath(this->path);
  this->edgeOverlay->paint(painter, option, widget);
}
