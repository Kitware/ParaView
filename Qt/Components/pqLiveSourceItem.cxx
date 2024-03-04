// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLiveSourceItem.h"

#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqTimer.h"
#include "pqView.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkPVSession.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

#include <QPointer>

//-----------------------------------------------------------------------------
class pqLiveSourceItem::pqInternals
{
public:
  bool IsPaused = false;
  pqTimer Timer;
  QPointer<pqPipelineSource> Source;
  double TimeRange[2];
  bool IsEmulatedTimeAlgorithm;

  static const int DEFAULT_INTERVAL = 100;

  //-----------------------------------------------------------------------------
  pqInternals(pqPipelineSource* src, vtkPVXMLElement* liveHints)
    : Source(QPointer<pqPipelineSource>(src))
  {
    assert(liveHints != nullptr);
    int interval = 0;
    if (!liveHints->GetScalarAttribute("interval", &interval) || interval <= 0)
    {
      interval = pqInternals::DEFAULT_INTERVAL;
    }

    this->Timer.setInterval(interval);

    int emulatedTime = 0;
    liveHints->GetScalarAttribute("emulated_time", &emulatedTime);
    this->IsEmulatedTimeAlgorithm = emulatedTime != 0;
    this->Timer.setSingleShot(true);

    if (this->IsEmulatedTimeAlgorithm)
    {
      this->updateTimeRange();
    }
    this->Timer.start();
  }

  //-----------------------------------------------------------------------------
  void updateTimeRange()
  {
    auto proxy = vtkSMSourceProxy::SafeDownCast(this->Source->getProxy());
    auto session = proxy->GetSession();

    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(proxy) << "GetTimeRange"
           << vtkClientServerStream::End;
    session->ExecuteStream(vtkPVSession::DATA_SERVER_ROOT, stream, /*ignore errors*/ true);

    vtkClientServerStream result = session->GetLastResult(vtkPVSession::DATA_SERVER_ROOT);
    if (result.GetNumberOfMessages() != 1 || result.GetNumberOfArguments(0) != 1 ||
      !result.GetArgument(0, 0, this->TimeRange, 2))
    {
      this->IsEmulatedTimeAlgorithm = false;
    }
  }

  //-----------------------------------------------------------------------------
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
pqLiveSourceItem::pqLiveSourceItem(
  pqPipelineSource* src, vtkPVXMLElement* liveHints, QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqLiveSourceItem::pqInternals(src, liveHints))
{
  src->getProxy()->AddObserver(
    vtkCommand::UpdateInformationEvent, this, &pqLiveSourceItem::onUpdateInformation);

  this->connect(&this->Internals->Timer, &pqTimer::timeout, this, &pqLiveSourceItem::refreshSource);

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(smmodel, &pqServerManagerModel::viewAdded, this, &pqLiveSourceItem::onViewAdded);

  Q_FOREACH (pqView* view, smmodel->findItems<pqView*>())
  {
    this->onViewAdded(view);
  }
  this->update(0);
}

//-----------------------------------------------------------------------------
pqLiveSourceItem::~pqLiveSourceItem() = default;

//-----------------------------------------------------------------------------
void pqLiveSourceItem::update(double time)
{
  this->Internals->Timer.stop();

  if (this->Internals->tryAgainLater())
  {
    this->Internals->Timer.start();
    return;
  }

  auto proxy = vtkSMSourceProxy::SafeDownCast(this->Internals->Source->getProxy());
  auto session = proxy->GetSession();
  vtkClientServerStream stream;

  stream << vtkClientServerStream::Invoke << VTKOBJECT(proxy) << "GetNeedsUpdate";
  if (this->Internals->IsEmulatedTimeAlgorithm)
  {
    stream << time;
  }
  stream << vtkClientServerStream::End;
  session->ExecuteStream(vtkPVSession::DATA_SERVER_ROOT, stream, /*ignore errors*/ true);

  vtkClientServerStream result = session->GetLastResult(vtkPVSession::DATA_SERVER_ROOT);
  bool needs_update = false;
  if (result.GetNumberOfMessages() == 1 && result.GetNumberOfArguments(0) == 1 &&
    result.GetArgument(0, 0, &needs_update) && needs_update)
  {
    proxy->MarkModified(proxy);
    this->Internals->Source->renderAllViews();
  }

  this->Internals->Timer.start();
}

//-----------------------------------------------------------------------------
void pqLiveSourceItem::pause()
{
  if (!this->Internals->IsPaused)
  {
    this->Internals->Timer.stop();
    this->Internals->IsPaused = true;
    Q_EMIT this->stateChanged(true);
  }
}

//-----------------------------------------------------------------------------
void pqLiveSourceItem::resume()
{
  if (this->Internals->IsPaused)
  {
    this->Internals->Timer.start();
    this->Internals->IsPaused = false;
    Q_EMIT this->stateChanged(true);
  }
}

//-----------------------------------------------------------------------------
bool pqLiveSourceItem::isPaused()
{
  return this->Internals->IsPaused;
}

//-----------------------------------------------------------------------------
void pqLiveSourceItem::onUpdateInformation()
{
  if (this->Internals->IsEmulatedTimeAlgorithm)
  {
    this->Internals->updateTimeRange();
    Q_EMIT onInformationUpdated();
  }
}

//-----------------------------------------------------------------------------
double* pqLiveSourceItem::getTimestampRange()
{
  if (this->Internals->IsEmulatedTimeAlgorithm)
  {
    return this->Internals->TimeRange;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
bool pqLiveSourceItem::isEmulatedTimeAlgorithm()
{
  return this->Internals->IsEmulatedTimeAlgorithm;
}

//-----------------------------------------------------------------------------
vtkTypeUInt32 pqLiveSourceItem::getSourceId()
{
  auto smProxy = this->Internals->Source->getProxy();
  return smProxy ? smProxy->GetGlobalID() : 0;
}

//-----------------------------------------------------------------------------
void pqLiveSourceItem::onViewAdded(pqView* view)
{
  if (auto viewProxy = vtkSMViewProxy::SafeDownCast(view->getProxy()))
  {
    if (auto iren = viewProxy->GetInteractor())
    {
      iren->AddObserver(
        vtkCommand::StartInteractionEvent, this, &pqLiveSourceItem::startInteractionEvent);
      iren->AddObserver(
        vtkCommand::EndInteractionEvent, this, &pqLiveSourceItem::endInteractionEvent);
    }
  }
}

//-----------------------------------------------------------------------------
void pqLiveSourceItem::startInteractionEvent()
{
  this->Internals->Timer.stop();
}

//-----------------------------------------------------------------------------
void pqLiveSourceItem::endInteractionEvent()
{
  if (!this->Internals->IsPaused)
  {
    this->Internals->Timer.start();
  }
}
