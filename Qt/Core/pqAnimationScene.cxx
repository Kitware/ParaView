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

#include "pqAnimationCue.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqTimeKeeper.h"
#include "vtkAnimationCue.h"
#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPoints.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUtilities.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <QPointer>
#include <QSet>
#include <QSize>
#include <QtDebug>

template <class T>
static uint qHash(QPointer<T> p)
{
  return qHash(static_cast<T*>(p));
}

class pqAnimationScene::pqInternals
{
public:
  QSet<QPointer<pqAnimationCue> > Cues;
  QPointer<pqAnimationCue> GlobalTimeCue;
  pqInternals() {}
};

//-----------------------------------------------------------------------------
pqAnimationScene::pqAnimationScene(const QString& group, const QString& name, vtkSMProxy* proxy,
  pqServer* server, QObject* _parent /*=NULL*/)
  : pqProxy(group, name, proxy, server, _parent)
{
  vtkObject* animationScene = vtkObject::SafeDownCast(proxy->GetClientSideObject());

  this->Internals = new pqAnimationScene::pqInternals();
  vtkEventQtSlotConnect* connector = this->getConnector();

  connector->Connect(
    proxy->GetProperty("Cues"), vtkCommand::ModifiedEvent, this, SLOT(onCuesChanged()));
  connector->Connect(animationScene, vtkCommand::AnimationCueTickEvent, this,
    SLOT(onTick(vtkObject*, unsigned long, void*, void*)));
  connector->Connect(animationScene, vtkCommand::StartEvent, this, SIGNAL(beginPlay()));
  connector->Connect(animationScene, vtkCommand::EndEvent, this, SIGNAL(endPlay()));

  connector->Connect(
    proxy->GetProperty("PlayMode"), vtkCommand::ModifiedEvent, this, SIGNAL(playModeChanged()));
  connector->Connect(
    proxy->GetProperty("Loop"), vtkCommand::ModifiedEvent, this, SIGNAL(loopChanged()));
  connector->Connect(proxy->GetProperty("NumberOfFrames"), vtkCommand::ModifiedEvent, this,
    SIGNAL(frameCountChanged()));

  connector->Connect(proxy->GetProperty("StartTime"), vtkCommand::ModifiedEvent, this,
    SIGNAL(clockTimeRangesChanged()));
  connector->Connect(proxy->GetProperty("EndTime"), vtkCommand::ModifiedEvent, this,
    SIGNAL(clockTimeRangesChanged()));
  connector->Connect(proxy->GetProperty("AnimationTime"), vtkCommand::ModifiedEvent, this,
    SLOT(onAnimationTimePropertyChanged()));
  this->onCuesChanged();
  this->onAnimationTimePropertyChanged();

  pqTimeKeeper* timekeeper = this->getServer()->getTimeKeeper();
  connector->Connect(timekeeper->getProxy()->GetProperty("TimeLabel"), vtkCommand::ModifiedEvent,
    this, SIGNAL(timeLabelChanged()));
  this->connect(timekeeper, SIGNAL(timeStepsChanged()), SIGNAL(timeStepsChanged()));
}

//-----------------------------------------------------------------------------
pqAnimationScene::~pqAnimationScene()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
QList<double> pqAnimationScene::getTimeSteps() const
{
  return this->getServer()->getTimeKeeper()->getTimeSteps();
  ;
}

//-----------------------------------------------------------------------------
QPair<double, double> pqAnimationScene::getClockTimeRange() const
{
  double start =
    pqSMAdaptor::getElementProperty(this->getProxy()->GetProperty("StartTime")).toDouble();
  double end = pqSMAdaptor::getElementProperty(this->getProxy()->GetProperty("EndTime")).toDouble();
  return QPair<double, double>(start, end);
}

//-----------------------------------------------------------------------------
void pqAnimationScene::onCuesChanged()
{
  pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("Cues"));
  QSet<QPointer<pqAnimationCue> > currentCues;

  for (unsigned int cc = 0; cc < pp->GetNumberOfProxies(); cc++)
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
  foreach (pqAnimationCue* cue, this->Internals->Cues)
  {
    ret.insert(cue);
  }
  return ret;
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationScene::getCue(
  vtkSMProxy* proxy, const char* propertyname, int index) const
{
  foreach (pqAnimationCue* pqCue, this->Internals->Cues)
  {
    vtkSMProxy* cue = pqCue->getProxy();
    vtkSMProxy* animatedProxy = pqSMAdaptor::getProxyProperty(cue->GetProperty("AnimatedProxy"));
    auto propertyNameProperty = cue->GetProperty("AnimatedPropertyName");
    if (!propertyNameProperty)
    {
      continue;
    }

    QString aname =
      pqSMAdaptor::getElementProperty(cue->GetProperty("AnimatedPropertyName")).toString();
    int aindex = pqSMAdaptor::getElementProperty(cue->GetProperty("AnimatedElement")).toInt();

    if ((animatedProxy == proxy) && (aname == propertyname) && (aindex == index))
    {
      return pqCue;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationScene::createCue(vtkSMProxy* proxy, const char* propertyname, int index)
{
  return this->createCueInternal("KeyFrameAnimationCue", proxy, propertyname, index);
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationScene::createCue(
  vtkSMProxy* proxy, const char* propertyname, int index, const QString& cuetype)
{
  return this->createCueInternal(cuetype, proxy, propertyname, index);
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationScene::createCue(const QString& cuetype)
{
  return this->createCueInternal(cuetype, NULL, NULL, -1);
}

//-----------------------------------------------------------------------------
static void pqAnimationSceneResetCameraKeyFrameToCurrent(vtkSMProxy* ren, vtkSMProxy* dest)
{
  ren->UpdatePropertyInformation();
  const char* names[] = { "Position", "FocalPoint", "ViewUp", "ViewAngle", "ParallelScale", 0 };
  const char* snames[] = { "CameraPositionInfo", "CameraFocalPointInfo", "CameraViewUpInfo",
    "CameraViewAngle", "CameraParallelScale", 0 };
  for (int cc = 0; names[cc] && snames[cc]; cc++)
  {
    QList<QVariant> p = pqSMAdaptor::getMultipleElementProperty(ren->GetProperty(snames[cc]));
    pqSMAdaptor::setMultipleElementProperty(dest->GetProperty(names[cc]), p);
  }
}

//-----------------------------------------------------------------------------
/// Initialize the cue with some default key frames.
void pqAnimationScene::initializeCue(
  vtkSMProxy* proxy, const char* propertyname, int index, pqAnimationCue* cue)
{
  // (This code has simply been moved from pqAnimationViewWidget to keep it
  // centralized in the "controller". Ideally this may even move to the server
  // manager layer -- but I'll do that in the next iteration as the location
  // for the controller materializes.)

  QString cueType = cue->getProxy()->GetXMLName();
  if (cueType == "KeyFrameAnimationCue")
  {
    // Create a pair of default keyframes for this new cue.
    vtkSMProxy* kf1 = cue->insertKeyFrame(0);
    vtkSMProxy* kf2 = cue->insertKeyFrame(1);

    // Initialize default values for the newly keyframes based on the domain
    // for the property to be animated.
    vtkSMProperty* prop = proxy->GetProperty(propertyname);
    QList<QVariant> mins;
    QList<QVariant> maxs;
    if (index == -1 && prop)
    {
      QList<QList<QVariant> > domains = pqSMAdaptor::getMultipleElementPropertyDomain(prop);
      QList<QVariant> currents = pqSMAdaptor::getMultipleElementProperty(prop);
      for (int i = 0; i < currents.size(); i++)
      {
        if (domains.size() > i && domains[i].size())
        {
          mins.append(domains[i][0].isValid() ? domains[i][0] : currents[i]);
          maxs.append(domains[i][1].isValid() ? domains[i][1] : currents[i]);
        }
        else
        {
          mins.append(currents[i]);
          maxs.append(currents[i]);
        }
      }
    }
    else
    {
      QList<QVariant> domain = pqSMAdaptor::getMultipleElementPropertyDomain(prop, index);
      QVariant current = pqSMAdaptor::getMultipleElementProperty(prop, index);
      if (domain.size() && domain[0].isValid())
      {
        mins.append(domain[0]);
      }
      else
      {
        mins.append(current);
      }
      if (domain.size() && domain[1].isValid())
      {
        maxs.append(domain[1]);
      }
      else
      {
        maxs.append(current);
      }
    }

    pqSMAdaptor::setMultipleElementProperty(kf1->GetProperty("KeyValues"), mins);
    pqSMAdaptor::setMultipleElementProperty(kf2->GetProperty("KeyValues"), maxs);
    kf1->UpdateVTKObjects();
    kf2->UpdateVTKObjects();
  }
  else if (cueType == "CameraAnimationCue")
  {
    cue->setKeyFrameType("CameraKeyFrame");

    // Setup default animation to revolve around the selected objects (if any)
    // in a plane normal to the current view-up vector.
    pqSMAdaptor::setElementProperty(
      cue->getProxy()->GetProperty("Mode"), 1); // PATH-based animation.
    vtkSMProxy* kf0 = cue->insertKeyFrame(0);
    vtkSMProxy* kf1 = cue->insertKeyFrame(1);
    pqAnimationSceneResetCameraKeyFrameToCurrent(proxy, kf0);
    pqAnimationSceneResetCameraKeyFrameToCurrent(proxy, kf1);
    kf0->UpdateVTKObjects();
    kf1->UpdateVTKObjects();

    double bounds[6] = { -1, 1, -1, 1, -1, 1 };
    cue->getServer()->activeSourcesSelectionModel()->GetSelectionDataBounds(bounds);

    vtkBoundingBox bbox(bounds);
    double center[3];
    bbox.GetCenter(center);
    vtkPoints* pts = vtkSMUtilities::CreateOrbit(center,
      &vtkSMPropertyHelper(kf0, "ViewUp").GetDoubleArray()[0], 5 * bbox.GetMaxLength() / 2.0, 10);
    vtkSMPropertyHelper(kf0, "PositionPathPoints")
      .Set(reinterpret_cast<double*>(pts->GetVoidPointer(0)), pts->GetNumberOfPoints() * 3);
    vtkSMPropertyHelper(kf0, "ClosedPositionPath").Set(1);
    vtkSMPropertyHelper(kf0, "FocalPathPoints").Set(center, 3);
    kf0->UpdateVTKObjects();
    pts->Delete();
  }
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationScene::createCueInternal(
  const QString& cuetype, vtkSMProxy* proxy, const char* propertyname, int index)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  pqObjectBuilder* builder = core->getObjectBuilder();
  vtkSMProxy* cueProxy =
    builder->createProxy("animation", cuetype.toLocal8Bit().data(), this->getServer(), "animation");
  pqAnimationCue* cue = smmodel->findItem<pqAnimationCue*>(cueProxy);
  if (!cue)
  {
    qDebug() << "Failed to create AnimationCue.";
    return 0;
  }

  if (proxy)
  {
    pqSMAdaptor::setProxyProperty(cueProxy->GetProperty("AnimatedProxy"), proxy);
    pqSMAdaptor::setElementProperty(cueProxy->GetProperty("AnimatedPropertyName"), propertyname);
    pqSMAdaptor::setElementProperty(cueProxy->GetProperty("AnimatedElement"), index);
    cueProxy->UpdateVTKObjects();
  }

  vtkSMProxy* sceneProxy = this->getProxy();
  pqSMAdaptor::addProxyProperty(sceneProxy->GetProperty("Cues"), cueProxy);
  sceneProxy->UpdateVTKObjects();

  if (proxy)
  {
    this->initializeCue(proxy, propertyname, index, cue);
  }

  // We don't directly add this cue to the internal Cues, it will get added
  // as a side effect of the change in the "Cues" property.
  return cue;
}

//-----------------------------------------------------------------------------
void pqAnimationScene::removeCue(pqAnimationCue* cue)
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("Cues"));

  pp->RemoveProxy(cue->getProxy());
  this->getProxy()->UpdateVTKObjects();

  builder->destroy(cue);
}

//-----------------------------------------------------------------------------
void pqAnimationScene::removeCues(vtkSMProxy* animated_proxy)
{
  pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("Cues"));

  QList<QPointer<pqAnimationCue> > toRemove;
  for (unsigned int cc = 0; cc < pp->GetNumberOfProxies(); cc++)
  {
    vtkSMProxy* cueProxy = pp->GetProxy(cc);
    if (pqSMAdaptor::getProxyProperty(cueProxy->GetProperty("AnimatedProxy")) == animated_proxy)
    {
      pqAnimationCue* pqCue = model->findItem<pqAnimationCue*>(cueProxy);
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

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

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
  pqSMAdaptor::setElementProperty(this->getProxy()->GetProperty("AnimationTime"), time);
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
  return pqSMAdaptor::getElementProperty(this->getProxy()->GetProperty("AnimationTime")).toDouble();
}

//-----------------------------------------------------------------------------
void pqAnimationScene::onTick(vtkObject*, unsigned long, void*, void* info)
{
  vtkAnimationCue::AnimationCueInfo* cueInfo =
    reinterpret_cast<vtkAnimationCue::AnimationCueInfo*>(info);
  if (!cueInfo)
  {
    return;
  }
  int progress = static_cast<int>(
    (cueInfo->AnimationTime - cueInfo->StartTime) * 100 / (cueInfo->EndTime - cueInfo->StartTime));

  this->setAnimationTime(cueInfo->AnimationTime);
  emit this->tick(progress);
}
