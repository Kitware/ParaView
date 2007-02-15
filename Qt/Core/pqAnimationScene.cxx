/*=========================================================================

   Program: ParaView
   Module:    pqAnimationScene.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqAnimationScene.h"

#include "vtkAnimationCue.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkSMAbstractViewModuleProxy.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyProperty.h"

#include <QPointer>
#include <QSet>
#include <QtDebug>
#include <QSize>

#include "pqAnimationCue.h"
#include "pqApplicationCore.h"
#include "pqPipelineBuilder.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqTimeKeeper.h"

template<class T>
static uint qHash(QPointer<T> p)
{
  return qHash(static_cast<T*>(p));
}


class pqAnimationScene::pqInternals
{
public:
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSmartPointer<vtkSMPropertyLink> TimestepValuesLink;
  vtkSmartPointer<vtkSMPropertyLink> TimeLink;
  QSet<QPointer<pqAnimationCue> > Cues;
  QPointer<pqAnimationCue> GlobalTimeCue;
  pqInternals()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->TimestepValuesLink = vtkSmartPointer<vtkSMPropertyLink>::New();
    this->TimeLink = vtkSmartPointer<vtkSMPropertyLink>::New();
    }
};

//-----------------------------------------------------------------------------
pqAnimationScene::pqAnimationScene(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* _parent/*=NULL*/)
: pqProxy(group, name, proxy, server, _parent)
{
  this->Internals = new pqAnimationScene::pqInternals();
  this->Internals->VTKConnect->Connect(proxy->GetProperty("Cues"),
    vtkCommand::ModifiedEvent, this, SLOT(onCuesChanged()));
  this->Internals->VTKConnect->Connect(proxy,
    vtkCommand::AnimationCueTickEvent, 
    this, SLOT(onTick(vtkObject*, unsigned long, void*, void*)));
  this->Internals->VTKConnect->Connect(proxy, vtkCommand::StartEvent,
    this, SIGNAL(beginPlay()));
  this->Internals->VTKConnect->Connect(proxy, vtkCommand::EndEvent,
    this, SIGNAL(endPlay()));

  this->Internals->VTKConnect->Connect(
    proxy->GetProperty("PlayMode"), vtkCommand::ModifiedEvent,
    this, SIGNAL(playModeChanged()));
  this->Internals->VTKConnect->Connect(
    proxy->GetProperty("Loop"), vtkCommand::ModifiedEvent,
    this, SIGNAL(loopChanged()));

  this->Internals->VTKConnect->Connect(
    proxy->GetProperty("ClockTimeRange"), vtkCommand::ModifiedEvent,
    this, SIGNAL(clockTimeRangesChanged()));
  this->onCuesChanged();


  // Initialize the time keeper.
  this->setupTimeTrack();
}

//-----------------------------------------------------------------------------
pqAnimationScene::~pqAnimationScene()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
vtkSMAnimationSceneProxy* pqAnimationScene::getAnimationSceneProxy() const
{
  return vtkSMAnimationSceneProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
void pqAnimationScene::setupTimeTrack()
{
  pqTimeKeeper* timekeeper = this->getServer()->getTimeKeeper();

  QObject::connect(timekeeper, SIGNAL(timeStepsChanged()),
    this, SLOT(updateTimeRanges()));

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("TimeKeeper"));
  if (pp && pp->GetNumberOfProxies() == 0)
    {
    pp->AddProxy(timekeeper->getProxy());
    this->getProxy()->UpdateVTKObjects();
    }

  // Link timekeeper properties.
  this->Internals->TimestepValuesLink->AddLinkedProperty(
    timekeeper->getProxy(), "TimestepValues", vtkSMLink::INPUT);
  this->Internals->TimestepValuesLink->AddLinkedProperty(
    this->getProxy(), "TimeSteps", vtkSMLink::OUTPUT);
  timekeeper->getProxy()->GetProperty("TimestepValues")->Modified();

  this->Internals->TimeLink->AddLinkedProperty(
    timekeeper->getProxy(), "Time", vtkSMLink::INPUT);
  this->Internals->TimeLink->AddLinkedProperty(
    this->getProxy(), "ClockTime", vtkSMLink::OUTPUT);
  timekeeper->getProxy()->GetProperty("Time")->Modified();

  this->updateTimeRanges();
}

//-----------------------------------------------------------------------------
void pqAnimationScene::updateTimeRanges()
{
  pqTimeKeeper* timekeeper = this->getServer()->getTimeKeeper();
  if (timekeeper->getNumberOfTimeStepValues() == 0 || 
    pqApplicationCore::instance()->isLoadingState())
    {
    // If timekeeper has no timesteps at all
    // or if we are currently loading state then we don't want to change
    // the currently set start/end times.
    return;
    }

  QPair<double, double> range = timekeeper->getTimeRange();
  vtkSMProxy* sceneProxy = this->getProxy();

  QList<QVariant> locks = pqSMAdaptor::getMultipleElementProperty(
    sceneProxy->GetProperty("ClockTimeRangeLocks"));
  if (!locks[0].toBool())
    {
    pqSMAdaptor::setMultipleElementProperty(
      sceneProxy->GetProperty("ClockTimeRange"), 0, range.first);
    }
  if (!locks[1].toBool())
    {
    pqSMAdaptor::setMultipleElementProperty(
      sceneProxy->GetProperty("ClockTimeRange"), 1, range.second);
    }
  sceneProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
QPair<double, double> pqAnimationScene::getClockTimeRange() const
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("ClockTimeRange"));
  return QPair<double,double>(values[0].toDouble(), values[1].toDouble());
}

//-----------------------------------------------------------------------------
void pqAnimationScene::onCuesChanged()
{
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Cues"));
  QSet<QPointer<pqAnimationCue> > currentCues;

  for(unsigned int cc=0; cc < pp->GetNumberOfProxies(); cc++)
    {
    vtkSMProxy* proxy = pp->GetProxy(cc);
    pqAnimationCue* cue  = qobject_cast<pqAnimationCue*>(
      model->getPQProxy(proxy));
    if (cue && cue->getServer() == this->getServer())
      {
      currentCues.insert(cue);
      }
    }

  QSet<QPointer<pqAnimationCue> > added = currentCues - this->Internals->Cues;
  QSet<QPointer<pqAnimationCue> > removed = this->Internals->Cues - currentCues;

  foreach (pqAnimationCue* cue, removed)
    {
    emit this->preRemovedCue(cue);
    this->Internals->Cues.remove(cue);
    emit this->removedCue(cue);
    }

  foreach (pqAnimationCue* cue, added)
    {
    emit this->preAddedCue(cue);
    this->Internals->Cues.insert(cue);
    emit this->addedCue(cue);
    }

  if (removed.size() > 0 || added.size() > 0)
    {
    emit this->cuesChanged();
    }
}

//-----------------------------------------------------------------------------
bool pqAnimationScene::contains(pqAnimationCue* cue) const
{
  return this->Internals->Cues.contains(cue);
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationScene::getCue(vtkSMProxy* proxy, 
  const char* propertyname, int index) const
{
  foreach (pqAnimationCue* pqCue, this->Internals->Cues)
    {
    vtkSMProxy* cue = pqCue->getProxy();
    vtkSMProxy* animatedProxy = 
      pqSMAdaptor::getProxyProperty(cue->GetProperty("AnimatedProxy"));
    QString aname = pqSMAdaptor::getElementProperty(
      cue->GetProperty("AnimatedPropertyName")).toString();
    int aindex = pqSMAdaptor::getElementProperty(
      cue->GetProperty("AnimatedElement")).toInt();

    if ((animatedProxy == proxy) && (aname == propertyname) && (aindex == index))
      {
      return pqCue;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationScene::createCue(vtkSMProxy* proxy, 
  const char* propertyname, int index) 
{
  return this->createCueInternal("KeyFrameAnimationCueManipulator",
    proxy, propertyname, index);
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationScene::createCueInternal(const QString& mtype,
  vtkSMProxy* proxy, const char* propertyname, int index) 
{
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineBuilder* builder = pqApplicationCore::instance()->getPipelineBuilder();

  vtkSMProxy* cueProxy = builder->createProxy("animation", "AnimationCue", "animation",
    this->getServer());
  cueProxy->SetServers(vtkProcessModule::CLIENT);
  pqAnimationCue* cue = qobject_cast<pqAnimationCue*>(smmodel->getPQProxy(cueProxy));
  if (!cue)
    {
    qDebug() << "Failed to create AnimationCue.";
    return 0;
    }
  cue->setManipulatorType(mtype);
  cue->setDefaults();

  pqSMAdaptor::setProxyProperty(cueProxy->GetProperty("AnimatedProxy"), proxy);
  pqSMAdaptor::setElementProperty(cueProxy->GetProperty("AnimatedPropertyName"), 
    propertyname);
  pqSMAdaptor::setElementProperty(cueProxy->GetProperty("AnimatedElement"), index);
  cueProxy->UpdateVTKObjects();

  vtkSMProxy* sceneProxy = this->getProxy();
  pqSMAdaptor::addProxyProperty(sceneProxy->GetProperty("Cues"), cueProxy);
  sceneProxy->UpdateVTKObjects();

  // We don't directly add this cue to the internal Cues, it will get added
  // as a side effect of the change in the "Cues" property.

  return cue;
}

//-----------------------------------------------------------------------------
void pqAnimationScene::removeCues(vtkSMProxy* animated_proxy)
{
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Cues"));

  QList<QPointer<pqAnimationCue> > toRemove;
  for(unsigned int cc=0; cc < pp->GetNumberOfProxies(); cc++)
    {
    vtkSMProxy* cueProxy = pp->GetProxy(cc);
    if (pqSMAdaptor::getProxyProperty(
        cueProxy->GetProperty("AnimatedProxy")) == animated_proxy)
      {
      pqAnimationCue* pqCue  = qobject_cast<pqAnimationCue*>(
        model->getPQProxy(cueProxy));
      toRemove.push_back(pqCue);
      }
    }
  vtkSMProxy* sceneProxy = this->getProxy();
  pp->RemoveAllProxies();
  foreach (pqAnimationCue* cue, toRemove)
    {
    if (cue)
      {
      pp->RemoveProxy(cue->getProxy());
      }
    }
  sceneProxy->UpdateVTKObjects();

  pqPipelineBuilder* builder = pqApplicationCore::instance()->getPipelineBuilder();
  foreach (pqAnimationCue* cue, toRemove)
    {
    if (cue)
      {
      // When the Cue is removed, the manipulator proxy as well as the keyframes
      // get unregistered automatically, since we've registered them as internal
      // proxies to the pqAnimationCue. Ofcourse, if python client had registered
      // the manipulator/keyframes, they won't get unregistered by this.
      builder->remove(cue);
      }
    }
}

//-----------------------------------------------------------------------------
QSize pqAnimationScene::getViewSize() const
{
  QSize size;
  // Simply get the first view module and get it's GUISize.
  vtkSMAnimationSceneProxy* sceneProxy = this->getAnimationSceneProxy();
  if (sceneProxy->GetNumberOfViewModules() > 0)
    {
    vtkSMAbstractViewModuleProxy* view = sceneProxy->GetViewModule(0);
    size.setWidth(view->GetGUISize()[0]);
    size.setHeight(view->GetGUISize()[1]);

    }
  return size;
}

//-----------------------------------------------------------------------------
void pqAnimationScene::play()
{
  this->getAnimationSceneProxy()->Play();
}

//-----------------------------------------------------------------------------
void pqAnimationScene::pause()
{
  this->getAnimationSceneProxy()->Stop();
}

//-----------------------------------------------------------------------------
void pqAnimationScene::onTick(vtkObject*, unsigned long, void*, void* info)
{
  vtkAnimationCue::AnimationCueInfo *cueInfo = 
    reinterpret_cast<vtkAnimationCue::AnimationCueInfo*>(info);
  if (!cueInfo)
    {
    return;
    }
  int progress = static_cast<int>(
    (cueInfo->AnimationTime - cueInfo->StartTime)*100/
    (cueInfo->EndTime - cueInfo->StartTime));

  emit this->tick(progress);
}
