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
#include "pqNodeEditorUtils.h"

#include <pqPipelineSource.h>

#include <vtkSMProxy.h>

#include <QApplication>
#include <QGraphicsScene>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <cmath>
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
    const auto dx = std::abs(xi - xo) * 0.5;
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

  this->oPoint =
    this->producer->getOutputPorts()[this->producerOutputPortIdx]->getConnectionPoint(this);
  this->iPoint =
    this->consumer->getInputPorts()[this->consumerInputPortIdx]->getConnectionPoint(this);

  if (this->type == Type::VIEW)
  {
    this->cPoint = this->mapFromItem(this->producer, this->producer->boundingRect().bottomRight());
    // slighly pad the point so it doesnt get too close of the node
    this->cPoint += QPointF(10, 0);
  }

  // compute path
  {
    const auto xo = this->oPoint.x();
    const auto yo = this->oPoint.y();
    const auto xi = this->iPoint.x();
    const auto yi = this->iPoint.y();
    const auto xc = this->cPoint.x();
    const auto yc = this->cPoint.y();

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    this->path = QPainterPath();
#else
    this->path.clear();
#endif
    path.moveTo(this->oPoint);

    if (this->type == Type::PIPELINE)
    {
      const auto dx = std::abs(xi - xo) * 0.5;
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
  // Pre-allocate pens for faster rendering
  static const QPen edgeOutlinePen(pqNodeEditorUtils::CONSTS::COLOR_BASE,
    pqNodeEditorUtils::CONSTS::EDGE_WIDTH + pqNodeEditorUtils::CONSTS::EDGE_OUTLINE, Qt::SolidLine,
    Qt::RoundCap, Qt::RoundJoin);
  static const QPen edgePipelinePen(pqNodeEditorUtils::CONSTS::COLOR_HIGHLIGHT,
    pqNodeEditorUtils::CONSTS::EDGE_WIDTH, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  static const QPen activeViewPen(pqNodeEditorUtils::CONSTS::COLOR_BASE_ORANGE,
    pqNodeEditorUtils::CONSTS::EDGE_WIDTH, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
  static const QPen unfocusedPen(pqNodeEditorUtils::CONSTS::COLOR_DULL_ORANGE,
    pqNodeEditorUtils::CONSTS::EDGE_WIDTH, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);

  if (this->type == Type::PIPELINE)
  {
    painter->setPen(edgeOutlinePen);
    painter->drawPath(this->path);
    painter->setPen(edgePipelinePen);
  }
  else if (this->consumer->getOutlineStyle() == pqNodeEditorNode::OutlineStyle::SELECTED_VIEW)
  {
    painter->setPen(activeViewPen);
  }
  else
  {
    painter->setPen(unfocusedPen);
  }
  painter->drawPath(this->path);
}
