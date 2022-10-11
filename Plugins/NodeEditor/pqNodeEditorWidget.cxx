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

#include "pqNodeEditorAnnotationItem.h"
#include "pqNodeEditorEdge.h"
#include "pqNodeEditorLabel.h"
#include "pqNodeEditorNSource.h"
#include "pqNodeEditorNView.h"
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
#include <pqSettings.h>
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
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QSpacerItem>
#include <QVBoxLayout>

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
  // Restore previous behavior for auto layout
  // XXX: ideally we'd really want to have a more flexible architecture if we want to restore
  // every behaviors : having a `.ui` file for the ui and instanciating a QDataWidgetMapper to
  // synchronize the UI with an actual Qt model would be nice
  auto* settings = pqApplicationCore::instance()->settings();
  this->autoUpdateLayout = settings->value("NodeEditor.autoUpdateLayout", false).toBool();

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
  : pqNodeEditorWidget(tr("Node Editor"), parent)
{
}

// ----------------------------------------------------------------------------
pqNodeEditorWidget::~pqNodeEditorWidget()
{
  auto* settings = pqApplicationCore::instance()->settings();
  settings->setValue("NodeEditor.autoUpdateLayout", this->autoUpdateLayout);
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
            BEGIN_UNDO_SET(tr("Change Input for %1").arg(to->getSMName()));
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

  QObject::connect(
    this->view, &pqNodeEditorView::annotate, this, &pqNodeEditorWidget::annotateNodes);

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

  addButton(tr("Apply"), this->actionApply);
  addButton(tr("Reset"), this->actionReset);
  addSeparator();

  addButton(tr("Zoom"), this->actionZoom);
  { // addButton "Layout"
    auto button = new QPushButton(tr("Layout"));
    button->setObjectName("LayoutButton");
    this->connect(button, &QPushButton::released, [this]() {
      this->actionLayout->trigger();
      this->actionZoom->trigger();
    });
    toolbarLayout->addWidget(button);
  };
  { // add checkbox auto layout
    auto checkBox = new QCheckBox(tr("Auto Layout"));
    checkBox->setObjectName("AutoLayoutCheckbox");
    checkBox->setCheckState(this->autoUpdateLayout ? Qt::Checked : Qt::Unchecked);
    this->connect(checkBox, &QCheckBox::stateChanged, this, [this](int state) {
      this->autoUpdateLayout = state;
      this->actionAutoLayout->trigger();
      return 1;
    });
    toolbarLayout->addWidget(checkBox);

    this->autoLayoutCheckbox = checkBox;
  }
  addSeparator();

  addButton(tr("Cycle Verbosity"), this->actionCycleNodeVerbosity);
  { // add checkbox view nodes
    auto checkBox = new QCheckBox(tr("View Nodes"));
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
  QObject::connect(appCore, &pqApplicationCore::aboutToReadState, this,
    [this](QString filename) { this->processedStateFile = filename; });

  QObject::connect(appCore, &pqApplicationCore::stateLoaded, this,
    [this](vtkPVXMLElement* /*root*/, vtkSMProxyLocator* /*locator*/) { this->importLayout(); });

  // state saved
  QObject::connect(appCore, &pqApplicationCore::aboutToWriteState, this,
    [this](QString filename) { this->processedStateFile = filename; });

  QObject::connect(appCore, &pqApplicationCore::stateSaved, this,
    [this](vtkPVXMLElement* /*root*/) { this->exportLayout(); });

  // Remove annotations when reset / connecting to a server
  QObject::connect(smm, &pqServerManagerModel::serverAdded, [this](pqServer*) {
    for (auto* annot : this->annotationRegistry)
    {
      this->scene->removeItem(annot);
    }
    this->annotationRegistry.clear();
  });

  // source/filter creation
  QObject::connect(
    smm, &pqServerManagerModel::sourceAdded, this, &pqNodeEditorWidget::createNodeForSource);

  // source/filter deletion
  QObject::connect(
    smm, &pqServerManagerModel::sourceRemoved, this, &pqNodeEditorWidget::removeNode);

  // view creation
  QObject::connect(
    smm, &pqServerManagerModel::viewAdded, this, &pqNodeEditorWidget::createNodeForView);

  // view deletion
  QObject::connect(smm, &pqServerManagerModel::viewRemoved, this, &pqNodeEditorWidget::removeNode);

  // edge removed
  QObject::connect(smm,
    static_cast<void (pqServerManagerModel::*)(pqPipelineSource*, pqPipelineSource*, int)>(
      &pqServerManagerModel::connectionRemoved),
    this, [this](pqPipelineSource* /*source*/, pqPipelineSource* consumer, int /*oPort*/) {
      return this->updatePipelineEdges(qobject_cast<pqPipelineFilter*>(consumer));
    });

  // edge creation
  QObject::connect(smm,
    static_cast<void (pqServerManagerModel::*)(pqPipelineSource*, pqPipelineSource*, int)>(
      &pqServerManagerModel::connectionAdded),
    this, [this](pqPipelineSource* /*source*/, pqPipelineSource* consumer, int /*oPort*/) {
      return this->updatePipelineEdges(qobject_cast<pqPipelineFilter*>(consumer));
    });

  // retrieve active object manager
  auto activeObjects = &pqActiveObjects::instance();

  // update proxy selections
  QObject::connect(activeObjects, &pqActiveObjects::selectionChanged, this,
    &pqNodeEditorWidget::updateActiveSourcesAndPorts);

  // update view selection
  QObject::connect(
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
    if (it.second->getNodeType() == pqNodeEditorNode::NodeType::VIEW)
    {
      it.second->setNodeActive(false);
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

  nodeIt->second->setNodeActive(true);

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
    if (it.second->getNodeType() == pqNodeEditorNode::NodeType::SOURCE)
    {
      it.second->setNodeActive(false);
      for (auto oPort : it.second->getOutputPorts())
      {
        oPort->setMarkedAsSelected(false);
      }
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

      nodeIt->second->setNodeActive(true);

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

      nodeIt->second->setNodeActive(true);
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

  auto* node = new pqNodeEditorNSource(this->scene, proxy);
  this->initializeNode(node, pqNodeEditorUtils::getID(proxy));

  QObject::connect(
    node, &pqNodeEditorNSource::inputPortClicked, [this, proxy](int port, bool clear) {
      this->setInput(proxy, port, clear);
      pqApplicationCore::instance()->render();
    });

  QObject::connect(
    node, &pqNodeEditorNSource::showOutputPort, this, &pqNodeEditorWidget::toggleInActiveView);

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::createNodeForView(pqView* proxy)
{
  if (proxy == nullptr)
  {
    return 0;
  }

  auto* node = new pqNodeEditorNView(this->scene, proxy);
  this->initializeNode(node, pqNodeEditorUtils::getID(proxy));

  // update representation link
  QObject::connect(proxy, &pqView::representationVisibilityChanged, node,
    [=](
      pqRepresentation* /*rep*/, bool /*visible*/) { return this->updateVisibilityEdges(proxy); });

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

  BEGIN_UNDO_SET(tr("Change Input for %1").arg(consumerAsFilter->getSMName()));

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
int pqNodeEditorWidget::toggleInActiveView(pqOutputPort* port, bool exclusive)
{
  if (exclusive)
  {
    this->hideAllInActiveView();
  }

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

// ----------------------------------------------------------------------------
void pqNodeEditorWidget::importLayout()
{
  const QString filename = this->constructLayoutFilename();

  if (!QFileInfo::exists(filename))
  {
    this->actionLayout->trigger();
    this->actionZoom->trigger();
  }
  else
  {
    // We want to deactivate the auto layout when we import a layout to be sure to
    // not mess with it
    this->autoLayoutCheckbox->setCheckState(Qt::CheckState::Unchecked);

    QSettings settings(filename, QSettings::Format::NativeFormat);
    for (auto node : this->nodeRegistry)
    {
      node.second->importLayout(settings);
    }
    auto annotations = pqNodeEditorAnnotationItem::importAll(settings);
    for (auto* annot : annotations)
    {
      this->scene->addItem(annot);
      this->annotationRegistry.push_back(annot);
    }

    this->actionZoom->trigger();
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorWidget::exportLayout()
{
  const QString filename = this->constructLayoutFilename();

  // If autolayout is disabled no need to export the layout. We should even delete
  // the existing layout if any.
  if (this->autoUpdateLayout)
  {
    if (QFileInfo::exists(filename))
    {
      QFile::remove(filename);
    }
    return;
  }

  QSettings settings(filename, QSettings::Format::NativeFormat);
  if (!settings.isWritable())
  {
    qWarning("NodeEditor: couldn't create a writable settings file, aborting");
    return;
  }

  settings.clear();
  for (auto node : this->nodeRegistry)
  {
    node.second->exportLayout(settings);
  }
  pqNodeEditorAnnotationItem::exportAll(settings, this->annotationRegistry);
}

// ----------------------------------------------------------------------------
void pqNodeEditorWidget::annotateNodes(bool del)
{
  if (del)
  {
    auto& registry = this->annotationRegistry;
    registry.erase(std::remove_if(registry.begin(), registry.end(),
                     [this](pqNodeEditorAnnotationItem* annot) {
                       const bool selected = annot->isSelected();
                       if (selected)
                       {
                         this->scene->removeItem(annot);
                       }
                       return selected;
                     }),
      registry.end());
  }
  else
  {
    QRectF bbox;
    for (const auto& nodeR : this->nodeRegistry)
    {
      auto* node = nodeR.second;
      if (node->isNodeActive())
      {
        bbox = bbox.united(node->boundingRect().translated(node->pos()));
      }
    }

    if (bbox.isValid())
    {
      constexpr float MARGIN = 30.f;
      bbox.adjust(-MARGIN, -MARGIN, MARGIN, MARGIN);
      auto* annot = new pqNodeEditorAnnotationItem(bbox);
      this->scene->addItem(annot);
      this->annotationRegistry.push_back(annot);
    }
  }
}

// ----------------------------------------------------------------------------
QString pqNodeEditorWidget::constructLayoutFilename() const
{
  if (this->processedStateFile.isEmpty())
  {
    return "";
  }
  else
  {
    const QFileInfo file(this->processedStateFile);
    return file.absoluteDir().filePath("." + file.baseName() + ".pvne");
  }
}
