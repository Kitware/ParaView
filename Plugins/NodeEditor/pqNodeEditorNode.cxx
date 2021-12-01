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

#include "pqNodeEditorLabel.h"
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
#include <QSplitter>
#include <QStyleOptionGraphicsItem>
#include <QVBoxLayout>

// The pqDoubleLineEdit.h file is only included to handle the issue that the
// simplified notation rendering of pqDoubleLineEdit widgets is currently not
// working correctly in QT Graphics View Framework and therefore needs to be
// explicitly disabled.
#include <pqDoubleLineEdit.h>

pqNodeEditorNode::Verbosity pqNodeEditorNode::DefaultNodeVerbosity{
  pqNodeEditorNode::Verbosity::NORMAL
};

// ----------------------------------------------------------------------------
pqNodeEditorNode::pqNodeEditorNode(QGraphicsScene* qscene, pqProxy* prx, QGraphicsItem* parent)
  : QGraphicsItem(parent)
  , scene(qscene)
  , proxy(prx)
  , proxyProperties(new pqProxyWidget(prx->getProxy()))
  , widgetContainer(new QWidget)
  , label(new pqNodeEditorLabel("", this))
{
  this->setFlag(GraphicsItemFlag::ItemIsMovable);
  this->setFlag(GraphicsItemFlag::ItemSendsGeometryChanges);
  this->setCacheMode(CacheMode::DeviceCoordinateCache);
  this->setCursor(Qt::ArrowCursor);
  this->setObjectName(QString("node") + this->proxy->getSMName());

  // compute headline height
  if (auto* proxyAsSource = dynamic_cast<pqPipelineSource*>(this->proxy))
  {
    int maxNPorts = proxyAsSource->getNumberOfOutputPorts();
    if (auto* proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy))
    {
      maxNPorts = std::max(maxNPorts, proxyAsFilter->getNumberOfInputPorts());
    }
    this->headlineHeight = pqNodeEditorUtils::CONSTS::PORT_HEIGHT * maxNPorts +
      pqNodeEditorUtils::CONSTS::PORT_PADDING * (maxNPorts + 1);
  }

  // init label
  {
    this->label->setObjectName("nodeLabel");
    this->label->setCursor(Qt::PointingHandCursor);

    QFont font;
    font.setBold(true);
    font.setPointSize(pqNodeEditorUtils::CONSTS::NODE_FONT_SIZE);
    this->label->setFont(font);

    // This function retrieves the name of the linked proxy and places it in the
    // middle of the node. If necessary the label is scaled to fit inside the
    // node. The function is connected to the nameChanged event.
    auto updateNodeLabel = [this]() {
      this->label->setPlainText(this->proxy->getSMName());
      this->label->setScale(1.0);

      const auto br = this->label->boundingRect();
      const auto nodeWidthToLabelWidthRatio = pqNodeEditorUtils::CONSTS::NODE_WIDTH / br.width();

      // if label width larger than node width resize label
      if (nodeWidthToLabelWidthRatio < 1.0)
      {
        this->label->setScale(nodeWidthToLabelWidthRatio);
      }

      this->label->setPos(
        0.5 * (pqNodeEditorUtils::CONSTS::NODE_WIDTH - br.width() * this->label->scale()), 1);
    };
    QObject::connect(this->proxy, &pqPipelineSource::nameChanged, this->label, updateNodeLabel);

    updateNodeLabel();
    this->labelHeight = this->label->boundingRect().height();
    this->headlineHeight += labelHeight + 3;
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

    // initialize property widgets container
    auto containerLayout = new QVBoxLayout;
    this->widgetContainer->setLayout(containerLayout);

    auto graphicsProxyWidget = new QGraphicsProxyWidget(this);
    graphicsProxyWidget->setObjectName("graphicsProxyWidget");
    graphicsProxyWidget->setWidget(this->widgetContainer);
    graphicsProxyWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    graphicsProxyWidget->setPos(QPointF(0, this->headlineHeight));

    this->proxyProperties->setObjectName("proxyPropertiesWidget");
    this->proxyProperties->updatePanel();

    // Disable the simplified notation rendering for pqDoubleLineEdit widgets.
    for (auto element : this->proxyProperties->findChildren<pqDoubleLineEdit*>())
    {
      element->setAlwaysUseFullPrecision(true);
    }

    containerLayout->addWidget(this->proxyProperties);
  }

  this->setVerbosity(pqNodeEditorNode::DefaultNodeVerbosity);
  this->updateSize();
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

    constexpr double portRadius = pqNodeEditorUtils::CONSTS::PORT_HEIGHT * 0.5;
    const vtkIdType proxyId = pqNodeEditorUtils::getID(this->proxy);

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
    this->setBackgroundStyle(dirty ? BackgroundStyle::DIRTY : BackgroundStyle::NORMAL);
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
}

// ----------------------------------------------------------------------------
pqNodeEditorNode::~pqNodeEditorNode()
{
  this->scene->removeItem(this);
}

// ----------------------------------------------------------------------------
int pqNodeEditorNode::updateSize()
{
  this->prepareGeometryChange();

  this->widgetContainer->resize(this->widgetContainer->layout()->sizeHint());
  Q_EMIT this->nodeResized();

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
    // a tag. Since we're pretty sure no one will ever name or a document a property with such a
    // string this is ok.
    case Verbosity::EMPTY:
      this->proxyProperties->filterWidgets(false, "%%%%%%%%%%%%%%");
      this->widgetContainer->hide();
      break;
    case Verbosity::NORMAL:
      this->proxyProperties->filterWidgets(false);
      this->widgetContainer->show();
      break;
    case Verbosity::ADVANCED:
      this->proxyProperties->filterWidgets(true);
      this->widgetContainer->show();
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
    Q_EMIT this->nodeMoved();
  }

  return QGraphicsItem::itemChange(change, value);
}

// ----------------------------------------------------------------------------
QRectF pqNodeEditorNode::boundingRect() const
{
  const auto& border = pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  const double height = this->headlineHeight +
    (this->widgetContainer->isVisible() ? this->widgetContainer->height() : 0.0);
  return QRectF(0, 0, pqNodeEditorUtils::CONSTS::NODE_WIDTH, height)
    .adjusted(-border, -border, border, border);
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  QPainterPath path;
  // Make sure the whole node is redrawn to avoid artefacts
  constexpr double borderOffset = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  const QRectF br =
    this->boundingRect().adjusted(borderOffset, borderOffset, -borderOffset, -borderOffset);
  path.addRoundedRect(
    br, pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH, pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH);

  QPen pen;
  pen.setWidth(pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH);
  switch (this->outlineStyle)
  {
    case OutlineStyle::NORMAL:
      pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_CONSTRAST);
      break;
    case OutlineStyle::SELECTED_FILTER:
      pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_HIGHLIGHT);
      break;
    case OutlineStyle::SELECTED_VIEW:
      pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_BASE_ORANGE);
      break;
    default:
      break;
  }

  painter->setPen(pen);
  painter->fillPath(path,
    this->backgroundStyle == BackgroundStyle::DIRTY ? pqNodeEditorUtils::CONSTS::COLOR_BASE_GREEN
                                                    : pqNodeEditorUtils::CONSTS::COLOR_BASE);
  painter->drawPath(path);
}
