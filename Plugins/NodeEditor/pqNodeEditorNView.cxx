// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqNodeEditorNView.h"

#include "pqActiveObjects.h"
#include "pqNodeEditorLabel.h"
#include "pqNodeEditorPort.h"
#include "pqNodeEditorUtils.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqProxyWidget.h"
#include "pqView.h"

#include <QBrush>
#include <QGraphicsSceneMouseEvent>
#include <QPen>

// ----------------------------------------------------------------------------
pqNodeEditorNView::pqNodeEditorNView(pqView* view, QGraphicsItem* parent)
  : pqNodeEditorNode((pqProxy*)view, parent)
{
  auto br = this->boundingRect();
  auto adjust = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  br.adjust(adjust, adjust, -adjust, -adjust);

  // create port
  auto iPort = new pqNodeEditorPort(
    pqNodeEditorPort::Type::INPUT, pqNodeEditorUtils::getID(proxy), 0, "", this);
  iPort->setPos(br.center().x(), br.top());
  this->iPorts.push_back(iPort);

  // what to do once properties have changed
  QObject::connect(this->proxyProperties, &pqProxyWidget::changeFinished, this, [this]() {
    this->proxy->setModifiedState(pqProxy::MODIFIED);
    this->proxyProperties->apply();
    qobject_cast<pqView*>(this->proxy)->render();
  });

  // label events
  // left click : select as active view
  // right click : increment verbosity
  this->getLabel()->setMousePressEventCallback([this, view](QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::MouseButton::RightButton)
    {
      this->incrementVerbosity();
    }
    else if (event->button() == Qt::MouseButton::LeftButton)
    {
      pqActiveObjects::instance().setActiveView(view);
    }
  });
}

// ----------------------------------------------------------------------------
void pqNodeEditorNView::setNodeActive(bool active)
{
  this->pqNodeEditorNode::setNodeActive(active);

  for (auto& iPort : this->getInputPorts())
  {
    iPort->setMarkedAsVisible(active);
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorNView::setupPaintTools(QPen& pen, QBrush& brush)
{
  pen.setWidth(pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH);
  if (this->nodeActive)
  {
    pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_BASE_ORANGE);
  }
  else
  {
    pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_CONSTRAST);
  }

  brush = pqNodeEditorUtils::CONSTS::COLOR_BASE;
}
