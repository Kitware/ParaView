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

#include "pqNodeEditorWidget.h"

#include "pqNodeEditorEdge.h"
#include "pqNodeEditorLabel.h"
#include "pqNodeEditorNode.h"
#include "pqNodeEditorPort.h"
#include "pqNodeEditorScene.h"
#include "pqNodeEditorUtils.h"
#include "pqNodeEditorView.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqApplyBehavior.h>
#include <pqDeleteReaction.h>
#include <pqOutputPort.h>
#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqProxy.h>
#include <pqProxySelection.h>
#include <pqProxyWidget.h>
#include <pqServerManagerModel.h>
#include <pqUndoStack.h>
#include <pqView.h>

#include <vtkPVXMLElement.h>
#include <vtkSMInputProperty.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyLocator.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMTrace.h>
#include <vtkSMViewProxy.h>

#include <QAction>
#include <QCheckBox>
#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>

#include <iostream>

// ----------------------------------------------------------------------------
class pqNodeEditorApplyBehavior : public pqApplyBehavior
{
public:
  using pqApplyBehavior::pqApplyBehavior;

  void apply(pqProxy* proxy) { this->applied(nullptr, proxy); }

  void appliedGlobal() { this->applied(nullptr); }
};

// ----------------------------------------------------------------------------
pqNodeEditorWidget::pqNodeEditorWidget(const QString& title, QWidget* parent)
  : QDockWidget(title, parent)
  , applyBehavior(new pqNodeEditorApplyBehavior(this))
{
  // create widget
  auto widget = new QWidget(this);
  widget->setObjectName("nodeEditorWidget");

  // create layout
  auto layout = new QVBoxLayout;
  layout->setObjectName("vlayoutNE");
  widget->setLayout(layout);

  // create node editor scene and view
  this->scene = new pqNodeEditorScene(this);
  this->view = new pqNodeEditorView(this->scene, this);

  // toolbar
  this->initializeActions();
  this->createToolbar(layout);

  // add view to layout
  layout->addWidget(this->view);

  this->attachServerManagerListeners();
  this->initializeSignals();
  this->setWidget(widget);

  // initialize view extent
  this->view->fitInView(-2, -10, 20, 20, Qt::KeepAspectRatio);
}

// ----------------------------------------------------------------------------
pqNodeEditorWidget::pqNodeEditorWidget(QWidget* parent)
  : pqNodeEditorWidget("Node Editor", parent)
{
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::apply()
{
  for (auto it : this->nodeRegistry)
  {
    if (it.second->getProxy()->modifiedState() != pqProxy::UNMODIFIED)
    {
      it.second->getProxyProperties()->apply();
      this->applyBehavior->apply(it.second->getProxy());
    }
  }
  this->applyBehavior->appliedGlobal();

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::reset()
{
  for (auto it : this->nodeRegistry)
  {
    auto proxy = dynamic_cast<pqPipelineSource*>(it.second->getProxy());
    if (proxy)
    {
      it.second->getProxyProperties()->reset();
      proxy->setModifiedState(pqProxy::ModifiedState::UNMODIFIED);
    }
  }
  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::zoom()
{
  constexpr int PADDING = 20;
  QRectF viewPort;
  for (QGraphicsItem* item : scene->items())
  {
    if (item->isVisible())
    {
      viewPort = viewPort.united(item->sceneBoundingRect());
    }
  }
  viewPort.adjust(-PADDING, -PADDING, PADDING, PADDING);
  this->view->fitInView(viewPort, Qt::KeepAspectRatio);
  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::initializeActions()
{
  this->actionApply = new QAction(this);
  QObject::connect(this->actionApply, &QAction::triggered, this, &pqNodeEditorWidget::apply);

  this->actionReset = new QAction(this);
  QObject::connect(this->actionReset, &QAction::triggered, this, &pqNodeEditorWidget::reset);

  this->actionZoom = new QAction(this);
  QObject::connect(this->actionZoom, &QAction::triggered, this, &pqNodeEditorWidget::zoom);

  this->actionLayout = new QAction(this);
  QObject::connect(this->actionLayout, &QAction::triggered, this->scene, [this]() {
    this->scene->computeLayout(this->nodeRegistry, this->edgeRegistry);
    return 1;
  });

  this->actionAutoLayout = new QAction(this);
  QObject::connect(this->actionAutoLayout, &QAction::triggered, this->scene, [this]() {
    if (this->autoUpdateLayout)
    {
      this->actionLayout->trigger();
    }
    return 1;
  });

  this->actionCycleNodeVerbosity = new QAction(this);
  QObject::connect(this->actionCycleNodeVerbosity, &QAction::triggered, this,
    &pqNodeEditorWidget::cycleNodeVerbosity);

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::initializeSignals()
{
  QObject::connect(this->scene, &pqNodeEditorScene::edgeDragAndDropRelease,
    [this](int fromProxy, int fromPort, int toProxy, int toPort) {
      auto* from = qobject_cast<pqPipelineSource*>(this->nodeRegistry[fromProxy]->getProxy());
      auto* to = qobject_cast<pqPipelineFilter*>(this->nodeRegistry[toProxy]->getProxy());
      if (from && to)
      {
        QString inName = to->getInputPortName(toPort);
        auto ip = vtkSMInputProperty::SafeDownCast(
          to->getProxy()->GetProperty(inName.toLocal8Bit().data()));
        if (ip)
        {
          if (ip->GetMultipleInput())
          {
            ip->AddInputConnection(from->getProxy(), fromPort);
          }
          else
          {
            BEGIN_UNDO_SET(QString("Change Input for %1").arg(to->getSMName()));
            ip->RemoveAllProxies();
            ip->AddInputConnection(from->getProxy(), fromPort);
            END_UNDO_SET();
          }
          to->getProxy()->UpdateVTKObjects();
          this->updatePipelineEdges(to);

          // render all views (see pqChangePipelineInputReaction)
          pqApplicationCore::instance()->render();
        }
      }
    });

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::createToolbar(QLayout* layout)
{
  auto toolbar = new QWidget(this);
  toolbar->setObjectName("toolbar");
  layout->addWidget(toolbar);

  auto toolbarLayout = new QHBoxLayout;
  toolbarLayout->setObjectName("HLayout");
  toolbar->setLayout(toolbarLayout);

  auto addButton = [=](QString label, QAction* action) {
    auto button = new QPushButton(label);
    button->setObjectName(label.simplified().remove(' ') + "Button");
    this->connect(button, &QPushButton::released, action, &QAction::trigger);
    toolbarLayout->addWidget(button);
    return 1;
  };

  auto addSeparator = [toolbarLayout]() {
    const auto separator = new QFrame();
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    toolbarLayout->addWidget(separator);
    return 1;
  };

  addButton("Apply", this->actionApply);
  addButton("Reset", this->actionReset);
  addSeparator();

  addButton("Zoom", this->actionZoom);
  { // addButton "Layout"
    auto button = new QPushButton("Layout");
    button->setObjectName("LayoutButton");
    this->connect(button, &QPushButton::released, [this]() {
      this->actionLayout->trigger();
      this->actionZoom->trigger();
    });
    toolbarLayout->addWidget(button);
  };
  { // add checkbox auto layout
    auto checkBox = new QCheckBox("Auto Layout");
    checkBox->setObjectName("AutoLayoutCheckbox");
    checkBox->setCheckState(this->autoUpdateLayout ? Qt::Checked : Qt::Unchecked);
    this->connect(checkBox, &QCheckBox::stateChanged, this, [this](int state) {
      this->autoUpdateLayout = state;
      this->actionAutoLayout->trigger();
      return 1;
    });
    toolbarLayout->addWidget(checkBox);
  }
  addSeparator();

  addButton("Cycle Verbosity", this->actionCycleNodeVerbosity);
  { // add checkbox view nodes
    auto checkBox = new QCheckBox("View Nodes");
    checkBox->setObjectName("ViewNodesCheckbox");
    checkBox->setCheckState(this->showViewNodes ? Qt::Checked : Qt::Unchecked);
    this->connect(checkBox, &QCheckBox::stateChanged, this, [this](int state) {
      this->showViewNodes = state;
      auto smm = pqApplicationCore::instance()->getServerManagerModel();
      for (auto proxy : smm->findItems<pqView*>())
      {
        this->updateVisibilityEdges(proxy);
      }
      this->updateActiveView();
      return 1;
    });
    toolbarLayout->addWidget(checkBox);
  }

  // add spacer
  toolbarLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::attachServerManagerListeners()
{
  // retrieve server manager model (used for listening to proxy events)
  auto appCore = pqApplicationCore::instance();
  auto smm = appCore->getServerManagerModel();

  // state loaded
  this->connect(appCore, &pqApplicationCore::stateLoaded, this,
    [this](vtkPVXMLElement* /*root*/, vtkSMProxyLocator* /*locator*/) {
      this->actionLayout->trigger();
      this->actionZoom->trigger();
    });

  // source/filter creation
  this->connect(
    smm, &pqServerManagerModel::sourceAdded, this, &pqNodeEditorWidget::createNodeForSource);

  // source/filter deletion
  this->connect(smm, &pqServerManagerModel::sourceRemoved, this, &pqNodeEditorWidget::removeNode);

  // view creation
  this->connect(
    smm, &pqServerManagerModel::viewAdded, this, &pqNodeEditorWidget::createNodeForView);

  // view deletion
  this->connect(smm, &pqServerManagerModel::viewRemoved, this, &pqNodeEditorWidget::removeNode);

  // edge removed
  this->connect(smm,
    static_cast<void (pqServerManagerModel::*)(pqPipelineSource*, pqPipelineSource*, int)>(
      &pqServerManagerModel::connectionRemoved),
    this, [this](pqPipelineSource* /*source*/, pqPipelineSource* consumer, int /*oPort*/) {
      return this->updatePipelineEdges(qobject_cast<pqPipelineFilter*>(consumer));
    });

  // edge creation
  this->connect(smm,
    static_cast<void (pqServerManagerModel::*)(pqPipelineSource*, pqPipelineSource*, int)>(
      &pqServerManagerModel::connectionAdded),
    this, [this](pqPipelineSource* /*source*/, pqPipelineSource* consumer, int /*oPort*/) {
      return this->updatePipelineEdges(qobject_cast<pqPipelineFilter*>(consumer));
    });

  // retrieve active object manager
  auto activeObjects = &pqActiveObjects::instance();

  // update proxy selections
  this->connect(activeObjects, &pqActiveObjects::selectionChanged, this,
    &pqNodeEditorWidget::updateActiveSourcesAndPorts);

  // update view selection
  this->connect(
    activeObjects, &pqActiveObjects::viewChanged, this, &pqNodeEditorWidget::updateActiveView);

  // init node editor scene with existing proxies
  for (auto proxy : smm->findItems<pqPipelineSource*>())
  {
    this->createNodeForSource(proxy);
    this->updatePipelineEdges(qobject_cast<pqPipelineFilter*>(proxy));
  }
  for (auto proxy : smm->findItems<pqView*>())
  {
    this->createNodeForView(proxy);
    this->updateVisibilityEdges(proxy);
    this->updateActiveView();
  }
  this->actionAutoLayout->trigger();

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::updateActiveView()
{
  auto aView = pqActiveObjects::instance().activeView();

  for (auto it : this->nodeRegistry)
  {
    if (dynamic_cast<pqView*>(it.second->getProxy()))
    {
      it.second->setOutlineStyle(pqNodeEditorNode::OutlineStyle::NORMAL);
      it.second->setVisible(this->showViewNodes);
    }
    else
    {
      // for 3D widgets; TODO: Probably related to 3D widgets toggle bug
      it.second->getProxyProperties()->setView(aView);
    }
  }

  if (!aView)
  {
    return 1;
  }

  auto nodeIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(aView));
  if (nodeIt == this->nodeRegistry.end())
  {
    return 1;
  }

  nodeIt->second->setOutlineStyle(pqNodeEditorNode::OutlineStyle::SELECTED_VIEW);

  // force redraw of edges
  for (auto edgesPerNodeIt : this->edgeRegistry)
  {
    for (auto edge : edgesPerNodeIt.second)
    {
      edge->setType(edge->getType());
    }
  }

  this->updatePortStyles();

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::updateActiveSourcesAndPorts()
{
  // unselect all nodes
  for (auto it : this->nodeRegistry)
  {
    if (!dynamic_cast<pqPipelineSource*>(it.second->getProxy()))
    {
      continue;
    }

    it.second->setOutlineStyle(pqNodeEditorNode::OutlineStyle::NORMAL);
    for (auto oPort : it.second->getOutputPorts())
    {
      oPort->setMarkedAsSelected(false);
    }
  }

  // select nodes in selection
  const auto selection = pqActiveObjects::instance().selection();

  for (auto it : selection)
  {
    if (auto source = dynamic_cast<pqPipelineSource*>(it))
    {
      auto nodeIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(source));
      if (nodeIt == this->nodeRegistry.end())
      {
        continue;
      }

      nodeIt->second->setOutlineStyle(pqNodeEditorNode::OutlineStyle::SELECTED_FILTER);

      auto oPorts = nodeIt->second->getOutputPorts();
      if (!oPorts.empty())
      {
        oPorts[0]->setMarkedAsSelected(true);
      }
    }
    else if (auto port = dynamic_cast<pqOutputPort*>(it))
    {
      auto nodeIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(port->getSource()));
      if (nodeIt == this->nodeRegistry.end())
      {
        continue;
      }

      nodeIt->second->setOutlineStyle(pqNodeEditorNode::OutlineStyle::SELECTED_FILTER);
      nodeIt->second->getOutputPorts()[port->getPortNumber()]->setMarkedAsSelected(true);
    }
  }

  return 1;
}

// ----------------------------------------------------------------------------
void pqNodeEditorWidget::initializeNode(pqNodeEditorNode* node, vtkIdType id)
{
  constexpr int DEFAULT_X_OFFSET = 1.15 * pqNodeEditorUtils::CONSTS::NODE_WIDTH;

  this->nodeRegistry.insert({ id, node });
  this->edgeRegistry.insert({ id, std::vector<pqNodeEditorEdge*>() });

  QObject::connect(node, &pqNodeEditorNode::nodeResized, this->actionAutoLayout, &QAction::trigger);

  pqPipelineSource* activeProxy = pqActiveObjects::instance().activeSource();
  if (activeProxy)
  {
    auto prevNodeIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(activeProxy));
    if (prevNodeIt != this->nodeRegistry.end())
    {
      QPointF prevPos = prevNodeIt->second->pos();
      node->setPos(prevPos.x() + DEFAULT_X_OFFSET, prevPos.y());
    }
  }

  this->actionAutoLayout->trigger();
  if (nodeRegistry.size() == 1)
  {
    this->actionZoom->trigger();
  }
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::createNodeForSource(pqPipelineSource* proxy)
{
  if (proxy == nullptr)
  {
    return 0;
  }

  auto* node = new pqNodeEditorNode(this->scene, proxy);
  this->initializeNode(node, pqNodeEditorUtils::getID(proxy));

  // node label events
  // right click : increment verbosity
  // left click : select node
  // left + ctrl : add to selection
  // middle click : delete node
  auto* nodeLabel = node->getLabel();
  nodeLabel->setMousePressEventCallback([node, proxy, this](QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::MouseButton::RightButton)
    {
      node->incrementVerbosity();
    }
    else if (event->button() == Qt::MouseButton::LeftButton)
    {
      auto* activeObjects = &pqActiveObjects::instance();
      if (event->modifiers() == 0)
      {
        activeObjects->setSelection({ proxy }, proxy);
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

  // input port label events
  // middle click : clear all incoming connections
  // left click + ctrl : set all incoming selected ports as input
  if (auto proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy))
  {
    for (size_t idx = 0; idx < node->getInputPorts().size(); idx++)
    {
      auto* label = node->getInputPorts()[idx]->getLabel();
      label->setMousePressEventCallback(
        [this, proxyAsFilter, idx](QGraphicsSceneMouseEvent* event) {
          if (event->button() == Qt::MouseButton::MiddleButton)
          {
            this->setInput(proxyAsFilter, static_cast<int>(idx), true);
            pqApplicationCore::instance()->render();
          }
          else if (event->button() == Qt::MouseButton::LeftButton &&
            (event->modifiers() & Qt::ControlModifier))
          {
            this->setInput(proxyAsFilter, static_cast<int>(idx), false);
          }
        });
    }
  }

  // output port label events
  // left click: set output port as active selection
  // left click + ctrl: add output port to active selection
  // left click + shift: toggle visibility in active view
  // left click + ctrl + shift: hide all but port in active view
  for (size_t idx = 0; idx < node->getOutputPorts().size(); idx++)
  {
    auto* label = node->getOutputPorts()[idx]->getLabel();

    label->setMousePressEventCallback([this, proxy, idx](QGraphicsSceneMouseEvent* event) {
      if (event->button() == Qt::MouseButton::LeftButton)
      {
        auto* activeObjects = &pqActiveObjects::instance();
        auto* portProxy = proxy->getOutputPort(static_cast<int>(idx));

        if (event->modifiers() & Qt::ShiftModifier)
        {
          if (event->modifiers() & Qt::ControlModifier)
          {
            this->hideAllInActiveView();
          }
          this->toggleInActiveView(proxy->getOutputPort(static_cast<int>(idx)));
        }
        else if (event->modifiers() == 0)
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

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::createNodeForView(pqView* proxy)
{
  if (proxy == nullptr)
  {
    return 0;
  }

  auto* node = new pqNodeEditorNode(this->scene, proxy);
  this->initializeNode(node, pqNodeEditorUtils::getID(proxy));

  // update representation link
  QObject::connect(proxy, &pqView::representationVisibilityChanged, node,
    [=](
      pqRepresentation* /*rep*/, bool /*visible*/) { return this->updateVisibilityEdges(proxy); });

  // node label events
  // left click : select as active view
  // right click : increment verbosity
  auto* nodeLabel = node->getLabel();
  nodeLabel->setMousePressEventCallback([this, node, proxy](QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::MouseButton::RightButton)
    {
      node->incrementVerbosity();
    }
    else if (event->button() == Qt::MouseButton::LeftButton)
    {
      pqActiveObjects::instance().setActiveView(proxy);
    }
  });

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::removeIncomingEdges(pqProxy* proxy)
{
  auto edgesIt = this->edgeRegistry.find(pqNodeEditorUtils::getID(proxy));
  if (edgesIt != this->edgeRegistry.end())
  {
    for (pqNodeEditorEdge* edge : edgesIt->second)
    {
      delete edge;
    }
    edgesIt->second.resize(0);
  }
  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::removeNode(pqProxy* proxy)
{
  // remove all visibility edges
  auto smm = pqApplicationCore::instance()->getServerManagerModel();
  for (auto cview : smm->findItems<pqView*>())
  {
    this->removeIncomingEdges(cview);
  }

  // get id
  auto proxyId = pqNodeEditorUtils::getID(proxy);

  // delete all incoming edges
  this->removeIncomingEdges(proxy);
  this->edgeRegistry.erase(proxyId);

  // delete all outgoing edges
  if (auto proxyAsSource = dynamic_cast<pqPipelineSource*>(proxy))
  {
    for (int p = 0; p < proxyAsSource->getNumberOfOutputPorts(); p++)
    {
      for (int i = 0; i < proxyAsSource->getNumberOfConsumers(p); i++)
      {
        this->removeIncomingEdges(proxyAsSource->getConsumer(p, i));
      }
    }
  }

  // delete node
  auto nodeIt = this->nodeRegistry.find(proxyId);
  if (nodeIt != this->nodeRegistry.end())
  {
    delete nodeIt->second;
  }
  this->nodeRegistry.erase(proxyId);

  // update visibility edges
  for (auto cview : smm->findItems<pqView*>())
  {
    this->updateVisibilityEdges(cview);
  }

  this->actionAutoLayout->trigger();

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::setInput(pqPipelineSource* consumer, int idx, bool clear)
{
  auto consumerAsFilter = dynamic_cast<pqPipelineFilter*>(consumer);
  if (!consumerAsFilter)
  {
    return 1;
  }

  BEGIN_UNDO_SET(QString("Change Input for %1").arg(consumerAsFilter->getSMName()));

  std::vector<vtkSMProxy*> inputPtrs;
  std::vector<unsigned int> inputPorts;

  if (!clear)
  {
    auto activeObjects = &pqActiveObjects::instance();
    auto selection = activeObjects->selection();

    for (auto item : selection)
    {
      if (auto itemAsPort = dynamic_cast<pqOutputPort*>(item))
      {
        inputPtrs.push_back(itemAsPort->getSource()->getProxy());
        inputPorts.push_back(itemAsPort->getPortNumber());
      }
      else if (auto itemAsSource = dynamic_cast<pqPipelineSource*>(item))
      {
        inputPtrs.push_back(itemAsSource->getProxy());
        inputPorts.push_back(0);
      }
    }

    // if no ports are selected do nothing
    if (inputPorts.empty())
    {
      return 1;
    }
  }

  auto iPortName = consumerAsFilter->getInputPortName(idx);
  auto ip = vtkSMInputProperty::SafeDownCast(
    consumerAsFilter->getProxy()->GetProperty(iPortName.toLocal8Bit().data()));
  ip->SetProxies(static_cast<unsigned int>(inputPtrs.size()), inputPtrs.data(), inputPorts.data());

  END_UNDO_SET();

  consumer->setModifiedState(pqProxy::ModifiedState::MODIFIED);

  this->actionAutoLayout->trigger();

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::toggleInActiveView(pqOutputPort* port)
{
  auto aView = pqActiveObjects::instance().activeView();
  if (!aView)
  {
    return 0;
  }

  auto viewSMProxy = static_cast<vtkSMViewProxy*>(aView->getProxy());
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  auto state =
    controller->GetVisibility(port->getSourceProxy(), port->getPortNumber(), viewSMProxy);

  controller->SetVisibility(port->getSourceProxy(), port->getPortNumber(), viewSMProxy, !state);

  aView->render();

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::hideAllInActiveView()
{
  auto aView = pqActiveObjects::instance().activeView();
  if (!aView)
  {
    return 0;
  }

  auto viewSMProxy = static_cast<vtkSMViewProxy*>(aView->getProxy());
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  for (auto nodeIt : this->nodeRegistry)
  {
    auto proxy = dynamic_cast<vtkSMSourceProxy*>(nodeIt.second->getProxy()->getProxy());
    if (proxy)
    {
      for (size_t jdx = 0; jdx < proxy->GetNumberOfOutputPorts(); jdx++)
      {
        controller->SetVisibility(proxy, static_cast<int>(jdx), viewSMProxy, false);
      }
    }
  }

  aView->render();

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::cycleNodeVerbosity()
{
  const auto verbosity = pqNodeEditorNode::CycleDefaultVerbosity();
  for (auto nodeIt : this->nodeRegistry)
  {
    nodeIt.second->setVerbosity(verbosity);
  }

  this->actionAutoLayout->trigger();

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::updatePortStyles()
{
  // mark all ports as invisible
  for (auto nodeIt : this->nodeRegistry)
  {
    for (auto port : nodeIt.second->getOutputPorts())
    {
      port->setMarkedAsVisible(false);
    }
  }

  // get active view
  auto aView = pqActiveObjects::instance().activeView();
  if (!aView)
  {
    return 1;
  }

  // iterate over active view edges to mark visible ports
  auto activeVisibilityEdges = this->edgeRegistry.find(pqNodeEditorUtils::getID(aView));
  if (activeVisibilityEdges == this->edgeRegistry.end())
  {
    return 1;
  }

  for (auto edge : activeVisibilityEdges->second)
  {
    edge->getProducer()->getOutputPorts()[edge->getProducerOutputPortIdx()]->setMarkedAsVisible(
      true);
  }

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::updateVisibilityEdges(pqView* proxy)
{
  this->removeIncomingEdges(proxy);

  auto viewEdgesIt = this->edgeRegistry.find(pqNodeEditorUtils::getID(proxy));
  if (viewEdgesIt == this->edgeRegistry.end())
  {
    return 1;
  }

  for (int i = 0; i < proxy->getNumberOfRepresentations(); i++)
  {
    auto rep = proxy->getRepresentation(i);
    if (!rep)
    {
      continue;
    }

    auto repAsDataRep = dynamic_cast<pqDataRepresentation*>(rep);
    if (!repAsDataRep || !repAsDataRep->isVisible())
    {
      continue;
    }

    auto producerPort = repAsDataRep->getOutputPortFromInput();
    auto producerIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(producerPort->getSource()));
    if (producerIt == this->nodeRegistry.end())
    {
      continue;
    }

    auto viewIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(proxy));
    if (viewIt == this->nodeRegistry.end())
    {
      continue;
    }

    // create edge
    viewEdgesIt->second.push_back(new pqNodeEditorEdge(this->scene, producerIt->second,
      producerPort->getPortNumber(), viewIt->second, 0, pqNodeEditorEdge::Type::VIEW));
    viewEdgesIt->second.back()->setVisible(this->showViewNodes);
  }

  this->updatePortStyles();

  this->actionAutoLayout->trigger();

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::updatePipelineEdges(pqPipelineFilter* consumer)
{
  if (consumer == nullptr)
  {
    return 1;
  }

  // get node of consumer
  auto consumerIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(consumer));
  if (consumerIt == this->nodeRegistry.end())
  {
    return 1;
  }

  // remove all input edges
  this->removeIncomingEdges(consumer);

  auto consumerEdgesIt = this->edgeRegistry.find(pqNodeEditorUtils::getID(consumer));
  if (consumerEdgesIt == this->edgeRegistry.end())
  {
    return 1;
  }

  // recreate all incoming edges
  for (int iPortIdx = 0; iPortIdx < consumer->getNumberOfInputPorts(); iPortIdx++)
  {

    // retrieve current input port name
    auto iPortName = consumer->getInputPortName(iPortIdx);

    // get number of all outconsumerNodeput ports connected to current input port
    int numberOfOutputPortsAtInputPort = consumer->getNumberOfInputs(iPortName);
    for (int oPortIt = 0; oPortIt < numberOfOutputPortsAtInputPort; oPortIt++)
    {
      // get current output port connected to current input port
      auto producerPort = consumer->getInput(iPortName, oPortIt);

      // get source of current output port
      auto producer = producerPort->getSource();

      // get node of producer
      auto producerIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(producer));
      if (producerIt == this->nodeRegistry.end())
      {
        continue;
      }

      // create edge
      consumerEdgesIt->second.push_back(new pqNodeEditorEdge(this->scene, producerIt->second,
        producerPort->getPortNumber(), consumerIt->second, iPortIdx));
    }
  }

  this->actionAutoLayout->trigger();

  return 1;
};
