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
#include "pqNodeEditorNode.h"
#include "pqNodeEditorPort.h"
#include "pqNodeEditorScene.h"
#include "pqNodeEditorUtils.h"
#include "pqNodeEditorView.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
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
pqNodeEditorWidget::pqNodeEditorWidget(const QString& title, QWidget* parent)
  : QDockWidget(title, parent)
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
  this->scene->setObjectName("sceneNE");
  this->view = new pqNodeEditorView(this->scene, this);
  this->view->setObjectName("viewNE");
  this->view->setDragMode(QGraphicsView::ScrollHandDrag);
  const QRectF MAX_SCENE_SIZE{ -1e4, -1e4, 3e4, 3e4 };
  this->view->setSceneRect(MAX_SCENE_SIZE);
  layout->addWidget(this->view);

  this->initializeActions();
  this->createToolbar(layout);

  this->attachServerManagerListeners();

  this->setWidget(widget);
}

// ----------------------------------------------------------------------------
pqNodeEditorWidget::pqNodeEditorWidget(QWidget* parent)
  : pqNodeEditorWidget("Node Editor", parent)
{
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::apply()
{
  auto nodes = this->nodeRegistry;
  for (auto it : nodes)
  {
    auto source = dynamic_cast<pqPipelineSource*>(it.second->getProxy());
    if (source)
    {
      it.second->getProxyProperties()->apply();
      source->setModifiedState(pqProxy::ModifiedState::UNMODIFIED);
      source->updatePipeline();
    }
  }

  auto activeView = pqActiveObjects::instance().activeView();
  if (!activeView)
  {
    return 1;
  }

  auto activeSource = pqActiveObjects::instance().activeSource();
  if (!activeSource || activeSource->getNumberOfConsumers() > 0)
  {
    return 1;
  }

  auto viewSMProxy = static_cast<vtkSMViewProxy*>(activeView->getProxy());
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  for (int i = 0; i < activeSource->getNumberOfOutputPorts(); i++)
  {
    controller->SetVisibility(
      static_cast<vtkSMSourceProxy*>(activeSource->getProxy()), i, viewSMProxy, 1);
  }

  if (auto activeFilter = dynamic_cast<pqPipelineFilter*>(activeSource))
  {
    for (auto port : activeFilter->getInputs())
    {
      controller->SetVisibility(static_cast<vtkSMSourceProxy*>(port->getSourceProxy()),
        port->getPortNumber(), viewSMProxy, 0);
    }
  }

  activeView->render();

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

  QRect viewPort = pqNodeEditorScene::getBoundingRect(this->nodeRegistry);
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

  this->actionCollapseAllNodes = new QAction(this);
  QObject::connect(
    this->actionCollapseAllNodes, &QAction::triggered, this, &pqNodeEditorWidget::collapseAllNodes);

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

  addButton("Apply", this->actionApply);
  addButton("Reset", this->actionReset);
  addButton("Layout", this->actionLayout);
  {
    auto checkBox = new QCheckBox("Auto Layout");
    checkBox->setObjectName("autolayoutCheckbox");
    checkBox->setCheckState(this->autoUpdateLayout ? Qt::Checked : Qt::Unchecked);
    this->connect(checkBox, &QCheckBox::stateChanged, this, [this](int state) {
      this->autoUpdateLayout = state;
      this->actionAutoLayout->trigger();
      return 1;
    });
    toolbarLayout->addWidget(checkBox);
  }
  addButton("Zoom", this->actionZoom);
  addButton("Collapse All", this->actionCollapseAllNodes);

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
      return this->updatePipelineEdges(consumer);
    });

  // edge creation
  this->connect(smm,
    static_cast<void (pqServerManagerModel::*)(pqPipelineSource*, pqPipelineSource*, int)>(
                  &pqServerManagerModel::connectionAdded),
    this, [this](pqPipelineSource* /*source*/, pqPipelineSource* consumer, int /*oPort*/) {
      return this->updatePipelineEdges(consumer);
    });

  // retrieve active object manager
  auto activeObjects = &pqActiveObjects::instance();

  // update proxy selections
  this->connect(activeObjects, &pqActiveObjects::selectionChanged, this,
    &pqNodeEditorWidget::updateActiveSourcesAndPorts);

  // update view selection
  this->connect(
    activeObjects, &pqActiveObjects::viewChanged, this, &pqNodeEditorWidget::updateActiveView);

  // init node editor scene with existing views
  {
    for (auto proxy : smm->findItems<pqPipelineSource*>())
    {
      this->createNodeForSource(proxy);
      this->updatePipelineEdges(proxy);
    }

    for (auto proxy : smm->findItems<pqView*>())
    {
      this->createNodeForView(proxy);
      this->updateVisibilityEdges(proxy);
      this->updateActiveView();
    }
  }

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
    }
    else
    {
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
      oPort->setStyle(pqNodeEditorPort::NodeStyle::NORMAL);
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
        oPorts[0]->setStyle(pqNodeEditorPort::NodeStyle::SELECTED);
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
      nodeIt->second->getOutputPorts()[port->getPortNumber()]->setStyle(
        pqNodeEditorPort::NodeStyle::SELECTED);
    }
  }

  return 1;
}

// ----------------------------------------------------------------------------
pqNodeEditorNode* pqNodeEditorWidget::createNode(pqProxy* proxy)
{
  constexpr int DEFAULT_X_OFFSET = 350;

  auto id = pqNodeEditorUtils::getID(proxy);

  // insert new node into registry
  auto* proxyAsView = dynamic_cast<pqView*>(proxy);
  auto* proxyAsSource = dynamic_cast<pqPipelineSource*>(proxy);

  auto* node = proxyAsView ? new pqNodeEditorNode(this->scene, proxyAsView) : proxyAsSource
      ? new pqNodeEditorNode(this->scene, proxyAsSource)
      : nullptr;

  if (!node)
  {
    vtkGenericWarningMacro(
      "NodeEditor: Unable to create node for proxy " << pqNodeEditorUtils::getLabel(proxy));
    return nullptr;
  }

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

  return node;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::createNodeForSource(pqPipelineSource* proxy)
{
  auto node = this->createNode(proxy);

  // update proxy selection
  {
    node->getLabel()->installEventFilter(pqNodeEditorUtils::createInterceptor(
      node->getLabel(), [=](QObject* /*object*/, QEvent* event) {
        if (event->type() != QEvent::GraphicsSceneMousePress)
        {
          return false;
        }
        auto eventMDC = static_cast<QGraphicsSceneMouseEvent*>(event);

        // double left click
        if (eventMDC->button() == Qt::MouseButton::LeftButton && pqNodeEditorUtils::isDoubleClick())
        {
          auto activeObjects = &pqActiveObjects::instance();

          // add to selection or make single active selection
          if (eventMDC->modifiers() == Qt::ControlModifier)
          {
            pqProxySelection sel = activeObjects->selection();
            sel.push_back(proxy);
            activeObjects->setSelection(sel, proxy);
          }
          else
          {
            activeObjects->setActiveSource(proxy);
          }

          return false;
        }

        // single right click
        if (eventMDC->button() == Qt::MouseButton::RightButton)
        {
          node->incrementVerbosity();
          return false;
        }

        return false;
      }));
  }

  // input port events
  if (auto proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy))
  {
    for (size_t idx = 0; idx < node->getInputPorts().size(); idx++)
    {
      auto nodePort = node->getInputPorts()[idx];
      nodePort->getLabel()->installEventFilter(pqNodeEditorUtils::createInterceptor(
        nodePort->getLabel(), [=](QObject* /*object*/, QEvent* event) {
          if (event->type() != QEvent::GraphicsSceneMousePress)
          {
            return false;
          }

          auto eventMDC = static_cast<QGraphicsSceneMouseEvent*>(event);
          if (pqNodeEditorUtils::isDoubleClick())
          {
            this->setInput(
              proxy, static_cast<int>(idx), eventMDC->modifiers() == Qt::ControlModifier);
          }

          return false;
        }));
    }
  }

  // output port events
  {
    for (size_t idx = 0; idx < node->getOutputPorts().size(); idx++)
    {
      auto nodePort = node->getOutputPorts()[idx];
      nodePort->getLabel()->installEventFilter(pqNodeEditorUtils::createInterceptor(
        nodePort->getLabel(), [=](QObject* /*object*/, QEvent* event) {
          if (event->type() != QEvent::GraphicsSceneMousePress)
          {
            return false;
          }

          auto eventMDC = static_cast<QGraphicsSceneMouseEvent*>(event);

          auto portProxy = proxy->getOutputPort(static_cast<int>(idx));

          // double left click
          if (eventMDC->button() == Qt::MouseButton::LeftButton &&
            pqNodeEditorUtils::isDoubleClick())
          {
            auto activeObjects = &pqActiveObjects::instance();

            // add to selection or make single active selection
            if (eventMDC->modifiers() == Qt::ControlModifier)
            {
              pqProxySelection sel = activeObjects->selection();
              sel.push_back(portProxy);
              activeObjects->setSelection(sel, portProxy);
            }
            else
            {
              activeObjects->setActivePort(portProxy);
            }

            return true;
          }

          // toggle visibility
          if (eventMDC->button() == Qt::MouseButton::LeftButton &&
            eventMDC->modifiers() & Qt::ShiftModifier)
          {
            // exclusive
            if (eventMDC->modifiers() & Qt::ControlModifier)
            {
              this->hideAllInActiveView();
            }

            this->toggleInActiveView(proxy->getOutputPort(static_cast<int>(idx)));

            return true;
          }

          return false;
        }));
    }
  }

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::createNodeForView(pqView* proxy)
{
  auto node = this->createNode(proxy);

  // update representation link
  QObject::connect(proxy, &pqView::representationVisibilityChanged, node,
    [=](pqRepresentation* /*rep*/, bool /*visible*/) {
      return this->updateVisibilityEdges(proxy);
    });

  // update proxy selection
  node->getLabel()->installEventFilter(
    pqNodeEditorUtils::createInterceptor(node->getLabel(), [=](QObject* /*object*/, QEvent* event) {
      if (event->type() != QEvent::GraphicsSceneMousePress)
      {
        return false;
      }

      auto eventMDC = static_cast<QGraphicsSceneMouseEvent*>(event);

      // double left click
      if (eventMDC->button() == Qt::MouseButton::LeftButton && pqNodeEditorUtils::isDoubleClick())
      {
        pqActiveObjects::instance().setActiveView(proxy);
      }

      // single right click
      if (eventMDC->button() == Qt::MouseButton::RightButton)
      {
        node->incrementVerbosity();
        return false;
      }

      return false;
    }));

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::removeIncomingEdges(pqProxy* proxy)
{
  auto edgesIt = this->edgeRegistry.find(pqNodeEditorUtils::getID(proxy));
  if (edgesIt != this->edgeRegistry.end())
  {
    for (int i = 0; i < edgesIt->second.size(); i++)
    {
      if (edgesIt->second[i])
      {
        delete edgesIt->second[i];
      }
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
      auto itemAsSource = dynamic_cast<pqPipelineSource*>(item);
      auto itemAsPort = dynamic_cast<pqOutputPort*>(item);
      if (itemAsPort)
      {
        inputPtrs.push_back(itemAsPort->getSource()->getProxy());
        inputPorts.push_back(itemAsPort->getPortNumber());
      }
      else if (itemAsSource)
      {
        inputPtrs.push_back(itemAsSource->getProxy());
        inputPorts.push_back(0);
      }
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
int pqNodeEditorWidget::collapseAllNodes()
{
  for (auto nodeIt : this->nodeRegistry)
  {
    nodeIt.second->setVerbosity(pqNodeEditorNode::Verbosity::EMPTY);
  }

  return 1;
};

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
      producerPort->getPortNumber(), viewIt->second, 0, pqNodeEditorEdge::EdgeType::VIEW));
  }

  this->actionAutoLayout->trigger();

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::updatePipelineEdges(pqPipelineSource* consumer)
{
  // check if consumer is actually a filter
  auto consumerAsFilter = dynamic_cast<pqPipelineFilter*>(consumer);
  if (!consumerAsFilter)
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
  for (int iPortIdx = 0; iPortIdx < consumerAsFilter->getNumberOfInputPorts(); iPortIdx++)
  {

    // retrieve current input port name
    auto iPortName = consumerAsFilter->getInputPortName(iPortIdx);

    // get number of all outconsumerNodeput ports connected to current input port
    int numberOfOutputPortsAtInputPort = consumerAsFilter->getNumberOfInputs(iPortName);
    for (int oPortIt = 0; oPortIt < numberOfOutputPortsAtInputPort; oPortIt++)
    {
      // get current output port connected to current input port
      auto producerPort = consumerAsFilter->getInput(iPortName, oPortIt);

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
