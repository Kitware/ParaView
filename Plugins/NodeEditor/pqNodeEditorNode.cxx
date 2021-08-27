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

#include "pqNodeEditorNode.h"

#include "pqNodeEditorPort.h"
#include "pqNodeEditorUtils.h"

#include <pqDataRepresentation.h>
#include <pqOutputPort.h>
#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqProxiesWidget.h>
#include <pqProxyWidget.h>
#include <pqServerManagerModel.h>
#include <pqView.h>

#include <vtkSMPropertyGroup.h>
#include <vtkSMProxy.h>

#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QStyleOptionGraphicsItem>
#include <QVBoxLayout>

#include <pqActiveObjects.h>
#include <pqProxySelection.h>

pqNodeEditorNode::Verbosity pqNodeEditorNode::DefaultNodeVerbosity{
  pqNodeEditorNode::Verbosity::NORMAL
};

// ----------------------------------------------------------------------------
pqNodeEditorNode::pqNodeEditorNode(QGraphicsScene* qscene, pqProxy* prx, QGraphicsItem* parent)
  : QGraphicsItem(parent)
  , scene(qscene)
  , proxy(prx)
  , widgetContainer(new QWidget)
  , label(new QGraphicsTextItem("", this))
{
  // set options
  this->setFlag(ItemIsMovable);
  this->setFlag(ItemSendsGeometryChanges);
  this->setCacheMode(DeviceCoordinateCache);
  this->setCursor(Qt::ArrowCursor);
  this->setObjectName(QString("node") + this->proxy->getSMName());

  // determine port height
  if (auto* proxyAsFilter = dynamic_cast<pqPipelineFilter*>(this->proxy))
  {
    this->portContainerHeight =
      std::max(proxyAsFilter->getNumberOfInputPorts(), proxyAsFilter->getNumberOfOutputPorts()) *
      pqNodeEditorUtils::CONSTS::PORT_HEIGHT;
  }
  else if (auto* proxyAsSource = dynamic_cast<pqPipelineSource*>(this->proxy))
  {
    this->portContainerHeight =
      proxyAsSource->getNumberOfOutputPorts() * pqNodeEditorUtils::CONSTS::PORT_HEIGHT;
  }

  // init label
  {
    this->label->setObjectName("nodeLabel");
    this->label->setCursor(Qt::PointingHandCursor);

    QFont font;
    font.setBold(true);
    font.setPointSize(pqNodeEditorUtils::CONSTS::NODE_FONT_SIZE);
    this->label->setFont(font);

    // Get the name from the linked proxy and place it in the middle of the GUI
    // Function is connected to be called each time the proxy get renamed.
    auto nameChangeFunc = [=]() {
      this->label->setPlainText(this->proxy->getSMName());
      auto br = this->label->boundingRect();
      this->label->setPos(0.5 * (pqNodeEditorUtils::CONSTS::NODE_WIDTH - br.width()),
        -this->portContainerHeight - this->labelHeight);
    };
    QObject::connect(this->proxy, &pqPipelineSource::nameChanged, this->label, nameChangeFunc);

    nameChangeFunc();
  }

  // create a widget container for property and display widgets
  {
    this->widgetContainer->setObjectName("nodeContainer");
    this->widgetContainer->setMinimumWidth(pqNodeEditorUtils::CONSTS::NODE_WIDTH);
    this->widgetContainer->setMaximumWidth(pqNodeEditorUtils::CONSTS::NODE_WIDTH);

    // install resize event filter
    this->widgetContainer->installEventFilter(pqNodeEditorUtils::createInterceptor(
      this->widgetContainer, [this](QObject* /*object*/, QEvent* event) {
        if (event->type() == QEvent::LayoutRequest)
        {
          this->updateSize();
        }
        return false;
      }));
  }

  // initialize property widgets container
  {
    auto containerLayout = new QVBoxLayout;
    this->widgetContainer->setLayout(containerLayout);

    auto graphicsProxyWidget = new QGraphicsProxyWidget(this);
    graphicsProxyWidget->setObjectName("graphicsProxyWidget");
    graphicsProxyWidget->setWidget(this->widgetContainer);
    graphicsProxyWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

    this->proxyProperties = new pqProxyWidget(this->proxy->getProxy());
    this->proxyProperties->setObjectName("proxyPropertiesWidget");
    this->proxyProperties->updatePanel();
    containerLayout->addWidget(this->proxyProperties);

    this->setVerbosity(pqNodeEditorNode::DefaultNodeVerbosity);

    this->updateSize();
  }

  this->scene->addItem(this);
}

// ----------------------------------------------------------------------------
pqNodeEditorNode::pqNodeEditorNode(
  QGraphicsScene* qscene, pqPipelineSource* source, QGraphicsItem* parent)
  : pqNodeEditorNode(qscene, (pqProxy*)source, parent)
{
  this->setZValue(pqNodeEditorUtils::CONSTS::NODE_LAYER);

  // create ports
  {
    auto br = this->boundingRect();
    auto adjust = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
    br.adjust(adjust, adjust, -adjust, -adjust);

    if (auto proxyAsFilter = dynamic_cast<pqPipelineFilter*>(source))
    {
      for (int i = 0; i < proxyAsFilter->getNumberOfInputPorts(); i++)
      {
        auto iPort = new pqNodeEditorPort(
          pqNodeEditorPort::Type::INPUT, proxyAsFilter->getInputPortName(i), this);
        iPort->setPos(br.left(),
          -this->portContainerHeight + (i + 0.5) * pqNodeEditorUtils::CONSTS::PORT_HEIGHT);
        this->iPorts.push_back(iPort);
      }
    }

    for (int i = 0; i < source->getNumberOfOutputPorts(); i++)
    {
      auto oPort = new pqNodeEditorPort(
        pqNodeEditorPort::Type::OUTPUT, source->getOutputPort(i)->getPortName(), this);
      oPort->setPos(br.right(),
        -this->portContainerHeight + (i + 0.5) * pqNodeEditorUtils::CONSTS::PORT_HEIGHT);
      this->oPorts.push_back(oPort);
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
    this->setBackgroundStyle(this->proxy->modifiedState() == pqProxy::ModifiedState::MODIFIED
        ? BackgroundStyle::DIRTY
        : BackgroundStyle::NORMAL);
    return 1;
  });
}

// ----------------------------------------------------------------------------
pqNodeEditorNode::pqNodeEditorNode(QGraphicsScene* qscene, pqView* view, QGraphicsItem* parent)
  : pqNodeEditorNode(qscene, (pqProxy*)view, parent)
{
  this->setZValue(pqNodeEditorUtils::CONSTS::VIEW_NODE_LAYER);

  auto br = this->boundingRect();
  auto adjust = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  br.adjust(adjust, adjust, -adjust, -adjust);

  // create port
  auto iPort = new pqNodeEditorPort(pqNodeEditorPort::Type::INPUT, "", this);
  iPort->setPos(br.center().x(), br.top());
  this->iPorts.push_back(iPort);

  // create property widgets
  QObject::connect(this->proxyProperties, &pqProxyWidget::changeFinished, this, [this]() {
    this->proxy->setModifiedState(pqProxy::MODIFIED);
    this->proxyProperties->apply();
    qobject_cast<pqView*>(this->proxy)->render();
  });
}

// ----------------------------------------------------------------------------
pqNodeEditorNode::~pqNodeEditorNode()
{
  this->scene->removeItem(this);
}

// ----------------------------------------------------------------------------
int pqNodeEditorNode::updateSize()
{
  this->widgetContainer->resize(this->widgetContainer->layout()->sizeHint());

  this->prepareGeometryChange();

  this->widgetContainerWidth = this->widgetContainer->width();
  this->widgetContainerHeight = this->widgetContainer->height();

  emit this->nodeResized();

  return 1;
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::setOutlineStyle(OutlineStyle style)
{
  this->outlineStyle = style;
  this->setZValue(style == OutlineStyle::NORMAL ? pqNodeEditorUtils::CONSTS::NODE_LAYER
                                                : pqNodeEditorUtils::CONSTS::NODE_LAYER + 1);
  this->update(this->boundingRect());
}
// ----------------------------------------------------------------------------
void pqNodeEditorNode::setBackgroundStyle(BackgroundStyle style)
{
  this->backgroundStyle = style;
  this->update(this->boundingRect());
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::setVerbosity(Verbosity v)
{
  this->verbosity = v;
  switch (this->verbosity)
  {
    // The string "%%%..." is used to filter out every widget that does not contains such string as
    // a tag.
    // Since we're pretty sure no one will ever name or a document a property with such a string
    // this is ok.
    case Verbosity::EMPTY:
      this->proxyProperties->filterWidgets(false, "%%%%%%%%%%%%%%");
      break;
    case Verbosity::NORMAL:
      this->proxyProperties->filterWidgets(false);
      break;
    case Verbosity::ADVANCED:
      this->proxyProperties->filterWidgets(true);
      break;
    default:
      break;
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::incrementVerbosity()
{
  this->setVerbosity(static_cast<Verbosity>((static_cast<int>(this->verbosity) + 1) % 3));
}

// ----------------------------------------------------------------------------
QVariant pqNodeEditorNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (change == GraphicsItemChange::ItemPositionHasChanged)
  {
    emit this->nodeMoved();
  }

  return QGraphicsItem::itemChange(change, value);
}

// ----------------------------------------------------------------------------
QRectF pqNodeEditorNode::boundingRect() const
{
  auto br = QRectF(0, -this->portContainerHeight - this->labelHeight, this->widgetContainerWidth,
    this->widgetContainerHeight + this->portContainerHeight + this->labelHeight);
  constexpr qreal offset = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH + 3.0; // +padding
  br.adjust(-offset, -offset, offset, offset);

  return br;
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  auto palette = QApplication::palette();

  QPainterPath path;
  auto br = this->boundingRect();
  constexpr qreal offset = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  br.adjust(offset, offset, -offset, -offset);
  path.addRoundedRect(br, pqNodeEditorUtils::CONSTS::NODE_BORDER_RADIUS,
    pqNodeEditorUtils::CONSTS::NODE_BORDER_RADIUS);

  QPen pen;
  pen.setWidth(pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH);
  switch (this->outlineStyle)
  {
    case OutlineStyle::NORMAL:
      pen.setBrush(palette.light());
      break;
    case OutlineStyle::SELECTED_FILTER:
      pen.setBrush(palette.highlight());
      break;
    case OutlineStyle::SELECTED_VIEW:
      pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_DARK_ORANGE);
      break;
    default:
      break;
  }

  painter->setPen(pen);
  painter->fillPath(path,
    this->backgroundStyle == BackgroundStyle::DIRTY ? pqNodeEditorUtils::CONSTS::COLOR_DARK_GREEN
                                                    : palette.window().color());
  painter->drawPath(path);
}
