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
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkCommand.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVStreamingMacros.h"
#include "vtkPVView.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"

//-----------------------------------------------------------------------------
pqViewStreamingBehavior::pqViewStreamingBehavior(QObject* parentObject)
  : Superclass(parentObject), Pass(0), DelayUpdate(false)
{
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
  if (vtkSMRenderViewProxy::SafeDownCast(view->getProxy()))
    {
    view->getProxy()->AddObserver(
      vtkCommand::UpdateDataEvent,
      this, &pqViewStreamingBehavior::onViewUpdated);
    }
}
//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::onViewUpdated(
  vtkObject* caller, unsigned long, void*)
{
  // every time the view "updates", we may have to stream new data and hence we
  // restart the streaming loop.
  vtkSMRenderViewProxy* rvProxy = vtkSMRenderViewProxy::SafeDownCast(caller);
  if (vtkPVView::GetEnableStreaming())
    {
    vtkStreamingStatusMacro("View updated. Restarting streaming loop.");
    this->Pass = 0;
    this->Timer.start(1000);
    rvProxy->GetInteractor()->AddObserver(
      vtkCommand::StartInteractionEvent,
      this, &pqViewStreamingBehavior::onStartInteractionEvent);
    rvProxy->GetInteractor()->AddObserver(
      vtkCommand::EndInteractionEvent,
      this, &pqViewStreamingBehavior::onEndInteractionEvent);
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
}

//-----------------------------------------------------------------------------
void pqViewStreamingBehavior::onTimeout()
{
  pqView* view = pqActiveObjects::instance().activeView();
  if (view)
    {
    vtkSMRenderViewProxy* rvProxy = vtkSMRenderViewProxy::SafeDownCast(
      view->getProxy());

    if (rvProxy->GetSession()->GetPendingProgress() ||
      view->getServer()->isProcessingPending() || this->DelayUpdate)
      {
      this->Timer.start(1000);
      }
    else
      {
      vtkStreamingStatusMacro("Update Pass: " << this->Pass);
      bool to_continue = rvProxy->StreamingUpdate(true);
      if (to_continue)
        {
        this->Pass++;
        this->Timer.start(0);
        }
      else
        {
        vtkStreamingStatusMacro("Finished. Stopping loop.");
        }
      }
    }
}
