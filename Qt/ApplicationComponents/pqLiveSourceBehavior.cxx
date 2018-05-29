/*=========================================================================

   Program: ParaView
   Module:  pqLiveSourceBehavior.cxx

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
#include "pqLiveSourceBehavior.h"

#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqTimer.h"
#include "pqView.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

#include <QPointer>

//-----------------------------------------------------------------------------
class pqLiveSourceBehavior::pqInternals
{
public:
  pqTimer Timer;
  QList<QPointer<pqPipelineSource> > LiveSources;

  pqInternals()
  {
    this->Timer.setInterval(100);
    this->Timer.setSingleShot(true);
  }

  void addSource(pqPipelineSource* src, vtkPVXMLElement* liveHints)
  {
    Q_ASSERT(liveHints != nullptr);
    Q_UNUSED(liveHints);
    this->LiveSources.push_back(src);
    this->Timer.start();
  }

  void pause() { this->Timer.stop(); }

  void resume()
  {
    this->LiveSources.removeAll(nullptr);
    if (this->LiveSources.size() > 0)
    {
      this->Timer.start();
    }
  }

  void update()
  {
    this->Timer.stop();
    this->LiveSources.removeAll(nullptr);
    if (this->LiveSources.size() == 0)
    {
      return;
    }

    if (this->tryAgainLater())
    {
      this->Timer.start();
      return;
    }

    // iterate over all sources and update those that need updating.
    for (pqPipelineSource* src : this->LiveSources)
    {
      auto proxy = vtkSMSourceProxy::SafeDownCast(src->getProxy());
      auto session = proxy->GetSession();

      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke << VTKOBJECT(proxy) << "GetNeedsUpdate"
             << vtkClientServerStream::End;
      session->ExecuteStream(vtkPVSession::DATA_SERVER_ROOT, stream, /*ignore errors*/ true);

      vtkClientServerStream result = session->GetLastResult(vtkPVSession::DATA_SERVER_ROOT);
      bool needs_update = false;
      if (result.GetNumberOfMessages() == 1 && result.GetNumberOfArguments(0) == 1 &&
        result.GetArgument(0, 0, &needs_update) && needs_update)
      {
        proxy->MarkModified(proxy);
        src->renderAllViews();
      }
    }

    this->Timer.start();
  }

private:
  bool tryAgainLater()
  {
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    auto servers = smmodel->findItems<pqServer*>();
    for (auto server : servers)
    {
      if (server->isProcessingPending() || server->session()->GetPendingProgress())
      {
        return true;
      }
    }
    return false;
  }
};

//-----------------------------------------------------------------------------
pqLiveSourceBehavior::pqLiveSourceBehavior(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqLiveSourceBehavior::pqInternals())
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(smmodel, SIGNAL(viewAdded(pqView*)), SLOT(viewAdded(pqView*)));
  this->connect(
    smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), SLOT(sourceAdded(pqPipelineSource*)));

  this->connect(&this->Internals->Timer, SIGNAL(timeout()), SLOT(timeout()));

  foreach (pqView* view, smmodel->findItems<pqView*>())
  {
    this->viewAdded(view);
  }

  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    this->sourceAdded(src);
  }
}

//-----------------------------------------------------------------------------
pqLiveSourceBehavior::~pqLiveSourceBehavior()
{
}

//-----------------------------------------------------------------------------
void pqLiveSourceBehavior::viewAdded(pqView* view)
{
  if (auto viewProxy = vtkSMViewProxy::SafeDownCast(view->getProxy()))
  {
    if (auto iren = viewProxy->GetInteractor())
    {
      iren->AddObserver(
        vtkCommand::StartInteractionEvent, this, &pqLiveSourceBehavior::startInteractionEvent);
      iren->AddObserver(
        vtkCommand::EndInteractionEvent, this, &pqLiveSourceBehavior::endInteractionEvent);
    }
  }
}

//-----------------------------------------------------------------------------
void pqLiveSourceBehavior::sourceAdded(pqPipelineSource* src)
{
  vtkSMProxy* proxy = src->getProxy();
  if (auto hints = proxy->GetHints())
  {
    if (auto liveHints = hints->FindNestedElementByName("LiveSource"))
    {
      this->Internals->addSource(src, liveHints);
    }
  }
}

//-----------------------------------------------------------------------------
void pqLiveSourceBehavior::startInteractionEvent()
{
  this->Internals->pause();
}

//-----------------------------------------------------------------------------
void pqLiveSourceBehavior::endInteractionEvent()
{
  this->Internals->resume();
}

//-----------------------------------------------------------------------------
void pqLiveSourceBehavior::timeout()
{
  this->Internals->update();
}
