/*=========================================================================

   Program: ParaView
   Module:  pqTimeInspectorWidget.cxx

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
#include "pqTimeInspectorWidget.h"
#include "ui_pqTimeInspectorWidget.h"

#include "pqActiveObjects.h"
#include "pqAnimationModel.h"
#include "pqAnimationTrack.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqPropertyLinks.h"
#include "pqPropertyLinksConnection.h"
#include "pqProxy.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkCommand.h"
#include "vtkCompositeAnimationPlayer.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"

#include <QHeaderView>
#include <QLineF>
#include <QPainter>
#include <QVariant>
#include <QtDebug>

#include <cassert>

namespace
{
pqProxy* getPQProxy(vtkSMProxy* proxy)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  return smmodel->findItem<pqProxy*>(proxy);
}
}

class pqTimeInspectorWidget::PropertyLinksConnection : public pqPropertyLinksConnection
{
  typedef pqPropertyLinksConnection Superclass;

public:
  PropertyLinksConnection(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex, bool use_unchecked_modified_event,
    QObject* parentObject = 0)
    : Superclass(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
  }
  ~PropertyLinksConnection() override {}
protected:
  /// These are the methods that subclasses can override to customize how
  /// values are updated in either directions.
  void setServerManagerValue(bool use_unchecked, const QVariant& value) override
  {
    assert(use_unchecked == false);
    (void)use_unchecked;

    std::vector<vtkSMProxy*> proxies;
    foreach (const QVariant& var, value.toList())
    {
      vtkSMProxy* aproxy = reinterpret_cast<vtkSMProxy*>(var.value<void*>());
      proxies.push_back(aproxy);
    }
    proxies.push_back(NULL);
    assert(proxies.size() > 0);
    vtkSMPropertyHelper(this->propertySM())
      .Set(&proxies[0], static_cast<unsigned int>(proxies.size() - 1));
  }

  QVariant currentServerManagerValue(bool use_unchecked) const override
  {
    assert(use_unchecked == false);
    (void)use_unchecked;
    QList<QVariant> value;

    vtkSMPropertyHelper helper(this->propertySM());
    for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; cc++)
    {
      value.push_back(QVariant::fromValue<void*>(helper.GetAsProxy(cc)));
    }
    return value;
  }
};

class pqTimeInspectorWidget::TimeTrack : public pqAnimationTrack
{
  typedef pqAnimationTrack Superclass;
  vtkSmartPointer<vtkSMProxy> Source;
  unsigned long ObserverId1;
  unsigned long ObserverId2;
  std::vector<double> Markers;
  bool HasDataTime;
  double DataTime;

public:
  TimeTrack(vtkSMProxy* sourceProxy, QObject* parentObj = NULL)
    : Superclass(parentObj)
    , Source(sourceProxy)
    , HasDataTime(false)
    , DataTime(0.0)
  {
    this->ObserverId1 = sourceProxy->AddObserver(
      vtkCommand::UpdateInformationEvent, this, &TimeTrack::updateTimeSteps);
    this->ObserverId2 =
      sourceProxy->AddObserver(vtkCommand::UpdateDataEvent, this, &TimeTrack::updateTime);
    this->updateTimeSteps();
  }
  ~TimeTrack() override
  {
    this->Source->RemoveObserver(this->ObserverId1);
    this->Source->RemoveObserver(this->ObserverId2);
  }

  vtkSMProxy* source() const { return this->Source.GetPointer(); }

  pqAnimationModel* animationModel() const
  {
    pqAnimationModel* amodel = qobject_cast<pqAnimationModel*>(this->parent());
    assert(amodel);
    return amodel;
  }

protected:
  void paint(QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget) override
  {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // draw border for this track
    p->save();
    p->setBrush(QBrush(QColor(255, 255, 255)));
    QPen pen(QColor(0, 0, 0));
    pen.setWidth(0);
    p->setPen(pen);

    QRectF trackRectF = this->boundingRect();
    p->drawRect(trackRectF);

    pen.setColor(QColor("blue"));
    p->setPen(pen);

    QLineF horzLine(trackRectF.left(), trackRectF.top() + trackRectF.height() / 2,
      trackRectF.left() + trackRectF.width(), trackRectF.top() + trackRectF.height() / 2);
    p->drawLine(horzLine);

    pen.setWidth(2);
    p->setPen(pen);

    pqAnimationModel* model = this->animationModel();
    foreach (double mark, this->Markers)
    {
      if (mark >= model->startTime() && mark <= model->endTime())
      {
        mark = (mark - model->startTime()) / (model->endTime() - model->startTime());
        mark = trackRectF.left() + mark * trackRectF.width();
        QLineF line(mark, trackRectF.top() + 10, mark, trackRectF.top() + trackRectF.height() - 10);
        p->drawLine(line);
      }
    }
    if (this->HasDataTime)
    {
      if (this->DataTime >= model->startTime() && this->DataTime <= model->endTime())
      {
        double time =
          (this->DataTime - model->startTime()) / (model->endTime() - model->startTime());
        time = trackRectF.left() + time * trackRectF.width();
        QLineF line(time, trackRectF.top() + 3, time, trackRectF.top() + trackRectF.height() - 3);
        pen.setColor(QColor("green"));
        p->setPen(pen);
        p->drawLine(line);
      }
    }
    p->restore();
  }

private:
  void updateTimeSteps()
  {
    this->Markers = vtkSMPropertyHelper(this->Source, "TimestepValues",
                      /*quiet*/ true)
                      .GetDoubleArray();
    pqAnimationModel* model = this->animationModel();
    model->update();
  }

  void updateTime()
  {
    vtkPVDataInformation* dinfo =
      vtkSMSourceProxy::SafeDownCast(this->Source)->GetDataInformation(0);
    if (dinfo->GetHasTime())
    {
      this->HasDataTime = true;
      this->DataTime = dinfo->GetTime();
    }
    else
    {
      this->HasDataTime = false;
    }
    pqAnimationModel* model = this->animationModel();
    model->update();
  }

  Q_DISABLE_COPY(TimeTrack)
};

class pqTimeInspectorWidget::pqInternals
{
public:
  void* VoidServer;
  QPointer<pqServer> Server;
  Ui::TimeInspectorWidget Ui;
  pqPropertyLinks Links;
  QList<QVariant> TimeSources;
  QList<QVariant> SuppressedTimeSources;

  pqInternals(pqTimeInspectorWidget* self)
    : VoidServer(NULL)
  {
    this->Ui.setupUi(self);
    this->Ui.AnimationWidget->createDeleteHeader()->hide();
    this->Ui.AnimationWidget->animationModel()->setInteractive(true);
    this->Ui.AnimationWidget->animationModel()->setEnabledHeaderToolTip(
      "<p>Uncheck to ignore timesteps from animation.</p>");
  }
};

//-----------------------------------------------------------------------------
pqTimeInspectorWidget::pqTimeInspectorWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqTimeInspectorWidget::pqInternals(this))
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(smmodel, SIGNAL(nameChanged(pqServerManagerModelItem*)),
    SLOT(handleProxyNameChanged(pqServerManagerModelItem*)));

  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  this->connect(animationModel, SIGNAL(currentTimeSet(double)), SLOT(setSceneCurrentTime(double)));

  this->connect(this->Internals->Ui.AnimationWidget, SIGNAL(enableTrackClicked(pqAnimationTrack*)),
    SLOT(toggleTrackSuppression(pqAnimationTrack*)));

  this->connect(
    &pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)), SLOT(setServer(pqServer*)));
  this->setServer(pqActiveObjects::instance().activeServer());

  pqCoreUtilities::connect(vtkPVGeneralSettings::GetInstance(), vtkCommand::ModifiedEvent, this,
    SLOT(generalSettingsChanged()));
}

//-----------------------------------------------------------------------------
pqTimeInspectorWidget::~pqTimeInspectorWidget()
{
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::setServer(pqServer* aserver)
{
  if (aserver != this->Internals->VoidServer)
  {
    this->Internals->VoidServer = aserver;
    this->Internals->Server = aserver;

    this->updateScene();
  }
}

//-----------------------------------------------------------------------------
pqServer* pqTimeInspectorWidget::server() const
{
  return this->Internals->Server;
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::updateScene()
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);

  pqPropertyLinks& links = this->Internals->Links;
  links.clear();

  pqServer* curServer = this->server();
  if (!curServer)
  {
    animationModel->setStartTime(0.0);
    animationModel->setEndTime(1.0);
    animationModel->setMode(pqAnimationModel::Sequence);
    this->Internals->Ui.AnimationTimeWidget->setAnimationScene(NULL);
    // FIXME: remove all tracks.
    return;
  }

  vtkSMSession* session = curServer->session();
  vtkNew<vtkSMParaViewPipelineController> controller;

  vtkSMProxy* sceneProxy = controller->FindAnimationScene(session);
  if (!sceneProxy)
  {
    // The sceneProxy may be null when cleaning up a session. Hence,
    // we handle that case.
    return;
  }

  this->Internals->Ui.AnimationTimeWidget->setAnimationScene(sceneProxy);

  links.addTraceablePropertyLink(this, "sceneStartTime", SIGNAL(dummySignal()), sceneProxy,
    sceneProxy->GetProperty("StartTime"));
  links.addTraceablePropertyLink(
    this, "sceneEndTime", SIGNAL(dummySignal()), sceneProxy, sceneProxy->GetProperty("EndTime"));
  links.addTraceablePropertyLink(
    this, "scenePlayMode", SIGNAL(dummySignal()), sceneProxy, sceneProxy->GetProperty("PlayMode"));
  links.addTraceablePropertyLink(this, "sceneNumberOfFrames", SIGNAL(dummySignal()), sceneProxy,
    sceneProxy->GetProperty("NumberOfFrames"));
  links.addTraceablePropertyLink(this, "sceneCurrentTime", SIGNAL(sceneCurrentTimeChanged()),
    sceneProxy, sceneProxy->GetProperty("AnimationTime"));
  // FIXME: update ticks based on play mode.

  vtkSMProxy* timeKeeper = controller->FindTimeKeeper(session);
  assert(timeKeeper);
  links.addPropertyLink(this, "sceneTimeSteps", SIGNAL(dummySignal()), timeKeeper,
    timeKeeper->GetProperty("TimestepValues"));
  links.addPropertyLink<PropertyLinksConnection>(
    this, "timeSources", SIGNAL(dummySignal()), timeKeeper, timeKeeper->GetProperty("TimeSources"));
  links.addPropertyLink<PropertyLinksConnection>(this, "suppressedTimeSources",
    SIGNAL(suppressedTimeSourcesChanged()), timeKeeper,
    timeKeeper->GetProperty("SuppressedTimeSources"));
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::setSceneStartTime(double val)
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  animationModel->setStartTime(val);
}

//-----------------------------------------------------------------------------
double pqTimeInspectorWidget::sceneStartTime() const
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  return animationModel->startTime();
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::setSceneEndTime(double val)
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  animationModel->setEndTime(val);
}

//-----------------------------------------------------------------------------
double pqTimeInspectorWidget::sceneEndTime() const
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  return animationModel->endTime();
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::setScenePlayMode(const QString& val)
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  if (val == "Sequence")
  {
    animationModel->setMode(pqAnimationModel::Sequence);
  }
  else if (val == "Real Time")
  {
    animationModel->setMode(pqAnimationModel::Real);
  }
  else if (val == "Snap To TimeSteps")
  {
    animationModel->setMode(pqAnimationModel::Custom);
  }
  else
  {
    qCritical() << "pqTimeInspectorWidget detected unknown play mode: " << val;
  }
}

//-----------------------------------------------------------------------------
QString pqTimeInspectorWidget::scenePlayMode() const
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  switch (animationModel->mode())
  {
    case pqAnimationModel::Real:
      return "Real Time";

    case pqAnimationModel::Sequence:
      return "Sequence";

    case pqAnimationModel::Custom:
    default:
      return "Snap To TimeSteps";
  }
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::setSceneTimeSteps(const QList<QVariant>& val)
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  std::vector<double> timeSteps;
  foreach (const QVariant& v, val)
  {
    if (v.canConvert<double>())
    {
      timeSteps.push_back(v.toDouble());
    }
  }
  if (timeSteps.size() > 0)
  {
    animationModel->setTickMarks(static_cast<int>(timeSteps.size()), &timeSteps[0]);
  }
  else
  {
    animationModel->setTickMarks(0, NULL);
  }

  this->generalSettingsChanged();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqTimeInspectorWidget::sceneTimeSteps() const
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  QList<QVariant> reply;
  foreach (double val, animationModel->customTicks())
  {
    reply.push_back(val);
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::setSceneNumberOfFrames(int val)
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  animationModel->setTicks(val);
}

//-----------------------------------------------------------------------------
int pqTimeInspectorWidget::sceneNumberOfFrames() const
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  return animationModel->ticks();
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::setTimeSources(const QList<QVariant>& value)
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);

  QSet<vtkSMProxy*> proxies;
  foreach (const QVariant& val, value)
  {
    vtkSMProxy* proxy = reinterpret_cast<vtkSMProxy*>(val.value<void*>());
    proxies.insert(proxy);
  }

  this->Internals->TimeSources = value;
  for (int max = animationModel->count(), cc = max - 1; cc >= 0; cc--)
  {
    TimeTrack* track = dynamic_cast<TimeTrack*>(animationModel->track(cc));
    assert(track);
    if (proxies.remove(track->source()) == false)
    {
      animationModel->removeTrack(track);
    }
  }
  foreach (vtkSMProxy* proxy, proxies)
  {
    if (proxy && (proxy->GetProperty("TimestepValues") || proxy->GetProperty("TimeRange")))
    {
      TimeTrack* track = new TimeTrack(proxy, animationModel);
      animationModel->addTrack(track);
      pqProxy* pqproxy = getPQProxy(proxy);
      assert(pqproxy);
      track->setProperty(pqproxy->getSMName());
      track->setEnabled(true);
    }
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqTimeInspectorWidget::timeSources() const
{
  return this->Internals->TimeSources;
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::setSuppressedTimeSources(const QList<QVariant>& value)
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);

  this->Internals->SuppressedTimeSources = value;
  QSet<vtkSMProxy*> proxies;
  foreach (const QVariant& val, value)
  {
    vtkSMProxy* proxy = reinterpret_cast<vtkSMProxy*>(val.value<void*>());
    proxies.insert(proxy);
  }

  for (int max = animationModel->count(), cc = max - 1; cc >= 0; cc--)
  {
    TimeTrack* track = dynamic_cast<TimeTrack*>(animationModel->track(cc));
    assert(track);
    track->setEnabled(!proxies.contains(track->source()));
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqTimeInspectorWidget::suppressedTimeSources() const
{
  return this->Internals->SuppressedTimeSources;
}

//-----------------------------------------------------------------------------
double pqTimeInspectorWidget::sceneCurrentTime() const
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  return animationModel->currentTime();
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::setSceneCurrentTime(double time)
{
  pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
  assert(animationModel);
  animationModel->setCurrentTime(time);
  emit this->sceneCurrentTimeChanged();
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::toggleTrackSuppression(pqAnimationTrack* track)
{
  TimeTrack* ttrack = dynamic_cast<TimeTrack*>(track);
  assert(ttrack);

  bool suppress = ttrack->isEnabled();

  if (suppress)
  {
    this->Internals->SuppressedTimeSources.push_back(QVariant::fromValue<void*>(ttrack->source()));
    ttrack->setEnabled(false);
  }
  else
  {
    ttrack->setEnabled(true);
    QList<QVariant> newValue;
    foreach (const QVariant& val, this->Internals->SuppressedTimeSources)
    {
      vtkSMProxy* proxy = reinterpret_cast<vtkSMProxy*>(val.value<void*>());
      if (proxy != ttrack->source())
      {
        newValue.push_back(val);
      }
    }
    this->Internals->SuppressedTimeSources = newValue;
  }
  emit this->suppressedTimeSourcesChanged();
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::handleProxyNameChanged(pqServerManagerModelItem* item)
{
  pqProxy* pqproxy = qobject_cast<pqProxy*>(item);
  if (pqproxy)
  {
    pqAnimationModel* animationModel = this->Internals->Ui.AnimationWidget->animationModel();
    assert(animationModel);
    for (int max = animationModel->count(), cc = max - 1; cc >= 0; cc--)
    {
      TimeTrack* track = dynamic_cast<TimeTrack*>(animationModel->track(cc));
      assert(track);
      if (track->source() == pqproxy->getProxy())
      {
        track->setProperty(pqproxy->getSMName());
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqTimeInspectorWidget::generalSettingsChanged()
{
  int timePrecision = vtkPVGeneralSettings::GetInstance()->GetAnimationTimePrecision();
  this->Internals->Ui.AnimationTimeWidget->setTimePrecision(timePrecision);
  this->Internals->Ui.AnimationWidget->animationModel()->setTimePrecision(timePrecision);
  char timeNotation = vtkPVGeneralSettings::GetInstance()->GetAnimationTimeNotation();
  this->Internals->Ui.AnimationTimeWidget->setTimeNotation(timeNotation);
  this->Internals->Ui.AnimationWidget->animationModel()->setTimeNotation(timeNotation);
}
