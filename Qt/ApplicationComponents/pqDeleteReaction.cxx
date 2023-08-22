// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDeleteReaction.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqOutputWidget.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineFilter.h"
#include "pqProxySelection.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMOutputPort.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewProxy.h"

#include <QDebug>
#include <QMessageBox>
#include <QSet>

//-----------------------------------------------------------------------------
pqDeleteReaction::pqDeleteReaction(QAction* parentObject, DeleteModes mode /* = SELECTED*/)
  : Superclass(parentObject)
{
  // needed to disable server reset while an animation is playing
  QObject::connect(pqPVApplicationCore::instance()->animationManager(),
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::beginPlay), this,
    &pqDeleteReaction::updateEnableState);
  QObject::connect(pqPVApplicationCore::instance()->animationManager(),
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::endPlay), this,
    &pqDeleteReaction::updateEnableState);

  this->DeleteMode = mode;
  if (this->DeleteMode != ALL)
  {
    QObject::connect(&pqActiveObjects::instance(), SIGNAL(portChanged(pqOutputPort*)), this,
      SLOT(updateEnableState()));

    // needed for when you delete the only consumer of an item. The selection
    // signal is emitted before the consumer is removed
    QObject::connect(pqApplicationCore::instance()->getServerManagerObserver(),
      SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)), this,
      SLOT(updateEnableState()));
  }

  if (this->DeleteMode == TREE)
  {
    // needed when dealing with multioutput filters. Without this, it's possible that clicking
    // on one of the outputs in the pipeline browser will not trigger a call to updateEnableState,
    // so ParaView thinks that Delete Tree can be invoked on the output when it actually can't.
    QObject::connect(&pqActiveObjects::instance(), SIGNAL(pipelineProxyChanged(pqProxy*)), this,
      SLOT(updateEnableState()));
    // similar situation, except when trying to call delete tree when the selection contains
    // multiple items
    QObject::connect(&pqActiveObjects::instance(),
      SIGNAL(selectionChanged(const pqProxySelection&)), this, SLOT(updateEnableState()));
  }

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::updateEnableState()
{
  if (pqPVApplicationCore::instance()->animationManager()->animationPlaying())
  {
    this->parentAction()->setEnabled(false);
    return;
  }

  if (this->DeleteMode == ALL)
  {
    this->parentAction()->setEnabled(true);
    return;
  }

  if (this->DeleteMode == TREE)
  {
    this->parentAction()->setEnabled(this->canDeleteTree());
    return;
  }

  this->parentAction()->setEnabled(this->canDeleteSelected());
}

//-----------------------------------------------------------------------------
static void pqDeleteReactionGetSelectedSet(
  vtkSMProxySelectionModel* selModel, QSet<pqProxy*>& selectedSources)
{
  pqProxySelection qselection;
  pqProxySelectionUtilities::copy(selModel, qselection);
  // convert all pqOutputPorts to pqPipelineSources
  qselection = pqProxySelectionUtilities::getPipelineProxies(qselection);
  for (auto& item : qselection)
  {
    if (auto proxy = qobject_cast<pqProxy*>(item))
    {
      selectedSources.insert(proxy);
    }
  }
}

//-----------------------------------------------------------------------------
bool pqDeleteReaction::canDeleteSelected()
{
  vtkSMProxySelectionModel* selModel = pqActiveObjects::instance().activeSourcesSelectionModel();
  if (selModel == nullptr || selModel->GetNumberOfSelectedProxies() == 0)
  {
    return false;
  }

  QSet<pqProxy*> selectedSources;
  ::pqDeleteReactionGetSelectedSet(selModel, selectedSources);

  if (selectedSources.empty())
  {
    return false;
  }

  // Now ensure that all consumers for the current sources don't have consumers
  // outside the selectedSources, then alone can we delete the selected items.
  for (auto item : selectedSources)
  {
    if (auto source = qobject_cast<pqPipelineSource*>(item))
    {
      QList<pqPipelineSource*> consumers = source->getAllConsumers();
      for (auto consumer : consumers)
      {
        if (consumer && !selectedSources.contains(consumer))
        {
          return false;
        }
      }
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteAll()
{
  SM_SCOPED_TRACE(CallFunction).arg("ResetSession");
  BEGIN_UNDO_EXCLUDE();
  if (pqServer* server = pqActiveObjects::instance().activeServer())
  {
    pqApplicationCore::instance()->getObjectBuilder()->resetServer(server);
  }
  if (QWidget* mainWidget = pqCoreUtilities::mainWidget())
  {
    if (pqOutputWidget* output = mainWidget->findChild<pqOutputWidget*>())
    {
      output->clear();
    }
  }
  END_UNDO_EXCLUDE();
  CLEAR_UNDO_STACK();
  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteSelected()
{
  if (!pqDeleteReaction::canDeleteSelected())
  {
    return;
  }

  vtkSMProxySelectionModel* selModel = pqActiveObjects::instance().activeSourcesSelectionModel();

  QSet<pqProxy*> selectedSources;
  ::pqDeleteReactionGetSelectedSet(selModel, selectedSources);
  pqDeleteReaction::deleteSources(selectedSources);
}

//-----------------------------------------------------------------------------
namespace
{

void addSubtreeToSelection(pqPipelineSource* source, pqProxySelection& selection)
{
  if (!source)
  {
    return;
  }

  QList<pqPipelineSource*> consumers = source->getAllConsumers();
  for (auto consumer : consumers)
  {
    if (consumer)
    {
      addSubtreeToSelection(consumer, selection);
      selection.push_back(consumer);
    }
  }
}

} // end anon namespace

void pqDeleteReaction::deleteTree()
{
  if (!pqDeleteReaction::canDeleteTree())
  {
    return;
  }

  vtkSMProxySelectionModel* selModel = pqActiveObjects::instance().activeSourcesSelectionModel();

  // have two pqProxySelection. both start out with the user's selected
  // proxies for deletion, but userSelection is used for iterating to
  // grab children proxes and fullSelection is the list that will be updated
  pqProxySelection userSelection, fullSelection;
  pqProxySelectionUtilities::copy(selModel, userSelection);
  // convert all pqOutputPorts to pqPipelineSources
  userSelection = pqProxySelectionUtilities::getPipelineProxies(userSelection);
  fullSelection = userSelection;

  // for each selection, add its children to the selection
  for (auto iter = userSelection.begin(); iter != userSelection.end(); ++iter)
  {
    auto source = qobject_cast<pqPipelineSource*>(*iter);
    addSubtreeToSelection(source, fullSelection);
  }

  QSet<pqProxy*> selectedSources;
  for (auto& item : fullSelection)
  {
    if (auto proxy = qobject_cast<pqProxy*>(item))
    {
      selectedSources.insert(proxy);
    }
  }
  pqDeleteReaction::deleteSources(selectedSources);
}

//-----------------------------------------------------------------------------
bool pqDeleteReaction::canDeleteTree()
{
  vtkSMProxySelectionModel* selModel = pqActiveObjects::instance().activeSourcesSelectionModel();
  if (selModel == nullptr || selModel->GetNumberOfSelectedProxies() == 0)
  {
    return false;
  }

  // Filters with single input and output -> can always delete
  // Filters with multiple inputs -> can always delete
  // Filters with multiple outputs -> can always delete if the filter is selected,
  // if one of its outputs are selected for deletion instead, do not delete
  for (unsigned int i = 0; i < selModel->GetNumberOfSelectedProxies(); ++i)
  {
    vtkSMOutputPort* outputPort = vtkSMOutputPort::SafeDownCast(selModel->GetSelectedProxy(i));
    if (outputPort)
    {
      vtkSMSourceProxy* source = outputPort->GetSourceProxy();
      if (source->GetNumberOfOutputPorts() > 1)
      {
        return false;
      }
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteSource(pqProxy* source)
{
  if (auto port = qobject_cast<pqOutputPort*>(source))
  {
    pqDeleteReaction::deleteSource(port->getSource());
  }
  else if (source)
  {
    pqDeleteReaction::deleteSources(QSet<pqProxy*>{
      source,
    });
  }
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteSources(const QSet<pqProxy*>& argSources)
{
  if (argSources.empty())
  {
    return;
  }

  QSet<pqProxy*> sources = argSources;

  vtkNew<vtkSMParaViewPipelineController> controller;
  if (sources.size() == 1)
  {
    auto source = (*sources.begin());
    BEGIN_UNDO_SET(tr("Delete %1").arg(source->getSMName()));
  }
  else
  {
    BEGIN_UNDO_SET(tr("Delete Selection"));
  }

  /// loop attempting to delete each source.
  bool something_deleted = false;
  bool something_deleted_in_current_iteration = false;
  do
  {
    something_deleted_in_current_iteration = false;
    for (pqProxy* proxy : sources)
    {
      auto source = qobject_cast<pqPipelineSource*>(proxy);
      if (proxy != nullptr && (source == nullptr || source->getNumberOfConsumers() == 0))
      {
        pqDeleteReaction::aboutToDelete(proxy);
        sources.remove(proxy);
        controller->UnRegisterProxy(proxy->getProxy());
        something_deleted = true;
        something_deleted_in_current_iteration = true;
        break;
      }
    }
  } while (something_deleted_in_current_iteration && !sources.empty());

  // update scalar bars, if needed
  int sbMode = vtkPVGeneralSettings::GetInstance()->GetScalarBarMode();
  if (something_deleted &&
    (sbMode == vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS ||
      sbMode == vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS))
  {
    vtkNew<vtkSMTransferFunctionManager> tmgr;
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    QList<pqView*> views = smmodel->findItems<pqView*>();
    Q_FOREACH (pqView* view, views)
    {
      tmgr->UpdateScalarBars(
        view->getProxy(), vtkSMTransferFunctionManager::HIDE_UNUSED_SCALAR_BARS);
    }
  }

  if (something_deleted)
  {
    // update animation playback mode.
    vtkSMAnimationSceneProxy::UpdateAnimationUsingDataTimeSteps(
      controller->GetAnimationScene(pqActiveObjects::instance().activeServer()->session()));
  }

  END_UNDO_SET();
  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::aboutToDelete(pqProxy* source)
{
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);
  if (!filter)
  {
    return;
  }

  pqOutputPort* firstInput = filter->getAnyInput();
  if (!firstInput)
  {
    return;
  }

  //---------------------------------------------------------------------------
  // Make input the active source is "source" was currently active.
  pqActiveObjects& activeObjects = pqActiveObjects::instance();
  if (activeObjects.activeSource() == source)
  {
    activeObjects.setActivePort(firstInput);
  }

  //---------------------------------------------------------------------------
  // Make input visible if it was hidden in views this source was displayed.
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  QList<pqView*> views = filter->getViews();
  Q_FOREACH (pqView* view, views)
  {
    vtkSMViewProxy* viewProxy = view->getViewProxy();
    if (controller->GetVisibility(filter->getSourceProxy(), 0, viewProxy))
    {
      // this will also hide scalar bars if needed.
      controller->Hide(filter->getSourceProxy(), 0, viewProxy);

      // if firstInput had a representation in this view that was hidden, show
      // it. We don't want to create a new representation, however.
      if (viewProxy->FindRepresentation(firstInput->getSourceProxy(), firstInput->getPortNumber()))
      {
        vtkSMProxy* reprProxy = controller->SetVisibility(
          firstInput->getSourceProxy(), firstInput->getPortNumber(), viewProxy, true);
        // since we turned on input representation, show scalar bar, if the user
        // preference is such.
        if (reprProxy &&
          vtkPVGeneralSettings::GetInstance()->GetScalarBarMode() ==
            vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS &&
          vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy))
        {
          vtkSMColorMapEditorHelper::SetScalarBarVisibility(reprProxy, viewProxy, true);
        }
        view->render();
      }
    }
  }
  //---------------------------------------------------------------------------
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::onTriggered()
{
  if (this->DeleteMode == ALL)
  {
    if (pqCoreUtilities::promptUser("pqDeleteReaction::onTriggered", QMessageBox::Question,
          tr("Delete All?"),
          tr("The current visualization will be reset\n"
             "and the state will be discarded.\n\n"
             "Are you sure you want to continue?"),
          QMessageBox::Yes | QMessageBox::No))
    {
      pqDeleteReaction::deleteAll();
    }
  }
  else if (this->DeleteMode == SELECTED)
  {
    pqDeleteReaction::deleteSelected();
  }
  else
  {
    pqDeleteReaction::deleteTree();
  }
}
