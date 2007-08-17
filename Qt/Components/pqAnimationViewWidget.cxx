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
#include <QDialog>
#include <QDialogButtonBox>

#include "pqAnimationWidget.h"
#include "pqAnimationModel.h"
#include "pqAnimationTrack.h"
#include "pqAnimationKeyFrame.h"

#include "vtkSMProxy.h"
#include "vtkSMAnimationSceneProxy.h"

#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqAnimationScene.h"
#include "pqAnimationCue.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqKeyFrameEditor.h"

//-----------------------------------------------------------------------------
class pqAnimationViewWidget::pqInternal
{
public:
  pqInternal()
    {
    }
  ~pqInternal()
    {
    }

  QPointer<pqAnimationScene> Scene;
  pqAnimationWidget* AnimationWidget;
  QSignalMapper KeyFramesChanged;
  typedef QMap<QPointer<pqAnimationCue>, pqAnimationTrack*> TrackMapType;
  TrackMapType TrackMap;
  QPointer<QDialog> Editor;

  pqAnimationTrack* findTrack(pqAnimationCue* cue)
    {
    TrackMapType::iterator iter;
    iter = this->TrackMap.find(cue);
    if(iter != this->TrackMap.end())
      {
      return iter.value();
      }
    return NULL;
    }
  pqAnimationCue* findCue(pqAnimationTrack* track)
    {
    TrackMapType::iterator iter;
    for(iter = this->TrackMap.begin(); iter != this->TrackMap.end(); ++iter)
      {
      if(iter.value() == track)
        {
        return iter.key();
        }
      }
    return NULL;
    }
  QString cueName(pqAnimationCue* cue)
    {
    QString name;
    if(this->cameraCue(cue))
      {
      name = "Camera";
      }
    else
      {
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
      }
    return name;
    }
  // returns if this is a cue for animating a camera
  bool cameraCue(pqAnimationCue* cue)
    {
    if(QString("CameraAnimationCue") == cue->getProxy()->GetXMLName())
      {
      return true;
      }
    return false;
    }

  int numberOfTicks()
    {
    vtkSMProxy* pxy = this->Scene->getProxy();
    QString mode =
      pqSMAdaptor::getEnumerationProperty(pxy->GetProperty("PlayMode")).toString();

    int num = 0;
    
    if(mode == "Sequence")
      {
      num = 
        pqSMAdaptor::getElementProperty(
          pxy->GetProperty("NumberOfFrames")).toInt();
      }
    else if(mode == "Snap To TimeSteps")
      {
      num = 
        pqSMAdaptor::getMultipleElementProperty(
          pxy->GetProperty("TimeSteps")).size();
      }
    return num;
    }
};

//-----------------------------------------------------------------------------
pqAnimationViewWidget::pqAnimationViewWidget(QWidget* _parent) : QWidget(_parent)
{
  this->Internal = new pqAnimationViewWidget::pqInternal();
  QVBoxLayout* vboxlayout = new QVBoxLayout(this);

  this->Internal->AnimationWidget = new pqAnimationWidget(this);
  vboxlayout->addWidget(this->Internal->AnimationWidget);

  QObject::connect(&this->Internal->KeyFramesChanged, SIGNAL(mapped(QObject*)),
                   this, SLOT(keyFramesChanged(QObject*)));

  QObject::connect(this->Internal->AnimationWidget,
                   SIGNAL(trackSelected(pqAnimationTrack*)),
                   this, SLOT(trackSelected(pqAnimationTrack*)));
}

//-----------------------------------------------------------------------------
pqAnimationViewWidget::~pqAnimationViewWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::setScene(pqAnimationScene* scene)
{
  if(this->Internal->Scene)
    {
    QObject::disconnect(this->Internal->Scene, 0, this, 0);
    }
  this->Internal->Scene = scene;
  if(this->Internal->Scene)
    {
    QObject::connect(scene, SIGNAL(cuesChanged()), 
      this, SLOT(onSceneCuesChanged()));
    QObject::connect(scene, SIGNAL(clockTimeRangesChanged()),
            this, SLOT(updateSceneTimeRange()));
    QObject::connect(scene, SIGNAL(timeStepsChanged()),
            this, SLOT(updateTicks()));
    QObject::connect(scene, SIGNAL(frameCountChanged()),
            this, SLOT(updateTicks()));
    QObject::connect(scene, SIGNAL(animationTime(double)),
            this, SLOT(updateSceneTime()));
    QObject::connect(scene, SIGNAL(playModeChanged()), 
      this, SLOT(updatePlayMode()));
    QObject::connect(scene, SIGNAL(playModeChanged()), 
      this, SLOT(updateTicks()));
    QObject::connect(scene, SIGNAL(playModeChanged()), 
      this, SLOT(updateSceneTime()));
    this->updateSceneTimeRange();
    this->updateSceneTime();
    this->updatePlayMode();
    this->updateTicks();
    }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::onSceneCuesChanged()
{
  QSet<pqAnimationCue*> cues = this->Internal->Scene->getCues();
  pqAnimationModel* animModel =
    this->Internal->AnimationWidget->animationModel();
    
  pqInternal::TrackMapType oldCues = this->Internal->TrackMap;
  pqInternal::TrackMapType::iterator iter;

  // add new tracks
  foreach(pqAnimationCue* cue, cues)
    {
    QString completeName = this->Internal->cueName(cue);

    iter = this->Internal->TrackMap.find(cue);

    if(iter == this->Internal->TrackMap.end())
      {
      pqAnimationTrack* t = animModel->addTrack();
      this->Internal->TrackMap.insert(cue, t);
      t->setProperty(completeName);
      this->Internal->KeyFramesChanged.setMapping(cue, cue);
      QObject::connect(cue, SIGNAL(keyframesModified()),
        &this->Internal->KeyFramesChanged,
        SLOT(map()));
      }
    else
      {
      oldCues.remove(cue);
      }
    }

  // remove old tracks
  for(iter = oldCues.begin(); iter != oldCues.end(); iter++)
    {
    animModel->removeTrack(iter.value());
    this->Internal->TrackMap.remove(iter.key());
    if(iter.key())
      {
      QObject::disconnect(iter.key(), SIGNAL(keyframesModified()),
        &this->Internal->KeyFramesChanged,
        SLOT(map()));
      }
    }

}
  
void pqAnimationViewWidget::keyFramesChanged(QObject* cueObject)
{
  pqAnimationCue* cue = qobject_cast<pqAnimationCue*>(cueObject);
  pqAnimationTrack* track = this->Internal->findTrack(cue);

  QList<vtkSMProxy*> keyFrames = cue->getKeyFrames();

  bool camera = this->Internal->cameraCue(cue);

  // clean out old ones
  while(track->count())
    {
    track->removeKeyFrame(track->keyFrame(0));
    }

  for(int j=0; j<keyFrames.count()-1; j++)
    {
    QIcon icon;
    QVariant startValue;
    QVariant endValue;
      
    double startTime =
      pqSMAdaptor::getElementProperty(keyFrames[j]->GetProperty("KeyTime")).toDouble();
    double endTime =
      pqSMAdaptor::getElementProperty(keyFrames[j+1]->GetProperty("KeyTime")).toDouble();

    if(!camera)
      {
      QVariant interpolation =
        pqSMAdaptor::getEnumerationProperty(keyFrames[j]->GetProperty("Type"));
      if(interpolation == "Boolean")
        interpolation = "Step";
      else if(interpolation == "Sinusoid")
        interpolation = "Sinusoidal";
      QString iconstr =
        QString(":pqWidgets/Icons/pq%1%2.png").arg(interpolation.toString()).arg(16);
      icon = QIcon(iconstr);
      
      startValue =
        pqSMAdaptor::getElementProperty(keyFrames[j]->GetProperty("KeyValues"));
      endValue =
        pqSMAdaptor::getElementProperty(keyFrames[j+1]->GetProperty("KeyValues"));
      }

    pqAnimationKeyFrame* newFrame = track->addKeyFrame();
    newFrame->setStartTime(startTime);
    newFrame->setEndTime(endTime);
    newFrame->setStartValue(startValue);
    newFrame->setEndValue(endValue);
    newFrame->setIcon(QIcon(icon));
    }
}

void pqAnimationViewWidget::updateSceneTimeRange()
{
  pqAnimationModel* animModel =
    this->Internal->AnimationWidget->animationModel();
  QPair<double, double> timeRange = this->Internal->Scene->getClockTimeRange();
  animModel->setStartTime(timeRange.first);
  animModel->setEndTime(timeRange.second);
}

void pqAnimationViewWidget::updateSceneTime()
{
  double time =
    this->Internal->Scene->getAnimationSceneProxy()->GetAnimationTime();

  pqAnimationModel* animModel =
    this->Internal->AnimationWidget->animationModel();
  animModel->setCurrentTime(time);
}


void pqAnimationViewWidget::trackSelected(pqAnimationTrack* track)
{
  pqAnimationCue* cue = this->Internal->findCue(track);
  if(!cue)
    {
    return;
    }

  if(this->Internal->Editor)
    {
    this->Internal->Editor->raise();
    return;
    }

  this->Internal->Editor = new QDialog;
  this->Internal->Editor->setAttribute(Qt::WA_QuitOnClose, false);
  this->Internal->Editor->setAttribute(Qt::WA_DeleteOnClose);
  this->Internal->Editor->resize(500, 400);
  this->Internal->Editor->setWindowTitle(tr("Animation Keyframes"));
  QVBoxLayout* l = new QVBoxLayout(this->Internal->Editor);
  pqKeyFrameEditor* editor = new pqKeyFrameEditor(this->Internal->Scene,
                                                  cue, this->Internal->Editor);
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                              | QDialogButtonBox::Cancel);
  l->addWidget(editor);
  l->addWidget(buttons);

  connect(this->Internal->Editor, SIGNAL(accepted()), 
          editor, SLOT(writeKeyFrameData()));
  connect(buttons, SIGNAL(accepted()), 
          this->Internal->Editor, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), 
          this->Internal->Editor, SLOT(reject()));

  this->Internal->Editor->show();
}
  
void pqAnimationViewWidget::updatePlayMode()
{
  pqAnimationModel* animModel =
    this->Internal->AnimationWidget->animationModel();
  vtkSMProxy* pxy = this->Internal->Scene->getProxy();

  QString mode = pqSMAdaptor::getEnumerationProperty(
    pxy->GetProperty("PlayMode")).toString();

  if(mode == "Real Time")
    {
    animModel->setMode(pqAnimationModel::Real);
    }
  else if(mode == "Sequence")
    {
    animModel->setMode(pqAnimationModel::Sequence);
    }
  else if(mode == "Snap To TimeSteps")
    {
    animModel->setMode(pqAnimationModel::Sequence);
    }
  else
    {
    qWarning("Unrecognized play mode");
    }

}
  
void pqAnimationViewWidget::updateTicks()
{
  pqAnimationModel* animModel =
    this->Internal->AnimationWidget->animationModel();
  int num = this->Internal->numberOfTicks();
  animModel->setTicks(num);
}


