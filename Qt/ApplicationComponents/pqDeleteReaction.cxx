/*=========================================================================

   Program: ParaView
   Module:    pqDeleteReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqDeleteReaction.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
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
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewProxy.h"

#include <QDebug>
#include <QMessageBox>
#include <QSet>

//-----------------------------------------------------------------------------
pqDeleteReaction::pqDeleteReaction(QAction* parentObject, bool delete_all)
  : Superclass(parentObject)
{
  // needed to disable server reset while an animation is playing
  QObject::connect(pqPVApplicationCore::instance()->animationManager(), SIGNAL(beginPlay()), this,
    SLOT(updateEnableState()));
  QObject::connect(pqPVApplicationCore::instance()->animationManager(), SIGNAL(endPlay()), this,
    SLOT(updateEnableState()));

  this->DeleteAll = delete_all;
  if (!this->DeleteAll)
  {
    QObject::connect(&pqActiveObjects::instance(), SIGNAL(portChanged(pqOutputPort*)), this,
      SLOT(updateEnableState()));

    // needed for when you delete the only consumer of an item. The selection
    // signal is emitted before the consumer is removed
    QObject::connect(pqApplicationCore::instance()->getServerManagerObserver(),
      SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)), this,
      SLOT(updateEnableState()));
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

  if (this->DeleteAll)
  {
    this->parentAction()->setEnabled(true);
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

  if (selectedSources.size() == 0)
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
  if (argSources.size() == 0)
  {
    return;
  }

  QSet<pqProxy*> sources = argSources;

  vtkNew<vtkSMParaViewPipelineController> controller;
  if (sources.size() == 1)
  {
    auto source = (*sources.begin());
    BEGIN_UNDO_SET(QString("Delete %1").arg(source->getSMName()));
  }
  else
  {
    BEGIN_UNDO_SET("Delete Selection");
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
  } while (something_deleted_in_current_iteration && (sources.size() > 0));

  // update scalar bars, if needed
  int sbMode = vtkPVGeneralSettings::GetInstance()->GetScalarBarMode();
  if (something_deleted &&
    (sbMode == vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS ||
        sbMode == vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS))
  {
    vtkNew<vtkSMTransferFunctionManager> tmgr;
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    QList<pqView*> views = smmodel->findItems<pqView*>();
    foreach (pqView* view, views)
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
  foreach (pqView* view, views)
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
        vtkSMProxy* repr = controller->SetVisibility(
          firstInput->getSourceProxy(), firstInput->getPortNumber(), viewProxy, true);
        // since we turned on input representation, show scalar bar, if the user
        // preference is such.
        if (repr &&
          vtkPVGeneralSettings::GetInstance()->GetScalarBarMode() ==
            vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS &&
          vtkSMPVRepresentationProxy::GetUsingScalarColoring(repr))
        {
          vtkSMPVRepresentationProxy::SetScalarBarVisibility(repr, viewProxy, true);
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
  if (this->DeleteAll)
  {
    if (pqCoreUtilities::promptUser("pqDeleteReaction::onTriggered", QMessageBox::Question,
          "Delete All?", tr("The current visualization will be reset \n"
                            "and the state will be discarded.\n\n"
                            "Are you sure you want to continue?"),
          QMessageBox::Yes | QMessageBox::No))
    {
      pqDeleteReaction::deleteAll();
    }
  }
  else
  {
    pqDeleteReaction::deleteSelected();
  }
}
