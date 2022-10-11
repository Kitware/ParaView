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
pqNodeEditorNView::pqNodeEditorNView(QGraphicsScene* qscene, pqView* view, QGraphicsItem* parent)
  : pqNodeEditorNode(qscene, (pqProxy*)view, parent)
{
  this->nodeType = NodeType::VIEW;
  this->setZValue(pqNodeEditorUtils::CONSTS::VIEW_NODE_LAYER);

  auto br = this->boundingRect();
  auto adjust = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  br.adjust(adjust, adjust, -adjust, -adjust);

  // create port
  auto iPort = new pqNodeEditorPort(
    pqNodeEditorPort::Type::INPUT, pqNodeEditorUtils::getID(proxy), 0, "", this);
  iPort->setPos(br.center().x(), br.top());
  this->iPorts.push_back(iPort);

  // create property widgets
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
