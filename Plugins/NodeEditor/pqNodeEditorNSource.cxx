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

#include "pqNodeEditorNSource.h"

#include "pqNodeEditorPort.h"
#include "pqNodeEditorUtils.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqProxyWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>

// ----------------------------------------------------------------------------
pqNodeEditorNSource::pqNodeEditorNSource(
  QGraphicsScene* qscene, pqPipelineSource* source, QGraphicsItem* parent)
  : pqNodeEditorNode(qscene, (pqProxy*)source, parent)
{
  this->nodeType = NodeType::SOURCE;
  this->setZValue(pqNodeEditorUtils::CONSTS::NODE_LAYER);

  // create ports ...
  QRectF br = this->boundingRect();
  constexpr auto adjust = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  br.adjust(adjust, adjust, -adjust, -adjust);
  constexpr double portRadius = pqNodeEditorUtils::CONSTS::PORT_HEIGHT * 0.5;
  const vtkIdType proxyId = pqNodeEditorUtils::getID(this->proxy);

  // ... input ports ...
  if (auto proxyAsFilter = dynamic_cast<pqPipelineFilter*>(this->proxy))
  {
    int offsetFromTop = this->labelHeight;
    for (int i = 0; i < proxyAsFilter->getNumberOfInputPorts(); i++)
    {
      auto iPort = new pqNodeEditorPort(
        pqNodeEditorPort::Type::INPUT, proxyId, i, proxyAsFilter->getInputPortName(i), this);
      iPort->setPos(br.left(), offsetFromTop + portRadius);
      this->iPorts.push_back(iPort);
      offsetFromTop +=
        pqNodeEditorUtils::CONSTS::PORT_PADDING + pqNodeEditorUtils::CONSTS::PORT_HEIGHT;
    }
  }

  // create property widgets
  QObject::connect(this->proxyProperties, &pqProxyWidget::changeFinished, this, [this]() {
    if (this->proxy->modifiedState() != pqProxy::UNINITIALIZED)
    {
      this->proxy->setModifiedState(pqProxy::MODIFIED);
    }
    return 1;
  });
  QObject::connect(this->proxy, &pqProxy::modifiedStateChanged, this, [this]() {
    bool dirty = this->proxy->modifiedState() == pqProxy::ModifiedState::MODIFIED ||
      this->proxy->modifiedState() == pqProxy::ModifiedState::UNINITIALIZED;
    this->setNodeState(dirty ? NodeState::DIRTY : NodeState::NORMAL);
    return 1;
  });
}

// ----------------------------------------------------------------------------
void pqNodeEditorNSource::setupPaintTools(QPen& pen, QBrush& brush)
{
  pen.setWidth(pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH);
  if (this->nodeActive)
  {
    pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_HIGHLIGHT);
  }
  else
  {
    pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_CONSTRAST);
  }

  brush = this->nodeState == NodeState::DIRTY ? pqNodeEditorUtils::CONSTS::COLOR_BASE_GREEN
                                              : pqNodeEditorUtils::CONSTS::COLOR_BASE;
}
