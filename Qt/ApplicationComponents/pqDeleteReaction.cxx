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
#include "pqApplicationCore.h"
#include "pqDeleteBehavior.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMTransferFunctionManager.h"

#include <QDebug>
#include <QSet>

//-----------------------------------------------------------------------------
pqDeleteReaction::pqDeleteReaction(QAction* parentObject, bool delete_all)
  : Superclass(parentObject)
{
  this->DeleteAll = delete_all;
  if (!this->DeleteAll)
    {
    QObject::connect(&pqActiveObjects::instance(),
      SIGNAL(portChanged(pqOutputPort*)),
      this, SLOT(updateEnableState()));

    //needed for when you delete the only consumer of an item. The selection
    //signal is emitted before the consumer is removed
    QObject::connect(pqApplicationCore::instance()->getServerManagerObserver(),
      SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*) ),
      this, SLOT(updateEnableState()) );
    }

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::updateEnableState()
{
  if (this->DeleteAll)
    {
    this->parentAction()->setEnabled(true);
    return;
    }

  this->parentAction()->setEnabled(this->canDeleteSelected());
}

//-----------------------------------------------------------------------------
static void pqDeleteReactionGetSelectedSet(
  vtkSMProxySelectionModel* selModel,
  QSet<pqPipelineSource*>& selectedSources)
{
  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  for (unsigned int cc=0; cc < selModel->GetNumberOfSelectedProxies(); cc++)
    {
    vtkSMProxy* proxy = selModel->GetSelectedProxy(cc);
    pqServerManagerModelItem* item =
      smmodel->findItem<pqServerManagerModelItem*>(proxy);
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = port? port->getSource():
      qobject_cast<pqPipelineSource*>(item);
    if (source)
      {
      selectedSources.insert(source);
      }
    }
}

//-----------------------------------------------------------------------------
bool pqDeleteReaction::canDeleteSelected()
{
  vtkSMProxySelectionModel* selModel =
    pqActiveObjects::instance().activeSourcesSelectionModel();
  if (selModel == NULL || selModel->GetNumberOfSelectedProxies() == 0)
    {
    return false;
    }

  QSet<pqPipelineSource*> selectedSources;
  ::pqDeleteReactionGetSelectedSet(selModel, selectedSources);

  if (selectedSources.size() == 0)
    {
    return false;
    }

  // Now ensure that all consumers for the current sources don't have consumers
  // outside the selectedSources, then alone can we delete the selected items.
  foreach (pqPipelineSource* source, selectedSources)
    {
    QList<pqPipelineSource*> consumers = source->getAllConsumers();
    for (int cc=0; cc < consumers.size(); cc++)
      {
      pqPipelineSource* consumer = consumers[cc];
      if (consumer && !selectedSources.contains(consumer))
        {
        return false;
        }
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteAll()
{
  BEGIN_UNDO_EXCLUDE();
  if (pqServer* server = pqActiveObjects::instance().activeServer())
    {
    vtkNew<vtkSMParaViewPipelineController> controller;
    controller->ResetSession(server->session());
    }
  END_UNDO_EXCLUDE();
  CLEAR_UNDO_STACK();
  pqApplicationCore::instance()->render();
}

//=============================================================================
namespace
{
  void pqDeleteSources(QSet<pqPipelineSource*> selectedSources)
    {
    vtkNew<vtkSMParaViewPipelineController> controller;
    if (selectedSources.size() == 1)
      {
      pqPipelineSource* source = (*selectedSources.begin());
      BEGIN_UNDO_SET(QString("Delete %1").arg(source->getSMName()));
      }
    else
      {
      BEGIN_UNDO_SET("Delete Selection");
      }

    while (selectedSources.size() > 0)
      {
      foreach (pqPipelineSource* source, selectedSources)
        {
        if (source && source->getNumberOfConsumers() == 0)
          {
          selectedSources.remove(source);
          controller->UnRegisterProxy(source->getProxy());
          break;
          }
        }
      }

    // update scalar bars, if needed.
    int sbMode = vtkPVGeneralSettings::GetInstance()->GetScalarBarMode();
    if (sbMode == vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS ||
      sbMode == vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS)
      {
      vtkNew<vtkSMTransferFunctionManager> tmgr;
      pqServerManagerModel* smmodel =
        pqApplicationCore::instance()->getServerManagerModel();
      QList<pqView*> views = smmodel->findItems<pqView*>();
      foreach (pqView* view, views)
        {
        tmgr->UpdateScalarBars(view->getProxy(),
          vtkSMTransferFunctionManager::HIDE_UNUSED_SCALAR_BARS);
        }
      }
    END_UNDO_SET();
    pqApplicationCore::instance()->render();
    }
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteSelected()
{
  if (!pqDeleteReaction::canDeleteSelected())
    {
    qCritical() << "Cannot delete selected ";
    return;
    }

  vtkSMProxySelectionModel* selModel =
    pqActiveObjects::instance().activeSourcesSelectionModel();

  QSet<pqPipelineSource*> selectedSources;
  ::pqDeleteReactionGetSelectedSet(selModel, selectedSources);
  pqDeleteSources(selectedSources);
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteSource(pqPipelineSource* source)
{
  if (source)
    {
    // not entirely sure if this is where we should put this logic.
    pqDeleteBehavior temp;
    temp.removeSource(source);
    QSet<pqPipelineSource*> sources;
    sources.insert(source);
    pqDeleteSources(sources);
    }
}
