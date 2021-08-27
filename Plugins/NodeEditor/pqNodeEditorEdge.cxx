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

// -----------------------------------------------------------------------------
pqNodeEditorEdge::pqNodeEditorEdge(QGraphicsScene* qscene, pqNodeEditorNode* producerNode,
  int outputPortIdx, pqNodeEditorNode* consumerNode, int inputPortIdx, Type edgeType,
  QGraphicsItem* parent)
  : QGraphicsPathItem(parent)
  , scene(qscene)
  , type(edgeType)
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

  this->setAcceptedMouseButtons(Qt::NoButton);
  this->setZValue(type != Type::PIPELINE ? pqNodeEditorUtils::CONSTS::FOREGROUND_LAYER
                                         : pqNodeEditorUtils::CONSTS::EDGE_LAYER);

  this->updatePoints();

  this->scene->addItem(this);
}

// -----------------------------------------------------------------------------
pqNodeEditorEdge::~pqNodeEditorEdge()
{
  this->scene->removeItem(this);
}

// -----------------------------------------------------------------------------
void pqNodeEditorEdge::setType(Type _type)
{
  this->type = _type;
  // to update path
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
  const auto xo = this->oPoint.x();
  const auto yo = this->oPoint.y();
  const auto xi = this->iPoint.x();
  const auto yi = this->iPoint.y();

  constexpr qreal BB_MARGIN =
    pqNodeEditorUtils::CONSTS::EDGE_WIDTH + pqNodeEditorUtils::CONSTS::EDGE_OUTLINE;

  if (this->type == Type::VIEW)
  {
    qreal x0 = std::min(xo, xi);
    qreal y0 = std::min(yo, yi - 60);
    qreal x1 = std::max(xo, std::max(this->cPoint.x(), xi));
    qreal y1 = std::max(yo, std::max(this->cPoint.y(), yi));

    return QRectF(x0, y0, x1 - x0, y1 - y0).adjusted(-BB_MARGIN, -BB_MARGIN, BB_MARGIN, BB_MARGIN);
  }
  else
  {
    const auto dx = std::abs(xi - xo) / 2.0;
    qreal x0 = std::min(xo, xi - dx);
    qreal y0 = std::min(yo, yi);
    qreal x1 = std::max(xi, xo + dx);
    qreal y1 = std::max(yo, yi);

    return QRectF(x0, y0, x1 - x0, y1 - y0).adjusted(-BB_MARGIN, -BB_MARGIN, BB_MARGIN, BB_MARGIN);
  }
}

// -----------------------------------------------------------------------------
int pqNodeEditorEdge::updatePoints()
{
  this->prepareGeometryChange();

  this->oPoint = QGraphicsItem::mapFromItem(
    this->producer->getOutputPorts()[this->producerOutputPortIdx]->getDisc(), 0, 0);
  this->iPoint = QGraphicsItem::mapFromItem(
    this->consumer->getInputPorts()[this->consumerInputPortIdx]->getDisc(), 0, 0);

  if (this->type == Type::VIEW)
  {
    this->cPoint = this->oPoint;

    auto nProducerOutputPorts = this->producer->getOutputPorts().size();
    const auto& bb = this->producer->boundingRect();
    this->cPoint = this->mapFromItem(this->producer, bb.bottomRight());

    const auto dx = (nProducerOutputPorts - this->producerOutputPortIdx) * 10.0;
    this->cPoint.setX(this->cPoint.x() + dx);
  }

  // compute path
  {
    const auto xo = this->oPoint.x();
    const auto yo = this->oPoint.y();
    const auto xi = this->iPoint.x();
    const auto yi = this->iPoint.y();
    const auto xc = this->cPoint.x();
    const auto yc = this->cPoint.y();

    this->path = QPainterPath();
    path.moveTo(this->oPoint);

    if (this->type == Type::PIPELINE)
    {
      const auto dx = std::abs(xi - xo) / 2.0;
      path.cubicTo(xo + dx, yo, xi - dx, yi, xi, yi);
    }
    else
    {
      if (yc < yi)
      {
        path.quadTo(xc, yo, xc, yo + xc - xo);
        path.lineTo(xc, yc);
        const auto dy = 0.5 * (yi - yc);
        path.cubicTo(xc, yc + dy, xi, yi - dy, xi, yi);
      }
      else
      {
        path.cubicTo(xo, yo, xi, yi - 60, xi, yi);
      }
    }
  }

  return 1;
}

// -----------------------------------------------------------------------------
void pqNodeEditorEdge::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  const bool isPipelineEdge = this->type == Type::PIPELINE;
  const bool consumerIsActiveView =
    this->consumer->getOutlineStyle() == pqNodeEditorNode::OutlineStyle::SELECTED_VIEW;

  if (isPipelineEdge)
  {
    static const QPen edgeOutlinePen(QApplication::palette().window(),
      pqNodeEditorUtils::CONSTS::EDGE_WIDTH + pqNodeEditorUtils::CONSTS::EDGE_OUTLINE,
      Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    painter->setPen(edgeOutlinePen);
    painter->drawPath(this->path);
  }

  painter->setPen(
    QPen(isPipelineEdge ? QApplication::palette().highlight()
                        : consumerIsActiveView ? pqNodeEditorUtils::CONSTS::COLOR_DARK_ORANGE
                                               : QApplication::palette().linkVisited(),
      pqNodeEditorUtils::CONSTS::EDGE_WIDTH, isPipelineEdge ? Qt::SolidLine : Qt::DashDotLine,
      Qt::RoundCap, Qt::RoundJoin));
  painter->drawPath(this->path);
}
