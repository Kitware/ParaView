/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqViewStreamingBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreTestUtility.h"
#include "pqEventPlayer.h"
#include "pqWidgetEventPlayer.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkCommand.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVView.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"

// #define PV_DEBUG_STREAMING
#include "vtkPVStreamingMacros.h"

static const int PQ_STREAMING_INTERVAL=1000;

class pqStreamingTestingEventPlayer : public pqWidgetEventPlayer
{
  typedef pqWidgetEventPlayer Superclass;
public:
  QPointer<pqViewStreamingBehavior> StreamingBehavior;

  pqStreamingTestingEventPlayer(QObject* p) : Superclass(p) { }
  virtual bool playEvent(QObject*,
    const QString& command, const QString& arguments, bool& error)
    {
    if (command == "pqViewStreamingBehavior" && this->StreamingBehavior)
      {
      if (arguments == "stop_streaming")
        {
        this->StreamingBehavior->stopAutoUpdates();
        }
      else if (arguments == "resume_streaming")
        {
        this->StreamingBehavior->resumeAutoUpdates();
        }
      else if (arguments == "next")
        {
        this->StreamingBehavior->triggerSingleUpdate();
        }
      else
        {
        error = true;
        }
      return true;
      }
    else
      {
      return false;
      }
    }
};

//-----------------------------------------------------------------------------
pqViewStreamingBehavior::pqViewStreamingBehavior(QObject* parentObject)
  : Superclass(parentObject), Pass(0), DelayUpdate(false), DisableAutomaticUpdates(false)
{
  pqStreamingTestingEventPlayer* player = new pqStreamingTestingEventPlayer(NULL);
  player->StreamingBehavior = this;

  // the testUtility takes ownership of the player.
  pqApplicationCore::instance()->testUtility()->eventPlayer()->addWidgetEventPlayer(
    player);

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(viewAdded(pqView*)),
    this, SLOT(onViewAdded(pqView*)));

  QObject::connect(&this->Timer, SIGNAL(timeout()),
    this, SLOT(onTimeout()));
  this->Timer.setSingleShot(true);

  foreach (pqView* view, smmodel->findItems<pqView*>())
    {
    this->onViewAdded(view);
    }
}

//-----------------------------------------------------------------------------
pqViewStreamingBehavior::~pqViewStreamingBehavior()
{
}

//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::onViewAdded(pqView* view)
{
  vtkSMRenderViewProxy* rvProxy =
    vtkSMRenderViewProxy::SafeDownCast(view->getProxy());
  if (rvProxy)
    {
    rvProxy->AddObserver(vtkCommand::UpdateDataEvent,
      this, &pqViewStreamingBehavior::onViewUpdated);
    rvProxy->GetInteractor()->AddObserver(
      vtkCommand::StartInteractionEvent,
      this, &pqViewStreamingBehavior::onStartInteractionEvent);
    rvProxy->GetInteractor()->AddObserver(
      vtkCommand::EndInteractionEvent,
      this, &pqViewStreamingBehavior::onEndInteractionEvent);
    }
}
//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::onViewUpdated(
  vtkObject* vtkNotUsed(caller), unsigned long, void*)
{
  // every time the view "updates", we may have to stream new data and hence we
  // restart the streaming loop.
  if (vtkPVView::GetEnableStreaming())
    {
    vtkStreamingStatusMacro("View updated. Restarting streaming loop.");
    this->Pass = 0;
    if (!this->DisableAutomaticUpdates)
      {
      this->Timer.start(PQ_STREAMING_INTERVAL);
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
      this->Timer.start(PQ_STREAMING_INTERVAL);
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
    vtkSMRenderViewProxy* rvProxy = vtkSMRenderViewProxy::SafeDownCast(
      view->getProxy());
    if(rvProxy == NULL)
      {
      // Not the valid active view. Then do nothing
      return;
      }

    if (!rvProxy)
      {
      return;
      }

    if (rvProxy->GetSession()->GetPendingProgress() ||
      view->getServer()->isProcessingPending() || this->DelayUpdate)
      {
      this->Timer.start(PQ_STREAMING_INTERVAL);
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
  this->Timer.start(PQ_STREAMING_INTERVAL);
  this->DisableAutomaticUpdates = false;
}

//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::triggerSingleUpdate()
{
  vtkStreamingStatusMacro("Trigger single automatic updates.");
  this->onTimeout();
}
