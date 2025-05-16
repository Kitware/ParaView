// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqNodeEditorNRepresentation.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqNodeEditorLabel.h"
#include "pqNodeEditorPort.h"
#include "pqNodeEditorUtils.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqProxyWidget.h"
#include "pqRepresentation.h"

#include <QBrush>
#include <QGraphicsSceneMouseEvent>
#include <QPen>

// ----------------------------------------------------------------------------
pqNodeEditorNRepresentation::pqNodeEditorNRepresentation(
  pqRepresentation* repr, QGraphicsItem* parent)
  : pqNodeEditorNode((pqProxy*)repr, parent)
{
  auto br = this->boundingRect();
  auto adjust = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  br.adjust(adjust, adjust, -adjust, -adjust);

  // create ports
  auto iPort = new pqNodeEditorPort(
    pqNodeEditorPort::Type::INPUT, pqNodeEditorUtils::getID(proxy), 0, "", this);
  iPort->setPos(br.center().x(), br.top());
  this->iPorts = { iPort };

  auto oPort = new pqNodeEditorPort(
    pqNodeEditorPort::Type::OUTPUT, pqNodeEditorUtils::getID(proxy), 0, "", this);
  oPort->setPos(br.center().x(), br.bottom());
  this->oPorts = { oPort };

  // update output port position when needed
  QObject::connect(this, &pqNodeEditorNode::nodeResized,
    [this]()
    {
      const auto bbox = this->boundingRect();
      this->oPorts[0]->setPos(bbox.center().x(), bbox.bottom());
    });

  // what to do once properties have changed
  QObject::connect(this->proxyProperties, &pqProxyWidget::changeFinished, this,
    [this]()
    {
      this->proxy->setModifiedState(pqProxy::MODIFIED);
      this->proxyProperties->apply();
      qobject_cast<pqRepresentation*>(this->proxy)->getView()->render();
    });

  // label events
  // left click : make related source and view active
  // right click : increment verbosity
  this->getLabel()->setMousePressEventCallback(
    [this, repr](QGraphicsSceneMouseEvent* event)
    {
      if (event->button() == Qt::MouseButton::RightButton)
      {
        this->incrementVerbosity();
      }
      else if (event->button() == Qt::MouseButton::LeftButton)
      {
        auto* view = repr->getView();
        pqActiveObjects::instance().setActiveView(view);

        if (auto* dataRepr = qobject_cast<pqDataRepresentation*>(repr))
        {
          auto* port = dataRepr->getOutputPortFromInput();
          pqActiveObjects::instance().setActivePort(port);
        }
      }
    });
}

// ----------------------------------------------------------------------------
QRectF pqNodeEditorNRepresentation::boundingRect() const
{
  QRectF boundingRect = this->pqNodeEditorNode::boundingRect();
  boundingRect.setHeight(boundingRect.height() + pqNodeEditorUtils::CONSTS::PORT_RADIUS);
  return boundingRect;
}

// ----------------------------------------------------------------------------
void pqNodeEditorNRepresentation::setNodeActive(bool active)
{
  this->pqNodeEditorNode::setNodeActive(active);

  for (auto& oPort : this->getOutputPorts())
  {
    oPort->setMarkedAsVisible(active);
  }

  for (auto& iPort : this->getInputPorts())
  {
    iPort->setMarkedAsVisible(active);
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorNRepresentation::setupPaintTools(QPen& pen, QBrush& brush)
{
  pen.setWidth(pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH);
  pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_DULL_ORANGE);
  brush = pqNodeEditorUtils::CONSTS::COLOR_BASE;
}
