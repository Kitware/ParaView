/*=========================================================================
Copyright (c) 2021, Jonas Lukasczyk
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#include "pqNodeEditorWidget.h"

#include "pqNodeEditorEdge.h"
#include "pqNodeEditorNode.h"
#include "pqNodeEditorPort.h"
#include "pqNodeEditorScene.h"
#include "pqNodeEditorUtils.h"
#include "pqNodeEditorView.h"

// qt includes
#include <QAction>
#include <QCheckBox>
#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>

// paraview includes
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

#include <vtkSMInputProperty.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMTrace.h>
#include <vtkSMViewProxy.h>

// for state files
#include <vtkPVXMLElement.h>
#include <vtkSMProxyLocator.h>

// std include
#include <iostream>

// TODO
#include <vtkSMPropertyIterator.h>

// ----------------------------------------------------------------------------
pqNodeEditorWidget::pqNodeEditorWidget(const QString& title, QWidget* parent)
  : QDockWidget(title, parent)
{
  // create widget
  auto widget = new QWidget(this);

  // create layout
  auto layout = new QVBoxLayout;
  widget->setLayout(layout);

  // create node editor scene and view
  this->scene = new pqNodeEditorScene(this);
  this->view = new pqNodeEditorView(this->scene, this);
  this->view->setDragMode(QGraphicsView::ScrollHandDrag);
  this->view->setSceneRect(-10000, -10000, 30000, 30000);
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
pqNodeEditorWidget::~pqNodeEditorWidget() = default;

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::apply()
{
  auto nodes = this->nodeRegistry;
  for (auto it : nodes)
  {
    auto proxy = dynamic_cast<pqPipelineSource*>(it.second->getProxy());
    if (proxy)
    {
      pqNodeEditorUtils::log("Apply Properties: " + pqNodeEditorUtils::getLabel(proxy));
      it.second->getProxyProperties()->apply();
      proxy->setModifiedState(pqProxy::ModifiedState::UNMODIFIED);
    }
  }
  for (auto it : nodes)
  {
    auto proxy = dynamic_cast<pqPipelineSource*>(it.second->getProxy());
    if (proxy)
    {
      pqNodeEditorUtils::log("Update Pipeline: " + pqNodeEditorUtils::getLabel(proxy));
      proxy->updatePipeline();
    }
  }
  for (auto it : nodes)
  {
    auto proxy = dynamic_cast<pqView*>(it.second->getProxy());
    if (proxy)
    {
      pqNodeEditorUtils::log("Update View: " + pqNodeEditorUtils::getLabel(proxy));
      proxy->render();
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
      pqNodeEditorUtils::log("Reset Properties: " + pqNodeEditorUtils::getLabel(proxy));
      it.second->getProxyProperties()->reset();
      proxy->setModifiedState(pqProxy::ModifiedState::UNMODIFIED);
    }
  }
  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::zoom()
{
  const int padding = 20;
  auto viewPort = this->scene->getBoundingRect(this->nodeRegistry);
  viewPort.adjust(-padding, -padding, padding, padding);
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
  QObject::connect(this->actionLayout, &QAction::triggered, this->scene, [=]() {
    this->scene->computeLayout(this->nodeRegistry, this->edgeRegistry);
    return 1;
  });

  this->actionAutoLayout = new QAction(this);
  QObject::connect(this->actionAutoLayout, &QAction::triggered, this->scene, [=]() {
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
  layout->addWidget(toolbar);

  auto toolbarLayout = new QHBoxLayout;
  toolbar->setLayout(toolbarLayout);

  auto addButton = [=](QString label, QAction* action) {
    auto button = new QPushButton(label);
    this->connect(button, &QPushButton::released, action, &QAction::trigger);
    toolbarLayout->addWidget(button);
    return 1;
  };

  addButton("Apply", this->actionApply);
  addButton("Reset", this->actionReset);

  addButton("Layout", this->actionLayout);
  {
    auto checkBox = new QCheckBox("Auto Layout");
    checkBox->setCheckState(this->autoUpdateLayout ? Qt::Checked : Qt::Unchecked);
    this->connect(checkBox, &QCheckBox::stateChanged, this, [=](int state) {
      this->autoUpdateLayout = state;
      this->actionAutoLayout->trigger();
      return 1;
    });
    toolbarLayout->addWidget(checkBox);
  }

  addButton("Zoom", actionZoom);

  {
    auto checkBox = new QCheckBox("Debug");
    checkBox->setCheckState(pqNodeEditorUtils::CONSTS::DEBUG ? Qt::Checked : Qt::Unchecked);
    this->connect(checkBox, &QCheckBox::stateChanged, this, [=](int state) {
      pqNodeEditorUtils::CONSTS::DEBUG = state;
      return 1;
    });
    toolbarLayout->addWidget(checkBox);
  }

  addButton("Collapse All", actionCollapseAllNodes);

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
  this->connect(appCore, &pqApplicationCore::aboutToLoadState, this, [=](vtkPVXMLElement* root) {
    // PV BUG: does not fire
  });
  this->connect(appCore, &pqApplicationCore::stateLoaded, this,
    [=](vtkPVXMLElement* root, vtkSMProxyLocator* locator) {
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
    this, [=](pqPipelineSource* source, pqPipelineSource* consumer, int srcOutputPort) {
      return this->updatePipelineEdges(consumer);
    });

  // edge creation
  this->connect(smm,
    static_cast<void (pqServerManagerModel::*)(pqPipelineSource*, pqPipelineSource*, int)>(
                  &pqServerManagerModel::connectionAdded),
    this, [=](pqPipelineSource* source, pqPipelineSource* consumer, int srcOutputPort) {
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

  // init node editor scene with exisiting views
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
  pqNodeEditorUtils::log("Update Active View");

  auto view = pqActiveObjects::instance().activeView();

  for (auto it : this->nodeRegistry)
  {
    if (dynamic_cast<pqView*>(it.second->getProxy()))
    {
      it.second->setOutlineStyle(0);
    }
    else
    {
      it.second->getProxyProperties()->setView(view);
    }
  }

  if (!view)
  {
    return 1;
  }

  auto nodeIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(view));
  if (nodeIt == this->nodeRegistry.end())
  {
    return 1;
  }

  nodeIt->second->setOutlineStyle(2);

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::updateActiveSourcesAndPorts()
{
  pqNodeEditorUtils::log("Selection Changed:");

  // unselect all nodes
  for (auto it : this->nodeRegistry)
  {
    if (!dynamic_cast<pqPipelineSource*>(it.second->getProxy()))
    {
      continue;
    }

    it.second->setOutlineStyle(0);
    for (auto oPort : it.second->getOutputPorts())
    {
      oPort->setStyle(0);
    }
  }

  // select nodes in selection
  const auto selection = pqActiveObjects::instance().selection();

  for (auto it : selection)
  {
    if (auto source = dynamic_cast<pqPipelineSource*>(it))
    {
      pqNodeEditorUtils::log("    -> source/filter");

      auto nodeIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(source));
      if (nodeIt == this->nodeRegistry.end())
      {
        continue;
      }

      nodeIt->second->setOutlineStyle(1);

      auto oPorts = nodeIt->second->getOutputPorts();
      if (oPorts.size() > 0)
      {
        oPorts[0]->setStyle(1);
      }
    }
    else if (auto port = dynamic_cast<pqOutputPort*>(it))
    {
      pqNodeEditorUtils::log("    -> port");
      auto nodeIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(port->getSource()));
      if (nodeIt == this->nodeRegistry.end())
      {
        continue;
      }

      nodeIt->second->setOutlineStyle(1);
      nodeIt->second->getOutputPorts()[port->getPortNumber()]->setStyle(1);
    }
  }

  return 1;
}

// ----------------------------------------------------------------------------
pqNodeEditorNode* pqNodeEditorWidget::createNode(pqProxy* proxy)
{
  pqNodeEditorUtils::log("Proxy Added: " + pqNodeEditorUtils::getLabel(proxy));

  auto id = pqNodeEditorUtils::getID(proxy);

  // insert new node into registry
  auto proxyAsView = dynamic_cast<pqView*>(proxy);
  auto proxyAsSource = dynamic_cast<pqPipelineSource*>(proxy);

  auto node = proxyAsView ? new pqNodeEditorNode(this->scene, proxyAsView) : proxyAsSource
      ? new pqNodeEditorNode(this->scene, proxyAsSource)
      : nullptr;

  if (!node)
  {
    pqNodeEditorUtils::log("ERROR: Unable to create node for pqProxy.", true);
    return nullptr;
  }

  this->nodeRegistry.insert({ id, node });
  this->edgeRegistry.insert({ id, std::vector<pqNodeEditorEdge*>() });

  QObject::connect(node, &pqNodeEditorNode::nodeResized, this->actionAutoLayout, &QAction::trigger);

  auto activeProxy = pqActiveObjects::instance().activeSource();
  if (activeProxy)
  {
    auto prevNodeIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(activeProxy));
    if (prevNodeIt != this->nodeRegistry.end())
    {
      auto prevPos = prevNodeIt->second->pos();
      node->setPos(prevPos.x() + 350, prevPos.y());
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
    node->getLabel()->installEventFilter(
      pqNodeEditorUtils::createInterceptor(node->getLabel(), [=](QObject* object, QEvent* event) {
        if (event->type() != QEvent::GraphicsSceneMousePress)
        {
          return false;
        }
        auto eventMDC = static_cast<QGraphicsSceneMouseEvent*>(event);

        // double left click
        if (eventMDC->button() == 1 && pqNodeEditorUtils::isDoubleClick())
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
            // TODO
            // auto temp = proxy->getProxy();
            // temp->InvokeCommand("ShowWidget");

            // auto it = temp->NewPropertyIterator();
            // it->SetTraverseSubProxies(1);
            // it->Begin();
            // while(!it->IsAtEnd()){
            //     it->GetProxy()->InvokeCommand("ShowWidget");
            //     it->Next();
            // }
          }

          return false;
        }

        // single right click
        if (eventMDC->button() == 2)
        {
          node->setVerbosity(node->getVerbosity() + 1);
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
        nodePort->getLabel(), [=](QObject* object, QEvent* event) {
          if (event->type() != QEvent::GraphicsSceneMousePress)
          {
            return false;
          }

          auto eventMDC = static_cast<QGraphicsSceneMouseEvent*>(event);
          if (pqNodeEditorUtils::isDoubleClick())
          {
            this->setInput(proxy, idx, eventMDC->modifiers() == Qt::ControlModifier);
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
        nodePort->getLabel(), [=](QObject* object, QEvent* event) {
          if (event->type() != QEvent::GraphicsSceneMousePress)
          {
            return false;
          }

          auto eventMDC = static_cast<QGraphicsSceneMouseEvent*>(event);

          auto portProxy = proxy->getOutputPort(idx);

          // double left click
          if (eventMDC->button() == 1 && pqNodeEditorUtils::isDoubleClick())
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
          if (eventMDC->button() == 1 && eventMDC->modifiers() & Qt::ShiftModifier)
          {
            pqNodeEditorUtils::log("Change Visibility of Port:");
            pqNodeEditorUtils::log("    " + std::to_string(idx));

            // exclusive
            if (eventMDC->modifiers() & Qt::ControlModifier)
            {
              this->hideAllInActiveView();
            }

            this->toggleInActiveView(proxy->getOutputPort(idx));

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
    [=](pqRepresentation* rep, bool visible) { return this->updateVisibilityEdges(proxy); });

  // update proxy selection
  node->getLabel()->installEventFilter(
    pqNodeEditorUtils::createInterceptor(node->getLabel(), [=](QObject* object, QEvent* event) {
      if (event->type() != QEvent::GraphicsSceneMousePress)
      {
        return false;
      }

      auto eventMDC = static_cast<QGraphicsSceneMouseEvent*>(event);

      // double left click
      if (eventMDC->button() == 1 && pqNodeEditorUtils::isDoubleClick())
      {
        pqActiveObjects::instance().setActiveView(proxy);
      }

      // single right click
      if (eventMDC->button() == 2)
      {
        node->setVerbosity(node->getVerbosity() + 1);
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
  pqNodeEditorUtils::log("Proxy Removed: " + pqNodeEditorUtils::getLabel(proxy));

  // remove all visibility edges
  auto smm = pqApplicationCore::instance()->getServerManagerModel();
  for (auto view : smm->findItems<pqView*>())
  {
    this->removeIncomingEdges(view);
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
  for (auto view : smm->findItems<pqView*>())
  {
    this->updateVisibilityEdges(view);
  }

  this->actionAutoLayout->trigger();

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::setInput(pqPipelineSource* consumer, int idx, bool clear)
{
  if (clear)
  {
    pqNodeEditorUtils::log(
      "Clear Input: " + pqNodeEditorUtils::getLabel(consumer) + "[" + std::to_string(idx) + "]");
  }
  else
  {
    pqNodeEditorUtils::log("Set Active Ports as Input: " + pqNodeEditorUtils::getLabel(consumer) +
      "[" + std::to_string(idx) + "]");
  }

  auto consumerAsFilter = dynamic_cast<pqPipelineFilter*>(consumer);
  if (!consumerAsFilter)
  {
    return 1;
  }

  BEGIN_UNDO_SET(QString("Change Input for %1").arg(consumerAsFilter->getSMName()));
  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", consumerAsFilter->getProxy());

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
  auto view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    return 0;
  }

  auto viewSMProxy = static_cast<vtkSMViewProxy*>(view->getProxy());
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  auto state =
    controller->GetVisibility(port->getSourceProxy(), port->getPortNumber(), viewSMProxy);

  controller->SetVisibility(port->getSourceProxy(), port->getPortNumber(), viewSMProxy, !state);

  view->render();

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::hideAllInActiveView()
{
  auto view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    return 0;
  }

  auto viewSMProxy = static_cast<vtkSMViewProxy*>(view->getProxy());
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  for (auto nodeIt : this->nodeRegistry)
  {
    auto proxy = dynamic_cast<vtkSMSourceProxy*>(nodeIt.second->getProxy()->getProxy());
    if (proxy)
    {
      for (size_t jdx = 0; jdx < proxy->GetNumberOfOutputPorts(); jdx++)
      {
        controller->SetVisibility(proxy, jdx, viewSMProxy, false);
      }
    }
  }

  view->render();

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::collapseAllNodes()
{
  for (auto nodeIt : this->nodeRegistry)
  {
    nodeIt.second->setVerbosity(0);
  }

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::updateVisibilityEdges(pqView* proxy)
{
  pqNodeEditorUtils::log(
    "Updating Visibility Pipeline Edges: " + pqNodeEditorUtils::getLabel(proxy));

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
    viewEdgesIt->second.push_back(new pqNodeEditorEdge(
      this->scene, producerIt->second, producerPort->getPortNumber(), viewIt->second, 0, 1));
  }

  this->actionAutoLayout->trigger();

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::updatePipelineEdges(pqPipelineSource* consumer)
{
  pqNodeEditorUtils::log(
    "Updating Incoming Pipeline Edges: " + pqNodeEditorUtils::getLabel(consumer));

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
