// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqViewStreamingBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkCommand.h"
#include "vtkPVView.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"

// #define PV_DEBUG_STREAMING
#include "vtkPVStreamingMacros.h"

static const int PQ_STREAMING_LONG_INTERVAL = 1000;
static const int PQ_STREAMING_SHORT_INTERVAL = 0;

//-----------------------------------------------------------------------------
pqViewStreamingBehavior::pqViewStreamingBehavior(QObject* parentObject)
  : Superclass(parentObject)
  , Pass(0)
  , DelayUpdate(false)
  , DisableAutomaticUpdates(false)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(viewAdded(pqView*)), this, SLOT(onViewAdded(pqView*)));

  QObject::connect(&this->Timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
  this->Timer.setSingleShot(true);

  Q_FOREACH (pqView* view, smmodel->findItems<pqView*>())
  {
    this->onViewAdded(view);
  }
}

//-----------------------------------------------------------------------------
pqViewStreamingBehavior::~pqViewStreamingBehavior() = default;

//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::onViewAdded(pqView* view)
{
  vtkSMRenderViewProxy* rvProxy = vtkSMRenderViewProxy::SafeDownCast(view->getProxy());
  if (rvProxy)
  {
    rvProxy->AddObserver(
      vtkCommand::UpdateDataEvent, this, &pqViewStreamingBehavior::onViewUpdated);
    rvProxy->GetInteractor()->AddObserver(
      vtkCommand::StartInteractionEvent, this, &pqViewStreamingBehavior::onStartInteractionEvent);
    rvProxy->GetInteractor()->AddObserver(
      vtkCommand::EndInteractionEvent, this, &pqViewStreamingBehavior::onEndInteractionEvent);
  }
}
//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::onViewUpdated(vtkObject* vtkNotUsed(caller), unsigned long, void*)
{
  // every time the view "updates", we may have to stream new data and hence we
  // restart the streaming loop.
  if (vtkPVView::GetEnableStreaming())
  {
    vtkStreamingStatusMacro("View updated. Restarting streaming loop.");
    this->Pass = 0;
    if (!this->DisableAutomaticUpdates)
    {
      this->Timer.start(PQ_STREAMING_LONG_INTERVAL);
    }
  }
}

//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::onStartInteractionEvent()
{
  this->DelayUpdate = true;
  if (this->Timer.isActive())
  {
    vtkStreamingStatusMacro("Pausing updates while interacting.");
  }
}

//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::onEndInteractionEvent()
{
  this->DelayUpdate = false;
  if (this->Timer.isActive())
  {
    vtkStreamingStatusMacro("Resuming updates since done interacting.");
  }
  else
  {
    if (vtkPVView::GetEnableStreaming())
    {
      this->Timer.start(PQ_STREAMING_SHORT_INTERVAL);
      vtkStreamingStatusMacro("View interaction changed. Restart streaming loop.");
    }
  }
}

//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::onTimeout()
{
  pqView* view = pqActiveObjects::instance().activeView();
  if (view)
  {
    vtkSMRenderViewProxy* rvProxy = vtkSMRenderViewProxy::SafeDownCast(view->getProxy());
    if (rvProxy == nullptr)
    {
      // Not the valid active view. Then do nothing
      return;
    }

    if (!rvProxy)
    {
      return;
    }

    if (rvProxy->GetSession()->GetPendingProgress() || view->getServer()->isProcessingPending() ||
      this->DelayUpdate)
    {
      this->Timer.start(PQ_STREAMING_SHORT_INTERVAL);
    }
    else
    {
      vtkStreamingStatusMacro("Update Pass: " << this->Pass);
      bool to_continue = rvProxy->StreamingUpdate(true);
      if (to_continue)
      {
        this->Pass++;
        if (this->DisableAutomaticUpdates)
        {
          vtkStreamingStatusMacro("Pausing, since automatic updates are disabled.");
        }
        else
        {
          this->Timer.start(0);
        }
      }
      else
      {
        vtkStreamingStatusMacro("Finished. Stopping loop.");
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::stopAutoUpdates()
{
  vtkStreamingStatusMacro("Pausing automatic updates.");
  this->Timer.stop();
  this->DisableAutomaticUpdates = true;
}

//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::resumeAutoUpdates()
{
  vtkStreamingStatusMacro("Resuming automatic updates.");
  this->Timer.start(PQ_STREAMING_LONG_INTERVAL);
  this->DisableAutomaticUpdates = false;
}

//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::triggerSingleUpdate()
{
  vtkStreamingStatusMacro("Trigger single automatic updates.");
  this->onTimeout();
}
