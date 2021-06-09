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

#include "pqNodeEditorEdge.h"

#include "pqNodeEditorNode.h"
#include "pqNodeEditorPort.h"
#include "pqNodeEditorScene.h"
#include "pqNodeEditorUtils.h"

#include <pqPipelineSource.h>
#include <vtkSMProxy.h>

#include <QApplication>
#include <QGraphicsScene>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <sstream>

// ----------------------------------------------------------------------------
pqNodeEditorEdge::pqNodeEditorEdge(QGraphicsScene* scene, pqNodeEditorNode* producer,
  int producerOutputPortIdx, pqNodeEditorNode* consumer, int consumerInputPortIdx, EdgeType type,
  QGraphicsItem* parent)
  : QGraphicsPathItem(parent)
  , scene(scene)
  , producer(producer)
  , producerOutputPortIdx(producerOutputPortIdx)
  , consumer(consumer)
  , consumerInputPortIdx(consumerInputPortIdx)
  , type(type)
{
  this->connect(
    this->producer, &pqNodeEditorNode::nodeMoved, this, &pqNodeEditorEdge::updatePoints);
  this->connect(
    this->consumer, &pqNodeEditorNode::nodeMoved, this, &pqNodeEditorEdge::updatePoints);
  this->connect(
    this->producer, &pqNodeEditorNode::nodeResized, this, &pqNodeEditorEdge::updatePoints);
  this->connect(
    this->consumer, &pqNodeEditorNode::nodeResized, this, &pqNodeEditorEdge::updatePoints);

  this->setAcceptedMouseButtons(Qt::NoButton);
  this->setZValue(type != EdgeType::NORMAL ? pqNodeEditorUtils::CONSTS::FOREGROUND_LAYER
                                           : pqNodeEditorUtils::CONSTS::EDGE_LAYER);

  this->updatePoints();

  this->scene->addItem(this);
}

// ----------------------------------------------------------------------------
pqNodeEditorEdge::~pqNodeEditorEdge()
{
  this->scene->removeItem(this);
}

// ----------------------------------------------------------------------------
void pqNodeEditorEdge::setType(EdgeType _type)
{
  this->type = _type;
  this->update(this->boundingRect());
}

// ----------------------------------------------------------------------------
std::string pqNodeEditorEdge::toString()
{
  std::stringstream ss;

  ss << pqNodeEditorUtils::getLabel(this->producer->getProxy()) << "["
     << this->producerOutputPortIdx << "]"
     << " -> " << pqNodeEditorUtils::getLabel(this->consumer->getProxy()) << "["
     << this->consumerInputPortIdx << "]";
  return ss.str();
}

// ----------------------------------------------------------------------------
QRectF pqNodeEditorEdge::boundingRect() const
{
  qreal x0 = std::min(this->oPoint.x(), std::min(this->cPoint.x(), this->iPoint.x()));
  qreal y0 = std::min(this->oPoint.y(), std::min(this->cPoint.y(), this->iPoint.y()));
  qreal x1 = std::max(this->oPoint.x(), std::max(this->cPoint.x(), this->iPoint.x()));
  qreal y1 = std::max(this->oPoint.y(), std::max(this->cPoint.y(), this->iPoint.y()));

  constexpr qreal BB_MARGIN = 4.0;
  return QRectF(x0, y0, x1 - x0, y1 - y0).adjusted(-BB_MARGIN, -BB_MARGIN, BB_MARGIN, BB_MARGIN);
}

// ----------------------------------------------------------------------------
int pqNodeEditorEdge::updatePoints()
{
  auto nProducerOutputPorts = this->producer->getOutputPorts().size();

  this->prepareGeometryChange();

  this->oPoint = QGraphicsItem::mapFromItem(
    this->producer->getOutputPorts()[this->producerOutputPortIdx]->getDisc(), 0, 0);
  this->iPoint = QGraphicsItem::mapFromItem(
    this->consumer->getInputPorts()[this->consumerInputPortIdx]->getDisc(), 0, 0);

  this->cPoint = this->oPoint;
  if (this->type == EdgeType::VIEW)
  {
    const auto& bb = this->producer->boundingRect();
    QPointF bottomRight =
      bb.bottomRight() + QPointF((nProducerOutputPorts - this->producerOutputPortIdx) * 15, 0);
    QPointF iPort = this->mapToItem(this->producer, this->iPoint);
    if (bottomRight.y() < iPort.y())
    {
      this->cPoint = this->mapFromItem(this->producer, bottomRight);
    }
  }

  return 1;
}

// ----------------------------------------------------------------------------
void pqNodeEditorEdge::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  QLineF line(this->oPoint, this->iPoint);

  QPainterPath path;
  path.moveTo(this->oPoint);

  if (this->type == EdgeType::NORMAL)
  {
    path.lineTo(this->iPoint);
  }
  else
  {
    path.quadTo(this->cPoint.x(), this->oPoint.y(), this->cPoint.x(), this->oPoint.y());
    path.lineTo(this->cPoint);
    auto xt = 0.5 * (this->cPoint.x() - this->iPoint.x());
    auto yt = 0.5 * (this->iPoint.y() - this->cPoint.y());
    path.cubicTo(this->cPoint.x(), this->cPoint.y() + yt, this->iPoint.x() + xt,
      this->cPoint.y() + yt, this->iPoint.x(), this->iPoint.y());
  }

  const QBrush& penColor = (this->type == EdgeType::NORMAL) ? QApplication::palette().highlight()
                                                            : QApplication::palette().linkVisited();
  Qt::PenStyle penStyle = (this->type == EdgeType::NORMAL) ? Qt::SolidLine : Qt::DashDotLine;
  painter->setPen(
    QPen(penColor, pqNodeEditorUtils::CONSTS::EDGE_WIDTH, penStyle, Qt::RoundCap, Qt::RoundJoin));
  painter->drawPath(path);
}
