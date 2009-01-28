/*=========================================================================

   Program: ParaView
   Module:    pqAnimationScene.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
#include "pqAnimationScene.h"

#include "vtkAnimationCue.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkSMViewProxy.h"
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
#include "pqObjectBuilder.h"
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
  QSet<QPointer<pqAnimationCue> > Cues;
  QPointer<pqAnimationCue> GlobalTimeCue;
  pqInternals()
    {
    }
};

//-----------------------------------------------------------------------------
pqAnimationScene::pqAnimationScene(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* _parent/*=NULL*/)
: pqProxy(group, name, proxy, server, _parent)
{
  this->Internals = new pqAnimationScene::pqInternals();
  vtkEventQtSlotConnect* connector = this->getConnector();

  connector->Connect(proxy->GetProperty("Cues"),
    vtkCommand::ModifiedEvent, this, SLOT(onCuesChanged()));
  connector->Connect(proxy,
    vtkCommand::AnimationCueTickEvent, 
    this, SLOT(onTick(vtkObject*, unsigned long, void*, void*)));
  connector->Connect(proxy, vtkCommand::StartEvent,
    this, SIGNAL(beginPlay()));
  connector->Connect(proxy, vtkCommand::EndEvent,
    this, SIGNAL(endPlay()));

  connector->Connect(
    proxy->GetProperty("PlayMode"), vtkCommand::ModifiedEvent,
    this, SIGNAL(playModeChanged()));
  connector->Connect(
    proxy->GetProperty("Loop"), vtkCommand::ModifiedEvent,
    this, SIGNAL(loopChanged()));
  connector->Connect(
    proxy->GetProperty("NumberOfFrames"), vtkCommand::ModifiedEvent,
    this, SIGNAL(frameCountChanged()));

  connector->Connect(
    proxy->GetProperty("StartTimeInfo"), vtkCommand::ModifiedEvent,
    this, SIGNAL(clockTimeRangesChanged()));
  connector->Connect(
    proxy->GetProperty("EndTimeInfo"), vtkCommand::ModifiedEvent,
    this, SIGNAL(clockTimeRangesChanged()));
  connector->Connect(
    proxy->GetProperty("AnimationTime"), vtkCommand::ModifiedEvent,
    this, SLOT(onAnimationTimePropertyChanged()));
  this->onCuesChanged();
  this->onAnimationTimePropertyChanged();

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
void  pqAnimationScene::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();

  // Create an animation cue for the pipeline time.
  this->createCueInternal("TimeAnimationCue",
    this->getServer()->getTimeKeeper()->getProxy(),
    "Time", 0);
  this->setAnimationTime(0.0);
}

//-----------------------------------------------------------------------------
void pqAnimationScene::setupTimeTrack()
{
  pqTimeKeeper* timekeeper = this->getServer()->getTimeKeeper();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("TimeKeeper"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(timekeeper->getProxy());
    this->getProxy()->UpdateVTKObjects();
    }

  QObject::connect(timekeeper, SIGNAL(timeStepsChanged()),
    this, SLOT(updateTimeSteps()));
  QObject::connect(timekeeper, SIGNAL(timeRangeChanged()),
    this, SLOT(updateTimeSteps()));
  this->updateTimeSteps();
}

//-----------------------------------------------------------------------------
void pqAnimationScene::updateTimeSteps()
{
  pqTimeKeeper* timekeeper = this->getServer()->getTimeKeeper();
  if (pqApplicationCore::instance()->isLoadingState())
    {
    // If we are currently loading state then we don't want to change
    // the currently set start/end times.
    return;
    }

  vtkSMProxy* sceneProxy = this->getProxy();

  // Adjust the play mode based on whether or not we have time steps.
  vtkSMProperty *playModeProperty = sceneProxy->GetProperty("PlayMode");
  if (timekeeper->getNumberOfTimeStepValues() == 0)
    {
    if (pqSMAdaptor::getEnumerationProperty(playModeProperty)
      == "Snap To TimeSteps" )
      {
      pqSMAdaptor::setEnumerationProperty(playModeProperty, "Sequence");
      pqSMAdaptor::setElementProperty(
        sceneProxy->GetProperty("UseCustomEndTimes"), 1);
      }
    }
  else
    {
    pqSMAdaptor::setEnumerationProperty(playModeProperty, "Snap To TimeSteps");
    pqSMAdaptor::setElementProperty(
      sceneProxy->GetProperty("UseCustomEndTimes"), 0);
    }

  bool disable_automatic =
    pqSMAdaptor::getElementProperty(
      sceneProxy->GetProperty("DisableAutomaticTimeAdjustment")).toBool();

  if (!disable_automatic)
    {
    QPair<double, double> range = timekeeper->getTimeRange();
    pqSMAdaptor::setElementProperty(
      sceneProxy->GetProperty("StartTime"), range.first);
    pqSMAdaptor::setElementProperty(
      sceneProxy->GetProperty("EndTime"), range.second);
    }

  sceneProxy->UpdateVTKObjects();

  /// If the animation time is not in the scene time range, set it to the min
  /// value.
  double min = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("StartTimeInfo")).toDouble();
  double max = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("EndTimeInfo")).toDouble();
  double cur = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("AnimationTime")).toDouble();
  if (cur < min || cur > max)
    {
    this->setAnimationTime(min);
    }
  emit this->timeStepsChanged();
}

//-----------------------------------------------------------------------------
QList<double> pqAnimationScene::getTimeSteps() const
{
  return this->getServer()->getTimeKeeper()->getTimeSteps();;
}

//-----------------------------------------------------------------------------
QPair<double, double> pqAnimationScene::getClockTimeRange() const
{
  double start = pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("StartTimeInfo")).toDouble();
  double end = pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("EndTimeInfo")).toDouble();
  return QPair<double,double>(start, end);
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
    pqAnimationCue* cue = model->findItem<pqAnimationCue*>(proxy);
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
QSet<pqAnimationCue*> pqAnimationScene::getCues() const
{
  QSet<pqAnimationCue*> ret;
  foreach(pqAnimationCue* cue, this->Internals->Cues)
    {
    ret.insert(cue);
    }
  return ret;
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
  return this->createCueInternal("KeyFrameAnimationCue",
    proxy, propertyname, index);
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationScene::createCue(vtkSMProxy* proxy, 
  const char* propertyname, int index, const QString& cuetype) 
{
  return this->createCueInternal(cuetype,
    proxy, propertyname, index);
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationScene::createCueInternal(const QString& cuetype,
  vtkSMProxy* proxy, const char* propertyname, int index) 
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  pqObjectBuilder* builder = core->getObjectBuilder();
  vtkSMProxy* cueProxy = builder->createProxy(
    "animation", cuetype.toAscii().data(), this->getServer(), "animation");
  cueProxy->SetServers(vtkProcessModule::CLIENT);
  pqAnimationCue* cue = smmodel->findItem<pqAnimationCue*>(cueProxy);
  if (!cue)
    {
    qDebug() << "Failed to create AnimationCue.";
    return 0;
    }
  cue->setDefaultPropertyValues();

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
void pqAnimationScene::removeCue(pqAnimationCue* cue)
{
  pqObjectBuilder* builder = 
    pqApplicationCore::instance()->getObjectBuilder();
  
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Cues"));

  pp->RemoveProxy(cue->getProxy());
  this->getProxy()->UpdateVTKObjects();
    
  builder->destroy(cue);
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
      pqAnimationCue* pqCue  = model->findItem<pqAnimationCue*>(cueProxy);
      toRemove.push_back(pqCue);
      }
    }
  vtkSMProxy* sceneProxy = this->getProxy();
  foreach (pqAnimationCue* cue, toRemove)
    {
    if (cue)
      {
      pp->RemoveProxy(cue->getProxy());
      }
    }
  sceneProxy->UpdateVTKObjects();

  pqObjectBuilder* builder = 
    pqApplicationCore::instance()->getObjectBuilder();

  foreach (pqAnimationCue* cue, toRemove)
    {
    // When the Cue is removed, the manipulator proxy as well as the keyframes
    // get unregistered automatically, since we've registered them as internal
    // proxies to the pqAnimationCue. Ofcourse, if python client had registered
    // the manipulator/keyframes, they won't get unregistered by this.
    builder->destroy(cue);
    }
}

//-----------------------------------------------------------------------------
void pqAnimationScene::play()
{
  this->getProxy()->InvokeCommand("Play");
}

//-----------------------------------------------------------------------------
void pqAnimationScene::pause()
{
  this->getProxy()->InvokeCommand("Stop");
}

//-----------------------------------------------------------------------------
void pqAnimationScene::setAnimationTime(double time)
{
  // It's safe to call this method in a tick callback, since vtkSMProxy ensures
  // that SetAnimationTime() will be ignored within a tick callback.

  // update the "AnimationTime" property on the scene proxy.
  pqSMAdaptor::setElementProperty(this->getProxy()->GetProperty("AnimationTime"),
    time);
  this->getProxy()->UpdateProperty("AnimationTime");
}

//-----------------------------------------------------------------------------
void pqAnimationScene::onAnimationTimePropertyChanged()
{
  emit this->animationTime(this->getAnimationTime());
}

//-----------------------------------------------------------------------------
double pqAnimationScene::getAnimationTime() const
{
 return pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("AnimationTime")).toDouble();
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

  this->setAnimationTime(cueInfo->AnimationTime);
  emit this->tick(progress);
}
