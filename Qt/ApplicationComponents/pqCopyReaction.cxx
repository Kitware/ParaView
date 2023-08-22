// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCopyReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqApplyBehavior.h"
#include "pqObjectBuilder.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqProxySelection.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"
#include "vtkAlgorithm.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyClipboard.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTransferFunctionManager.h"

#include <QPointer>
#include <QSet>

#include <stack>
#include <unordered_map>

// necessary for using QPointer in a QSet
template <class T>
static uint qHash(QPointer<T> p)
{
  return qHash(static_cast<T*>(p));
}

QSet<QPointer<pqProxy>> pqCopyReaction::FilterSelection;
pqProxy* pqCopyReaction::SelectionRoot = nullptr;

QSet<pqCopyReaction*> pqCopyReaction::PastePipelineContainer;
QSet<pqCopyReaction*> pqCopyReaction::CopyPipelineContainer;
QMap<pqProxy*, QMetaObject::Connection> pqCopyReaction::SelectedProxyConnections;

//-----------------------------------------------------------------------------
pqCopyReaction::pqCopyReaction(QAction* parentObject, bool paste_mode, bool pipeline_mode)
  : Superclass(parentObject)
  , Paste(paste_mode)
  , CreatePipeline(pipeline_mode)
{
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnableState()));
  this->updateEnableState();
  if (!this->Paste && this->CreatePipeline)
  {
    for (auto paster : this->PastePipelineContainer)
    {
      QObject::connect(
        this, &pqCopyReaction::pipelineCopied, paster, &pqCopyReaction::updateEnableState);
    }
    this->CopyPipelineContainer.insert(this);
  }
  else if (this->Paste && this->CreatePipeline)
  {
    for (auto copier : this->CopyPipelineContainer)
    {
      QObject::connect(
        copier, &pqCopyReaction::pipelineCopied, this, &pqCopyReaction::updateEnableState);
    }
    this->PastePipelineContainer.insert(this);
  }
}

//-----------------------------------------------------------------------------
pqCopyReaction::~pqCopyReaction()
{
  if (this->Paste && this->CreatePipeline)
  {
    this->PastePipelineContainer.remove(this);
  }
  else if (!this->Paste && this->CreatePipeline)
  {
    this->CopyPipelineContainer.remove(this);
  }
}

//-----------------------------------------------------------------------------
void pqCopyReaction::updateEnableState()
{
  if (this->Paste && !this->CreatePipeline)
  {
    QObject* clipboard = pqApplicationCore::instance()->manager("SOURCE_ON_CLIPBOARD");
    pqPipelineSource* active = pqActiveObjects::instance().activeSource();
    this->parentAction()->setEnabled(
      clipboard != nullptr && active != clipboard && active != nullptr);
  }
  else if (this->Paste && this->CreatePipeline)
  {
    this->parentAction()->setEnabled(this->canPastePipeline());
  }
  else if (!this->Paste && !this->CreatePipeline)
  {
    this->parentAction()->setEnabled(pqActiveObjects::instance().activeSource() != nullptr);
  }
  else
  {
    this->parentAction()->setEnabled(pqCopyReaction::getSelectedPipelineRoot() != nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqCopyReaction::copy()
{
  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
  if (!activeSource)
  {
    qDebug("Could not find an active source to copy to.");
    return;
  }

  // since pqApplicationCore uses QPointer for the managers, we don't have to
  // worry about unregistering the source when it is deleted.
  pqApplicationCore* appCore = pqApplicationCore::instance();
  // need to remove any previous SOURCE_ON_CLIPBOARD else pqApplicationCore
  // warns.
  appCore->unRegisterManager("SOURCE_ON_CLIPBOARD");
  appCore->registerManager("SOURCE_ON_CLIPBOARD", activeSource);
}

//-----------------------------------------------------------------------------
void pqCopyReaction::paste()
{
  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
  pqPipelineSource* clipboard =
    qobject_cast<pqPipelineSource*>(pqApplicationCore::instance()->manager("SOURCE_ON_CLIPBOARD"));
  if (!clipboard)
  {
    qDebug("No source on clipboard to copy from.");
    return;
  }
  pqCopyReaction::copy(activeSource->getProxy(), clipboard->getProxy());
  activeSource->renderAllViews();
}

namespace
{
// checks that source is a downstream filter of some item in selection
bool checkDownstream(QList<pqPipelineSource*>& selection, pqPipelineSource* source)
{
  for (auto src2 : selection)
  {
    if (src2 == source)
    {
      continue;
    }
    auto consumers = src2->getAllConsumers();
    if (consumers.contains(source))
    {
      return true;
    }
    return checkDownstream(consumers, source);
  }

  return false;
}

//-----------------------------------------------------------------------------
// Returns nullptr if there are more than 1 input port to the whole pipeline
pqProxy* getPipelineRoot(const pqProxySelection& sel)
{
  if (sel.empty())
  {
    return nullptr;
  }

  // For each proxy, we count how many input and output ports are connected to proxies within the
  // selection. We want at most one proxy with one input port to not be connected within the
  // selection. Every proxy in the selection should have either all its output ports connected or
  // none.
  // Each proxy is mapped to ninputs
  std::unordered_map<pqProxy*, unsigned int> connections;

  for (pqServerManagerModelItem* item : sel)
  {
    if (auto proxy = qobject_cast<pqProxy*>(item))
    {
      if (auto port = qobject_cast<pqOutputPort*>(proxy))
      {
        connections.emplace(port->getSource(), 0);
      }
      else
      {
        connections.emplace(proxy, 0);
      }
    }
  }

  // Count the number of input connections of each source
  for (auto& pair : connections)
  {
    pqProxy* proxy = pair.first;

    if (auto source = qobject_cast<pqPipelineSource*>(proxy))
    {
      for (int i = 0; i < source->getNumberOfOutputPorts(); ++i)
      {
        pqOutputPort* port = source->getOutputPort(i);
        int noutputs = 0;
        // consumers include lots of other proxies but it will include pipeline ones
        for (int j = 0; j < port->getNumberOfConsumers(); ++j)
        {
          auto it = connections.find(port->getConsumer(j));
          if (it != connections.end())
          {
            ++it->second;
            ++noutputs;
          }
        }
        if (noutputs && noutputs != port->getNumberOfConsumers())
        {
          return nullptr;
        }
      }
    }
  }

  pqProxy* root = nullptr;
  for (auto& pair : connections)
  {
    pqProxy* proxy = pair.first;
    unsigned int ninputs = pair.second;

    if (auto source = qobject_cast<pqPipelineSource*>(proxy))
    {
      if (ninputs && source->getSourceProxy()->GetNumberOfAlgorithmRequiredInputPorts() != ninputs)
      {
        return nullptr;
      }
      else if (!ninputs)
      {
        if (root)
        {
          return nullptr;
        }
        root = proxy;
      }
    }
  }

  return root;
}
} // anonymous namespace

//-----------------------------------------------------------------------------
pqProxy* pqCopyReaction::getSelectedPipelineRoot()
{
  vtkSMProxySelectionModel* selModel = pqActiveObjects::instance().activeSourcesSelectionModel();
  if (!selModel || selModel->GetNumberOfSelectedProxies() == 0)
  {
    return nullptr;
  }

  pqProxySelection selection;
  pqProxySelectionUtilities::copy(selModel, selection);
  return ::getPipelineRoot(selection);
}

//-----------------------------------------------------------------------------
bool pqCopyReaction::canPastePipeline()
{
  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();

  // There is nothing selected
  if (!SelectionRoot)
  {
    return false;
  }

  // If there is no active source selected, we can only paste sources with no input
  if (!activeSource)
  {
    return !SelectionRoot->getProxy()->GetProperty("Input");
  }

  // If the root is a pipeline source, at this point, it needs to have input port
  auto root = qobject_cast<pqPipelineSource*>(SelectionRoot);
  if (root && !root->getSourceProxy()->GetNumberOfAlgorithmRequiredInputPorts())
  {
    return true;
  }

  // We now check if we can stitch the active source to the root of the selection
  // We only support copying pipelines with 1 input port to filters with 1 output port
  if (vtkSMSourceProxy* activeSourceProxy = activeSource->getSourceProxy())
  {
    if (activeSourceProxy->GetNumberOfAlgorithmOutputPorts() == 1)
    {
      if (auto inputProperty =
            vtkSMInputProperty::SafeDownCast(root->getSourceProxy()->GetProperty("Input")))
      {
        if (inputProperty->GetNumberOfProxies() == 1)
        {
          if (auto inputTypes =
                vtkSMDataTypeDomain::SafeDownCast(inputProperty->GetDomain("input_type")))
          {
            if (inputTypes->IsInDomain(activeSourceProxy))
            {
              return true;
            }
          }
        }
      }
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
void pqCopyReaction::copyPipeline()
{
  pqProxy* root = getSelectedPipelineRoot();

  if (!root)
  {
    return;
  }

  SelectionRoot = root;

  vtkSMProxySelectionModel* selModel = pqActiveObjects::instance().activeSourcesSelectionModel();
  if (!selModel || selModel->GetNumberOfSelectedProxies() == 0)
  {
    qDebug("Could not find active sources to copy.");
    return;
  }

  FilterSelection.clear();
  for (auto& connection : SelectedProxyConnections)
  {
    QObject::disconnect(connection);
  }
  SelectedProxyConnections.clear();
  pqProxySelection selection;
  pqProxySelectionUtilities::copy(selModel, selection);
  for (auto item : selection)
  {
    auto proxy = qobject_cast<pqProxy*>(item);
    SelectedProxyConnections.insert(proxy, QObject::connect(proxy, &QObject::destroyed, [proxy] {
      auto& connection = SelectedProxyConnections[proxy];
      QObject::disconnect(connection);
      SelectedProxyConnections.remove(proxy);
      FilterSelection.clear();
      SelectionRoot = nullptr;

      for (auto paster : PastePipelineContainer)
      {
        paster->updateEnableState();
      }
    }));

    FilterSelection.insert(proxy);
  }
}

namespace
{
//-----------------------------------------------------------------------------
void copyDescendants(pqObjectBuilder* builder, pqPipelineSource* source, pqPipelineSource* parent,
  QSet<QPointer<pqProxy>>& selection, int port = 0)
{
  vtkSMSourceProxy* proxy = source->getSourceProxy();

  // Create a copy
  pqPipelineSource* child = parent
    ? builder->createFilter(proxy->GetXMLGroup(), proxy->GetXMLName(), parent, port)
    : builder->createSource(
        proxy->GetXMLGroup(), proxy->GetXMLName(), pqActiveObjects::instance().activeServer());
  selection.remove(source);

  // Copy properties into the new filter
  pqCopyReaction::copy(child->getSourceProxy(), proxy);

  // Run the filter
  child->updatePipeline();

  // Deactivate Apply button for this filter
  child->setModifiedState(pqProxy::UNMODIFIED);

  pqView* view = pqActiveObjects::instance().activeView();

  // Update the representation of the output
  for (int outputPort = 0; outputPort < source->getNumberOfOutputPorts(); ++outputPort)
  {
    pqDataRepresentation* repr =
      builder->createDataRepresentation(child->getOutputPort(outputPort), view);
    if (repr)
    {
      vtkSMColorMapEditorHelper::SetupLookupTable(repr->getProxy());
    }
  }

  // Hide the parent filter if its representation is SURFACE or SURFACE_WITH_EDGES
  if (pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(child))
  {
    pqApplyBehavior::hideInputIfRequired(filter, view);
  }

  // Recurse
  auto consumers = source->getAllConsumers();
  for (int i = 0; i < consumers.size(); ++i)
  {
    auto consumer = consumers[i];
    if (selection.contains(consumer))
    {
      copyDescendants(builder, consumer, child, selection, i);
    }
  }
}
}

//-----------------------------------------------------------------------------
void pqCopyReaction::pastePipeline()
{
  if (!canPastePipeline())
  {
    return;
  }

  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();

  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();

  auto root = qobject_cast<pqPipelineSource*>(SelectionRoot);

  pqPipelineSource* parent =
    root->getSourceProxy()->GetNumberOfAlgorithmRequiredInputPorts() ? activeSource : nullptr;

  auto selection = FilterSelection;
  BEGIN_UNDO_SET(QString("Paste Pipeline"));
  copyDescendants(builder, root, parent, selection);
  END_UNDO_SET();

  if (activeSource)
  {
    activeSource->renderAllViews();
  }
  else if (pqView* view = pqActiveObjects::instance().activeView())
  {
    view->render();
  }
}

//-----------------------------------------------------------------------------
void pqCopyReaction::copy(vtkSMProxy* dest, vtkSMProxy* source)
{
  if (dest && source)
  {
    BEGIN_UNDO_SET(tr("Copy Properties"));

    vtkNew<vtkSMProxyClipboard> clipboard;
    clipboard->Copy(source);
    clipboard->Paste(dest);

    END_UNDO_SET();
  }
}
