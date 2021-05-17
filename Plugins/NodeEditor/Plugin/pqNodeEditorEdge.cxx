/*=========================================================================
Copyright (c) 2021, Jonas Lukasczyk
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

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
  int producerOutputPortIdx, pqNodeEditorNode* consumer, int consumerInputPortIdx, int type,
  QGraphicsItem* parent)
  : QObject()
  , QGraphicsPathItem(parent)
  , scene(scene)
  , producer(producer)
  , producerOutputPortIdx(producerOutputPortIdx)
  , consumer(consumer)
  , consumerInputPortIdx(consumerInputPortIdx)
  , type(type)
{
  pqNodeEditorUtils::log("  +pqNodeEditorEdge: " + this->toString());

  this->connect(
    this->producer, &pqNodeEditorNode::nodeMoved, this, &pqNodeEditorEdge::updatePoints);
  this->connect(
    this->consumer, &pqNodeEditorNode::nodeMoved, this, &pqNodeEditorEdge::updatePoints);
  this->connect(
    this->producer, &pqNodeEditorNode::nodeResized, this, &pqNodeEditorEdge::updatePoints);
  this->connect(
    this->consumer, &pqNodeEditorNode::nodeResized, this, &pqNodeEditorEdge::updatePoints);

  this->setAcceptedMouseButtons(Qt::NoButton);
  this->setZValue(type > 0 ? 3 : 2);

  this->updatePoints();

  this->scene->addItem(this);
}

// ----------------------------------------------------------------------------
pqNodeEditorEdge::~pqNodeEditorEdge()
{
  pqNodeEditorUtils::log("  -pqNodeEditorEdge: " + this->toString());
  this->scene->removeItem(this);
}

// ----------------------------------------------------------------------------
int pqNodeEditorEdge::setType(int type)
{
  this->type = type;
  this->update(this->boundingRect());
  return this->type;
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

  const qreal extra = 4.0;
  return QRectF(x0, y0, x1 - x0, y1 - y0).adjusted(-extra, -extra, extra, extra);
}

// ----------------------------------------------------------------------------
int pqNodeEditorEdge::updatePoints()
{

  auto nProducerOutputPorts = this->producer->getOutputPorts().size();
  auto b = this->producer->boundingRect();

  this->prepareGeometryChange();

  this->oPoint = QGraphicsItem::mapFromItem(
    this->producer->getOutputPorts()[this->producerOutputPortIdx]->getDisc(), 0, 0);
  this->iPoint = QGraphicsItem::mapFromItem(
    this->consumer->getInputPorts()[this->consumerInputPortIdx]->getDisc(), 0, 0);

  this->cPoint =
    this->type == 0 ? this->oPoint : this->mapFromItem(this->producer, b.bottomRight()) +
      QPointF((nProducerOutputPorts - this->producerOutputPortIdx) * 15, 0);

  return 1;
}

// ----------------------------------------------------------------------------
void pqNodeEditorEdge::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
  QLineF line(this->oPoint, this->iPoint);

  QPainterPath path;
  path.moveTo(this->oPoint);

  if (this->type == 0)
  {
    path.lineTo(this->iPoint);
  }
  else
  {
    path.quadTo(this->cPoint.x(), this->oPoint.y(), this->cPoint.x(), this->oPoint.y() + 40);
    path.lineTo(this->cPoint);
    auto xt = 0.5 * (this->cPoint.x() - this->iPoint.x());
    auto yt = 0.5 * (this->iPoint.y() - this->cPoint.y());
    path.cubicTo(this->cPoint.x(), this->cPoint.y() + yt, this->iPoint.x() + xt,
      this->cPoint.y() + yt, this->iPoint.x(), this->iPoint.y());
  }

  painter->setPen(QPen(this->type == 0 ? QApplication::palette().highlight().color()
                                       : pqNodeEditorUtils::CONSTS::COLOR_ORANGE,
    pqNodeEditorUtils::CONSTS::EDGE_WIDTH, this->type == 0 ? Qt::SolidLine : Qt::DashDotLine,
    Qt::RoundCap, Qt::RoundJoin));
  painter->drawPath(path);
}
