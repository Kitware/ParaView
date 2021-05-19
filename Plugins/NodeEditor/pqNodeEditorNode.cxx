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

// ----------------------------------------------------------------------------
pqNodeEditorNode::pqNodeEditorNode(QGraphicsScene* scene, pqProxy* proxy, QGraphicsItem* parent)
  : QGraphicsItem(parent)
  , scene(scene)
  , proxy(proxy)
  , widgetContainer(new QWidget)
  , label(new QGraphicsTextItem("", this))
{
  // set options
  this->setFlag(ItemIsMovable);
  this->setFlag(ItemSendsGeometryChanges);
  this->setCacheMode(DeviceCoordinateCache);
  this->setCursor(Qt::ArrowCursor);
  this->setZValue(pqNodeEditorUtils::CONSTS::NODE_LAYER);
  this->setObjectName(QString("node") + proxy->getSMName());

  // determine port height
  if (auto* proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy))
  {
    this->portContainerHeight =
      std::max(proxyAsFilter->getNumberOfInputPorts(), proxyAsFilter->getNumberOfOutputPorts()) *
      this->portHeight;
  }
  else if (auto* proxyAsSource = dynamic_cast<pqPipelineSource*>(proxy))
  {
    this->portContainerHeight = proxyAsSource->getNumberOfOutputPorts() * this->portHeight;
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
      this->label->setPlainText(proxy->getSMName());
      auto br = this->label->boundingRect();
      this->label->setPos(0.5 * (pqNodeEditorUtils::CONSTS::NODE_WIDTH - br.width()),
        -this->portContainerHeight - this->labelHeight);
    };
    QObject::connect(proxy, &pqPipelineSource::nameChanged, this->label, nameChangeFunc);

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

    this->setVerbosity(
      static_cast<pqNodeEditorNode::Verbosity>(pqNodeEditorUtils::CONSTS::NODE_DEFAULT_VERBOSITY));

    this->updateSize();
  }

  this->scene->addItem(this);
}

// ----------------------------------------------------------------------------
pqNodeEditorNode::pqNodeEditorNode(
  QGraphicsScene* scene, pqPipelineSource* proxy, QGraphicsItem* parent)
  : pqNodeEditorNode(scene, (pqProxy*)proxy, parent)
{
  // create ports
  {
    auto br = this->boundingRect();
    auto adjust = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
    br.adjust(adjust, adjust, -adjust, -adjust);

    if (auto proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy))
    {
      for (int i = 0; i < proxyAsFilter->getNumberOfInputPorts(); i++)
      {
        auto iPort = new pqNodeEditorPort(
          pqNodeEditorPort::NodeType::INPUT, proxyAsFilter->getInputPortName(i), this);
        iPort->setPos(br.left(), -this->portContainerHeight + (i + 0.5) * this->portHeight);
        this->iPorts.push_back(iPort);
      }
    }

    for (int i = 0; i < proxy->getNumberOfOutputPorts(); i++)
    {
      auto oPort = new pqNodeEditorPort(
        pqNodeEditorPort::NodeType::OUTPUT, proxy->getOutputPort(i)->getPortName(), this);
      oPort->setPos(br.right(), -this->portContainerHeight + (i + 0.5) * this->portHeight);
      this->oPorts.push_back(oPort);
    }
  }

  // create property widgets
  QObject::connect(this->proxyProperties, &pqProxyWidget::changeFinished, this, [this]() {
    this->proxy->setModifiedState(pqProxy::MODIFIED);
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
pqNodeEditorNode::pqNodeEditorNode(QGraphicsScene* scene, pqView* proxy, QGraphicsItem* parent)
  : pqNodeEditorNode(scene, (pqProxy*)proxy, parent)
{
  auto br = this->boundingRect();
  auto adjust = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  br.adjust(adjust, adjust, -adjust, -adjust);

  // create port
  auto iPort = new pqNodeEditorPort(pqNodeEditorPort::NodeType::INPUT, "", this);
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
  this->update(this->boundingRect());
}
// ----------------------------------------------------------------------------
void pqNodeEditorNode::setBackgroundStyle(BackgroundStyle style)
{
  this->backgroundStyle = style;
  this->update(this->boundingRect());
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::setVerbosity(Verbosity _verbosity)
{
  // this string is used to filter out every widget that does not contains such string as a tag.
  // Since we're pretty sure no one will ever name or a document a property with such a string
  // we'll use this value.
  constexpr const char* NO_PROPERTIES_FILTER = "%%%%%%%%%%%%%%";

  this->verbosity = _verbosity;
  switch (this->verbosity)
  {
    case Verbosity::EMPTY:
      this->proxyProperties->filterWidgets(false, NO_PROPERTIES_FILTER);
      break;
    case Verbosity::SIMPLE:
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
int pqNodeEditorNode::incrementVerbosity()
{
  int next = static_cast<int>(this->verbosity) + 1;
  Verbosity verb =
    next > static_cast<int>(Verbosity::ADVANCED) ? Verbosity::EMPTY : static_cast<Verbosity>(next);
  this->setVerbosity(verb);
  return this->getVerbosity();
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
  auto offset = pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  auto br = QRectF(-offset, -offset - this->portContainerHeight - this->labelHeight,
    this->widgetContainerWidth + 2 * offset,
    this->widgetContainerHeight + 2 * offset + this->portContainerHeight + this->labelHeight);
  br.adjust(0, 0, 0, pqNodeEditorUtils::CONSTS::NODE_BORDER_RADIUS);

  return br;
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  auto palette = QApplication::palette();

  QPainterPath path;
  int offset = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  auto br = this->boundingRect();
  br.adjust(offset, offset, -offset, -offset);
  path.addRoundedRect(br, pqNodeEditorUtils::CONSTS::NODE_BORDER_RADIUS,
    pqNodeEditorUtils::CONSTS::NODE_BORDER_RADIUS);

  QPen pen;
  pen.setWidth(pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH);
  switch (this->outlineStyle)
  {
    case OutlineStyle::NORMAL:
      pen.setBrush(palette.base());
      break;
    case OutlineStyle::SELECTED_FILTER:
      pen.setBrush(palette.highlight());
      break;
    case OutlineStyle::SELECTED_VIEW:
      pen.setBrush(palette.linkVisited());
      break;
    default:
      break;
  }

  painter->setPen(pen);
  painter->fillPath(path, this->backgroundStyle == BackgroundStyle::DIRTY
      ? pqNodeEditorUtils::CONSTS::COLOR_SWEET_GREEN
      : palette.window().color());
  painter->drawPath(path);
}
