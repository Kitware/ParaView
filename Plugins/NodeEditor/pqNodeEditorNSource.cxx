// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqNodeEditorNSource.h"

#include "pqActiveObjects.h"
#include "pqDeleteReaction.h"
#include "pqNodeEditorLabel.h"
#include "pqNodeEditorPort.h"
#include "pqNodeEditorUtils.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqProxyWidget.h"

#include <QBrush>
#include <QGraphicsSceneMouseEvent>
#include <QPen>

// ----------------------------------------------------------------------------
pqNodeEditorNSource::pqNodeEditorNSource(pqPipelineSource* source, QGraphicsItem* parent)
  : pqNodeEditorNode((pqProxy*)source, parent)
{
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

  // ... and output ports
  int offsetFromTop = this->labelHeight;
  for (int i = 0; i < source->getNumberOfOutputPorts(); i++)
  {
    auto oPort = new pqNodeEditorPort(
      pqNodeEditorPort::Type::OUTPUT, proxyId, i, source->getOutputPort(i)->getPortName(), this);
    oPort->setPos(br.right(), offsetFromTop + portRadius);
    this->oPorts.push_back(oPort);
    offsetFromTop +=
      pqNodeEditorUtils::CONSTS::PORT_PADDING + pqNodeEditorUtils::CONSTS::PORT_HEIGHT;
  }

  // what to do once properties have changed
  QObject::connect(this->proxyProperties, &pqProxyWidget::changeFinished, this,
    [this]()
    {
      if (this->proxy->modifiedState() != pqProxy::UNINITIALIZED)
      {
        this->proxy->setModifiedState(pqProxy::MODIFIED);
      }
      return 1;
    });
  QObject::connect(this->proxy, &pqProxy::modifiedStateChanged, this,
    [this]()
    {
      bool dirty = this->proxy->modifiedState() == pqProxy::ModifiedState::MODIFIED ||
        this->proxy->modifiedState() == pqProxy::ModifiedState::UNINITIALIZED;
      this->setNodeState(dirty ? NodeState::DIRTY : NodeState::NORMAL);
      return 1;
    });

  // handle label events
  // right click : increment verbosity
  // left click : select node
  // left + ctrl : add to selection
  // middle click : delete node
  this->getLabel()->setMousePressEventCallback(
    [this](QGraphicsSceneMouseEvent* event)
    {
      if (event->button() == Qt::MouseButton::RightButton)
      {
        this->incrementVerbosity();
      }
      else if (event->button() == Qt::MouseButton::LeftButton)
      {
        auto* activeObjects = &pqActiveObjects::instance();
        if (event->modifiers() == Qt::NoModifier)
        {
          activeObjects->setSelection({ this->proxy }, this->proxy);
        }
        else if (event->modifiers() == Qt::ControlModifier)
        {
          auto sel = activeObjects->selection();
          pqServerManagerModelItem* newActive = proxy;
          if (sel.count(proxy))
          {
            sel.removeAll(proxy);
            newActive = sel.empty() ? nullptr : sel[0];
          }
          else
          {
            sel.push_back(proxy);
          }
          activeObjects->setSelection(sel, newActive);
        }
      }
      else if (event->button() == Qt::MouseButton::MiddleButton)
      {
        pqDeleteReaction::deleteSources({ proxy });
        // Important so no further events are processed on the destroyed widget
        event->accept();
      }
    });

  // input port label events, if any
  // middle click : clear all incoming connections
  // left click + ctrl : set all incoming selected ports as input
  if (dynamic_cast<pqPipelineFilter*>(this->proxy))
  {
    int index = 0;
    for (auto* inPort : this->iPorts)
    {
      inPort->getLabel()->setMousePressEventCallback(
        [this, index](QGraphicsSceneMouseEvent* event)
        {
          if (event->button() == Qt::MouseButton::MiddleButton)
          {
            Q_EMIT this->inputPortClicked(static_cast<int>(index), true);
          }
          else if (event->button() == Qt::MouseButton::LeftButton &&
            (event->modifiers() & Qt::ControlModifier))
          {
            Q_EMIT this->inputPortClicked(static_cast<int>(index), false);
          }
        });

      index++;
    }
  }

  // output port label events
  // left click: set output port as active selection
  // left click + ctrl: add output port to active selection
  // left click + shift: toggle visibility in active view
  // left click + ctrl + shift: hide all but port in active view
  int index = 0;
  for (auto* outPort : this->oPorts)
  {
    outPort->getLabel()->setMousePressEventCallback(
      [this, source, index](QGraphicsSceneMouseEvent* event)
      {
        if (event->button() == Qt::MouseButton::LeftButton)
        {
          auto* activeObjects = &pqActiveObjects::instance();
          auto* portProxy = source->getOutputPort(index);

          if (event->modifiers() & Qt::ShiftModifier)
          {
            const bool hideAll = (event->modifiers() & Qt::ControlModifier);
            Q_EMIT this->outputPortClicked(portProxy, hideAll);
          }
          else if (event->modifiers() == Qt::NoModifier)
          {
            activeObjects->setActivePort(portProxy);
          }
          else if (event->modifiers() == Qt::ControlModifier)
          {
            pqProxySelection sel = activeObjects->selection();
            sel.push_back(portProxy);
            activeObjects->setSelection(sel, portProxy);
          }
        }
      });
  }
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
