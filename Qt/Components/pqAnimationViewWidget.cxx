/*=========================================================================

   Program: ParaView
   Module:    pqAnimationViewWidget.cxx

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

=========================================================================*/

#include "pqAnimationViewWidget.h"

#include <QVBoxLayout>
#include <QPointer>
#include <QSignalMapper>

#include "pqAnimationWidget.h"
#include "pqAnimationModel.h"
#include "pqAnimationTrack.h"
#include "pqAnimationKeyFrame.h"

#include "vtkSMProxy.h"

#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqAnimationCue.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqAnimationViewWidget::pqInternals
{
public:
  QPointer<pqAnimationManager> Manager;
  QPointer<pqAnimationScene> ActiveScene;
  pqAnimationWidget* AnimationWidget;
  QSignalMapper KeyFramesChanged;

  pqAnimationTrack* findTrack(pqAnimationCue* cue)
    {
    pqAnimationModel* animModel =
      this->AnimationWidget->animationModel();
    QString name = this->cueName(cue);
    
    for(int i=0; i<animModel->count(); i++)
      {
      pqAnimationTrack* t = animModel->track(i);
      if(t->property() == name)
        {
        return t;
        }
      }
    return NULL;
    }
  QString cueName(pqAnimationCue* cue)
    {
    QString name;
    pqServerManagerModel* model = 
      pqApplicationCore::instance()-> getServerManagerModel();
    pqProxy* pxy = model->findItem<pqProxy*>(cue->getAnimatedProxy());
    vtkSMProperty* pty = cue->getAnimatedProperty();
    if(pxy && pty)
      {
      QString n = pxy->getSMName();
      QString p = pxy->getProxy()->GetPropertyName(pty);
      if(pqSMAdaptor::getPropertyType(pty) == pqSMAdaptor::MULTIPLE_ELEMENTS)
        {
        p = QString("%1 (%2)").arg(p).arg(cue->getAnimatedPropertyIndex());
        }
      name = QString("%1 - %2").arg(n).arg(p);
      }
    return name;
    }
};

//-----------------------------------------------------------------------------
pqAnimationViewWidget::pqAnimationViewWidget(QWidget* _parent) : QWidget(_parent)
{
  this->Internal = new pqAnimationViewWidget::pqInternals();
  QVBoxLayout* vboxlayout = new QVBoxLayout(this);

  this->Internal->AnimationWidget = new pqAnimationWidget(this);
  vboxlayout->addWidget(this->Internal->AnimationWidget);

  QObject::connect(&this->Internal->KeyFramesChanged, SIGNAL(mapped(QObject*)),
                   this, SLOT(keyFramesChanged(QObject*)));
}

//-----------------------------------------------------------------------------
pqAnimationViewWidget::~pqAnimationViewWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::setManager(pqAnimationManager* mgr)
{
  if (this->Internal->Manager == mgr)
    {
    return;
    }

  if (this->Internal->Manager)
    {
    QObject::disconnect(this->Internal->Manager, 0, this, 0);
    }

  this->Internal->Manager = mgr;

  if (this->Internal->Manager)
    {
    QObject::connect(this->Internal->Manager,
      SIGNAL(activeSceneChanged(pqAnimationScene*)),
      this, SLOT(onActiveSceneChanged(pqAnimationScene*)));
    }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::onActiveSceneChanged(pqAnimationScene* scene)
{
  if(this->Internal->ActiveScene)
    {
    QObject::disconnect(this->Internal->ActiveScene, 0, this, 0);
    }
  this->Internal->ActiveScene = scene;
  if(this->Internal->ActiveScene)
    {
    pqAnimationModel* animModel =
      this->Internal->AnimationWidget->animationModel();
    QPair<double, double> timeRange = scene->getClockTimeRange();
    animModel->setStartTime(timeRange.first);
    animModel->setEndTime(timeRange.second);

    QObject::connect(scene, SIGNAL(cuesChanged()), 
      this, SLOT(onSceneCuesChanged()));
    }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::onSceneCuesChanged()
{
  QSet<pqAnimationCue*> cues = this->Internal->ActiveScene->getCues();
  pqAnimationModel* animModel =
    this->Internal->AnimationWidget->animationModel();

  // get all existing tracks
  QList<QString> oldTracks;
  for(int i=0; i<animModel->count(); i++)
    {
    pqAnimationTrack* t = animModel->track(i);
    oldTracks.append(t->property().toString());
    }
  
  // add new tracks
  foreach(pqAnimationCue* cue, cues)
    {
    QString completeName = this->Internal->cueName(cue);

    if(!oldTracks.contains(completeName))
      {
      pqAnimationTrack* t = animModel->addTrack();
      t->setProperty(completeName);
      this->Internal->KeyFramesChanged.setMapping(cue, cue);
      QObject::connect(cue, SIGNAL(keyframesModified()),
        &this->Internal->KeyFramesChanged,
        SLOT(map()));
      }
    else
      {
      oldTracks.removeAll(completeName);
      }
    }

  // remove dead tracks
  foreach(QString s, oldTracks)
    {
    for(int i=0; i<animModel->count(); i++)
      {
      pqAnimationTrack* t = animModel->track(i);
      if(t->property() == s)
        {
        animModel->removeTrack(t);
        break;
        }
      }
    }

}
  
void pqAnimationViewWidget::keyFramesChanged(QObject* cueObject)
{
  pqAnimationCue* cue = qobject_cast<pqAnimationCue*>(cueObject);
  pqAnimationTrack* track = this->Internal->findTrack(cue);

  QList<vtkSMProxy*> keyFrames = cue->getKeyFrames();

  // clean out old ones
  while(track->count())
    {
    track->removeKeyFrame(track->keyFrame(0));
    }

  if(keyFrames.count())
    {
    QVariant startTime =
      pqSMAdaptor::getElementProperty(keyFrames.first()->GetProperty("KeyTime"));
    QVariant endTime =
      pqSMAdaptor::getElementProperty(keyFrames.last()->GetProperty("KeyTime"));
    track->setStartTime(startTime.toDouble());
    track->setEndTime(endTime.toDouble());
    }

  for(int j=0; j<keyFrames.count()-1; j++)
    {
    QVariant startTime =
      pqSMAdaptor::getElementProperty(keyFrames[j]->GetProperty("KeyTime"));
    QVariant endTime =
      pqSMAdaptor::getElementProperty(keyFrames[j+1]->GetProperty("KeyTime"));
    QVariant startValue =
      pqSMAdaptor::getElementProperty(keyFrames[j]->GetProperty("KeyValues"));
    QVariant endValue =
      pqSMAdaptor::getElementProperty(keyFrames[j+1]->GetProperty("KeyValues"));
    pqAnimationKeyFrame* newFrame = track->addKeyFrame();
    newFrame->setStartTime(startTime.toDouble());
    newFrame->setEndTime(endTime.toDouble());
    newFrame->setStartValue(startValue);
    newFrame->setEndValue(endValue);
    }
}


