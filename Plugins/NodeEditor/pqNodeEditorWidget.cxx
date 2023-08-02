// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2021 Jonas Lukasczyk
// SPDX-License-Identifier: BSD-3-Clause

#include "pqNodeEditorWidget.h"

#include "pqNodeEditorAnnotationItem.h"
#include "pqNodeEditorEdge.h"
#include "pqNodeEditorLabel.h"
#include "pqNodeEditorNRepresentation.h"
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

  for (auto edgesIt : this->edgeRegistry)
  {
    for (auto* edge : edgesIt.second)
    {
      delete edge;
    }
  }

  for (auto nodeIt : this->nodeRegistry)
  {
    delete nodeIt.second;
  }

  for (auto* annotIt : this->annotationRegistry)
  {
    delete annotIt;
  }
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::apply()
{
  for (const auto& node : this->nodeRegistry)
  {
    if (node.second->getProxy()->modifiedState() != pqProxy::UNMODIFIED)
    {
      node.second->getProxyProperties()->apply();
      this->applyBehavior->apply(node.second->getProxy());
    }
  }
  this->applyBehavior->appliedGlobal();

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::reset()
{
  for (const auto& node : this->nodeRegistry)
  {
    auto proxy = dynamic_cast<pqPipelineSource*>(node.second->getProxy());
    if (proxy)
    {
      node.second->getProxyProperties()->reset();
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
  this->view->update();
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

  auto toolbarLayout = new QGridLayout;
  toolbarLayout->setObjectName("GLayout");
  toolbar->setLayout(toolbarLayout);

  auto addButton = [=](QString label, QAction* action, int row, int col) {
    auto button = new QPushButton(label);
    button->setObjectName(label.simplified().remove(' ') + "Button");
    this->connect(button, &QPushButton::released, action, &QAction::trigger);
    toolbarLayout->addWidget(button, row, col);
    return 1;
  };

  addButton(tr("Apply"), this->actionApply, 0, 0);
  addButton(tr("Reset"), this->actionReset, 1, 0);

  addButton(tr("Verbosity"), this->actionCycleNodeVerbosity, 0, 1);
  { // add checkbox view nodes
    auto checkBox = new QCheckBox(tr("View Nodes"));
    checkBox->setObjectName("ViewNodesCheckbox");
    checkBox->setCheckState(this->showViewNodes ? Qt::Checked : Qt::Unchecked);
    this->connect(checkBox, &QCheckBox::stateChanged, this, [this](int state) {
      this->showViewNodes = state;
      this->toggleViewNodesVisibility();
    });
    toolbarLayout->addWidget(checkBox, 1, 1);
  }

  { // addButton "Layout"
    auto button = new QPushButton(tr("Layout"));
    button->setObjectName("LayoutButton");
    this->connect(button, &QPushButton::released, [this]() {
      this->actionLayout->trigger();
      this->actionZoom->trigger();
    });
    toolbarLayout->addWidget(button, 0, 2);
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
    toolbarLayout->addWidget(checkBox, 1, 2);

    this->autoLayoutCheckbox = checkBox;
  }
  addButton(tr("Zoom"), this->actionZoom, 0, 3);

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::attachServerManagerListeners()
{
  auto* appCore = pqApplicationCore::instance();

  // import layout when loading state
  QObject::connect(appCore, &pqApplicationCore::aboutToReadState, this,
    [this](QString filename) { this->processedStateFile = filename; });
  QObject::connect(appCore, &pqApplicationCore::stateLoaded, this,
    [this](vtkPVXMLElement* /*root*/, vtkSMProxyLocator* /*locator*/) { this->importLayout(); });

  // export layout when saving state
  QObject::connect(appCore, &pqApplicationCore::aboutToWriteState, this,
    [this](QString filename) { this->processedStateFile = filename; });
  QObject::connect(appCore, &pqApplicationCore::stateSaved, this,
    [this](vtkPVXMLElement* /*root*/) { this->exportLayout(); });

  auto* smm = appCore->getServerManagerModel();

  // Remove annotations when reset / connecting to a server
  QObject::connect(smm, &pqServerManagerModel::serverAdded, [this](pqServer*) {
    for (auto* annot : this->annotationRegistry)
    {
      delete annot;
    }
    this->annotationRegistry.clear();
  });

  // sources and filters
  QObject::connect(
    smm, &pqServerManagerModel::sourceAdded, this, &pqNodeEditorWidget::createNodeForSource);
  QObject::connect(
    smm, &pqServerManagerModel::sourceRemoved, this, &pqNodeEditorWidget::removeNode);

  // views
  QObject::connect(
    smm, &pqServerManagerModel::viewAdded, this, &pqNodeEditorWidget::createNodeForView);
  QObject::connect(smm, &pqServerManagerModel::viewRemoved, this, &pqNodeEditorWidget::removeNode);

  // representations
  // We don't create the `pqServerManagerModel::representationAdded` because given representations
  // are not in a valid state yet. Instead we use `pqView::representationAdded` (see
  // createNodeForSource). However we need to use `pqServerManagerModel` for the deletion or else we
  // won't catch it.
  QObject::connect(
    smm, &pqServerManagerModel::representationRemoved, [this](pqRepresentation* repr) {
      if (qobject_cast<pqDataRepresentation*>(repr))
      {
        this->removeNode(repr);
      }
    });

  // edges
  QObject::connect(smm,
    QOverload<pqPipelineSource*, pqPipelineSource*, int>::of(
      &pqServerManagerModel::connectionAdded),
    this, [this](pqPipelineSource* /*source*/, pqPipelineSource* consumer, int /*oPort*/) {
      return this->updatePipelineEdges(qobject_cast<pqPipelineFilter*>(consumer));
    });
  QObject::connect(smm,
    QOverload<pqPipelineSource*, pqPipelineSource*, int>::of(
      &pqServerManagerModel::connectionRemoved),
    this, [this](pqPipelineSource* /*source*/, pqPipelineSource* consumer, int /*oPort*/) {
      return this->updatePipelineEdges(qobject_cast<pqPipelineFilter*>(consumer));
    });

  auto* activeObjects = &pqActiveObjects::instance();

  // update pipeline selection
  QObject::connect(activeObjects, &pqActiveObjects::selectionChanged, this,
    &pqNodeEditorWidget::updateActiveSourcesAndPorts);

  // update view selection
  QObject::connect(
    activeObjects, &pqActiveObjects::viewChanged, this, &pqNodeEditorWidget::updateActiveView);

  // update representation selection
  QObject::connect(activeObjects,
    QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged),
    [this](pqDataRepresentation* repr) {
      for (const auto& nodeIt : this->nodeRegistry)
      {
        const auto reprId = pqNodeEditorUtils::getID(repr);
        if (nodeIt.second->getNodeType() == pqNodeEditorNode::NodeType::REPRESENTATION)
        {
          nodeIt.second->setNodeActive(nodeIt.first == reprId);
        }
      }
    });

  // init node editor scene with existing proxies
  for (auto proxy : smm->findItems<pqPipelineSource*>())
  {
    this->createNodeForSource(proxy);
    this->updatePipelineEdges(qobject_cast<pqPipelineFilter*>(proxy));
  }
  for (auto proxy : smm->findItems<pqView*>())
  {
    this->createNodeForView(proxy);
  }
  for (auto proxy : smm->findItems<pqDataRepresentation*>())
  {
    this->createNodeForRepresentation(proxy);
  }
  this->updateActiveView();
  this->toggleViewNodesVisibility();
  this->actionAutoLayout->trigger();

  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::updateActiveView(pqView* inView)
{
  auto aView = (inView == nullptr) ? pqActiveObjects::instance().activeView() : inView;

  for (const auto& nodeIt : this->nodeRegistry)
  {
    const auto& node = nodeIt.second;
    if (node->getNodeType() == pqNodeEditorNode::NodeType::VIEW)
    {
      node->setNodeActive(node->getProxy() == aView);
    }
    else
    {
      // for 3D widgets; TODO: Probably related to 3D widgets toggle bug
      node->getProxyProperties()->setView(aView);
    }
  }

  return 1;
}

// ----------------------------------------------------------------------------
void pqNodeEditorWidget::toggleViewNodesVisibility()
{
  for (const auto& nodeIt : this->nodeRegistry)
  {
    const auto& node = nodeIt.second;
    if (node->getNodeType() == pqNodeEditorNode::NodeType::VIEW)
    {
      node->setVisible(this->showViewNodes);
      auto& edges = this->edgeRegistry.at(nodeIt.first);
      for (auto& edge : edges)
      {
        const bool reprVis =
          qobject_cast<pqRepresentation*>(edge->getProducer()->getProxy())->isVisible();
        edge->setVisible(this->showViewNodes && reprVis);
      }
    }
    else if (node->getNodeType() == pqNodeEditorNode::NodeType::REPRESENTATION)
    {
      const bool reprVis = qobject_cast<pqRepresentation*>(node->getProxy())->isVisible();
      node->setVisible(this->showViewNodes && reprVis);
      auto& edges = this->edgeRegistry.at(nodeIt.first);
      for (auto& edge : edges)
      {
        edge->setVisible(this->showViewNodes && reprVis);
      }
    }
  }
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
void pqNodeEditorWidget::registerNode(pqNodeEditorNode* node, vtkIdType id)
{
  this->scene->addItem(node);
  this->scene->clearSelection();
  node->setSelected(true);
  this->nodeRegistry.insert({ id, node });
  this->edgeRegistry.insert({ id, {} });

  QObject::connect(node, &pqNodeEditorNode::nodeResized, this->actionAutoLayout, &QAction::trigger);

  // Some kind-of smart placement based on node type. See auto layout for smarter placement.
  if (nodeRegistry.size() == 1)
  {
    node->setPos(0., 0.);
    this->actionZoom->trigger();
  }
  else if (node->getNodeType() == pqNodeEditorNode::NodeType::VIEW)
  {
    auto* activeView = pqActiveObjects::instance().activeView();
    auto prevNodeIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(activeView));
    if (prevNodeIt != this->nodeRegistry.end())
    {
      const auto prevPos = prevNodeIt->second->pos();
      const auto prevBox = prevNodeIt->second->boundingRect();
      node->setPos(prevPos.x() + 2.0 * prevBox.width(), prevPos.y());
    }
  }
  else if (node->getNodeType() == pqNodeEditorNode::NodeType::SOURCE)
  {
    pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
    auto prevNodeIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(activeSource));
    if (prevNodeIt != this->nodeRegistry.end())
    {
      const auto prevPos = prevNodeIt->second->pos();
      const auto prevBox = prevNodeIt->second->boundingRect();
      node->setPos(prevPos.x() + 1.60 * prevBox.width(), prevPos.y());
    }
  }
  else if (node->getNodeType() == pqNodeEditorNode::NodeType::REPRESENTATION)
  {
    auto* reprSource = qobject_cast<pqDataRepresentation*>(node->getProxy())->getInput();
    auto prevNodeIt = this->nodeRegistry.find(pqNodeEditorUtils::getID(reprSource));
    if (prevNodeIt != this->nodeRegistry.end())
    {
      const auto prevPos = prevNodeIt->second->pos();
      const auto prevBox = prevNodeIt->second->boundingRect();
      node->setPos(prevPos.x() + prevBox.width() * 0.55, prevPos.y() + prevBox.height());
    }
  }

  this->actionAutoLayout->trigger();
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::createNodeForSource(pqPipelineSource* proxy)
{
  if (proxy == nullptr)
  {
    return 0;
  }

  auto* node = new pqNodeEditorNSource(proxy);
  this->registerNode(node, pqNodeEditorUtils::getID(proxy));

  QObject::connect(
    node, &pqNodeEditorNSource::inputPortClicked, [this, proxy](int port, bool clear) {
      this->setInput(proxy, port, clear);
      pqApplicationCore::instance()->render();
    });

  QObject::connect(
    node, &pqNodeEditorNSource::outputPortClicked, this, &pqNodeEditorWidget::toggleInActiveView);

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::createNodeForView(pqView* proxy)
{
  if (proxy == nullptr)
  {
    return 0;
  }

  auto* node = new pqNodeEditorNView(proxy);
  this->registerNode(node, pqNodeEditorUtils::getID(proxy));

  QObject::connect(
    proxy, &pqView::representationAdded, this, &pqNodeEditorWidget::createNodeForRepresentation);

  return 1;
};

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::createNodeForRepresentation(pqRepresentation* createdRepr)
{
  auto* repr = qobject_cast<pqDataRepresentation*>(createdRepr);
  if (repr == nullptr)
  {
    return 0;
  }
  const bool showNodes = repr->isVisible() && this->showViewNodes;

  auto* reprNode = new pqNodeEditorNRepresentation(repr);
  reprNode->setVisible(showNodes);
  const auto repId = pqNodeEditorUtils::getID(repr);
  this->registerNode(reprNode, repId);

  // Create edges related to this representation
  auto* sourcePort = repr->getOutputPortFromInput();
  auto* sourceNode = this->nodeRegistry.at(pqNodeEditorUtils::getID(sourcePort->getSource()));
  auto* sourceToRepr = new pqNodeEditorEdge(
    sourceNode, sourcePort->getPortNumber(), reprNode, 0, pqNodeEditorEdge::Type::VIEW);
  sourceToRepr->setVisible(showNodes);
  this->scene->addItem(sourceToRepr);
  this->scene->addItem(sourceToRepr->overlay());
  this->edgeRegistry.at(repId) = { sourceToRepr };

  const auto viewId = pqNodeEditorUtils::getID(repr->getView());
  auto* viewNode = this->nodeRegistry.at(viewId);
  auto* reprToView = new pqNodeEditorEdge(reprNode, 0, viewNode, 0, pqNodeEditorEdge::Type::VIEW);
  reprToView->setVisible(showNodes);
  this->scene->addItem(reprToView);
  this->scene->addItem(reprToView->overlay());
  this->edgeRegistry.at(viewId).emplace_back(reprToView);

  // Make sure to hide those when the representation is not visible anymore
  // XXX: `this` as third argument here is MANDATORY so we're sure that it is not deleted
  // when we call the lambda (which can sometimes happen when closing ParaView)
  QObject::connect(repr, &pqRepresentation::visibilityChanged, this, [=](bool visible) {
    if (this->edgeRegistry.count(repId) > 0)
    {
      sourceToRepr->setVisible(visible && this->showViewNodes);
    }
    if (this->nodeRegistry.count(repId) > 0)
    {
      reprNode->setVisible(visible && this->showViewNodes);
    }
    if (this->edgeRegistry.count(viewId) > 0)
    {
      reprToView->setVisible(visible && this->showViewNodes);
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
    edgesIt->second.clear();
  }
  return 1;
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::removeNode(pqProxy* proxy)
{
  const auto proxyId = pqNodeEditorUtils::getID(proxy);

  // delete all edges related to the node we're trying to remove
  this->removeIncomingEdges(proxy);
  this->edgeRegistry.erase(proxyId);
  for (auto& edgesIt : this->edgeRegistry)
  {
    auto& currentEdges = edgesIt.second;
    currentEdges.erase(std::remove_if(currentEdges.begin(), currentEdges.end(),
                         [proxy](pqNodeEditorEdge* edge) {
                           const bool shouldDelete = (edge->getProducer()->getProxy() == proxy) ||
                             (edge->getConsumer()->getProxy() == proxy);
                           if (shouldDelete)
                           {
                             delete edge;
                           }
                           return shouldDelete;
                         }),
      currentEdges.end());
  }

  // delete node
  const auto proxyNode = this->nodeRegistry.find(proxyId);
  delete proxyNode->second;
  this->nodeRegistry.erase(proxyNode);

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

  auto* aView = pqActiveObjects::instance().activeView();
  this->setPortVisibility(port, aView, -1);

  aView->render();

  return 1;
};

// ----------------------------------------------------------------------------
void pqNodeEditorWidget::setPortVisibility(pqOutputPort* port, pqView* pqview, int vis)
{
  const static vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  auto* viewSMProxy = pqview ? static_cast<vtkSMViewProxy*>(pqview->getProxy()) : nullptr;
  if (!viewSMProxy)
  {
    return;
  }

  if (vis < 0)
  {
    vis = controller->GetVisibility(port->getSourceProxy(), port->getPortNumber(), viewSMProxy);
    vis = !static_cast<bool>(vis);
  }

  controller->SetVisibility(
    port->getSourceProxy(), port->getPortNumber(), viewSMProxy, static_cast<bool>(vis));
}

// ----------------------------------------------------------------------------
int pqNodeEditorWidget::hideAllInActiveView()
{
  auto* aView = pqActiveObjects::instance().activeView();

  for (auto nodeIt : this->nodeRegistry)
  {
    auto* proxy = qobject_cast<pqPipelineSource*>(nodeIt.second->getProxy());
    if (proxy == nullptr)
    {
      continue;
    }

    for (int i = 0; i < proxy->getNumberOfOutputPorts(); ++i)
    {
      this->setPortVisibility(proxy->getOutputPort(i), aView, false);
    }
  }

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
      auto* edge = new pqNodeEditorEdge(
        producerIt->second, producerPort->getPortNumber(), consumerIt->second, iPortIdx);
      this->scene->addItem(edge);
      this->scene->addItem(edge->overlay());
      consumerEdgesIt->second.push_back(edge);
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
                     [](pqNodeEditorAnnotationItem* annot) {
                       const bool selected = annot->isSelected();
                       if (selected)
                       {
                         delete annot;
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
