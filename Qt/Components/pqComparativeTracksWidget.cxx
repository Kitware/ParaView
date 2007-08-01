/*=========================================================================

   Program: ParaView
   Module:    pqComparativeTracksWidget.cxx

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
#include "pqComparativeTracksWidget.h"

// Server Manager Includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMProxyProperty.h"

// Qt Includes.
#include <QVBoxLayout>
#include <QTimer>

// ParaView Includes.
#include "pqAnimationWidget.h"
#include "pqAnimationModel.h"
#include "pqAnimationTrack.h"
#include "pqAnimationKeyFrame.h"
#include "pqSMAdaptor.h"

class pqComparativeTracksWidget::pqInternal
{
public:
  pqAnimationWidget* AnimationWidget;
  vtkEventQtSlotConnect* VTKConnect;
  vtkSmartPointer<vtkSMProxy> CVProxy;
  QTimer Timer;

  pqInternal(QWidget* parent)
    {
    this->AnimationWidget = new pqAnimationWidget(parent);
    this->VTKConnect = vtkEventQtSlotConnect::New();
    }

  ~pqInternal()
    {
    this->VTKConnect->Delete();
    delete this->AnimationWidget;
    }

  pqAnimationModel* model()
    {
    return this->AnimationWidget->animationModel();
    }

  QString cueName(vtkSMAnimationCueProxy* cue)
    {
    QString name;
    vtkSMProxy* pxy = cue->GetAnimatedProxy();
    vtkSMProperty* pty = cue->GetAnimatedProperty();
    if(pxy && pty)
      {
      QString p = pxy->GetPropertyName(pty);
      if(pqSMAdaptor::getPropertyType(pty) == pqSMAdaptor::MULTIPLE_ELEMENTS)
        {
        p = QString("%1 (%2)").arg(p).arg(cue->GetAnimatedElement());
        }
      name = QString("%1").arg(p);
      }
    return name;
    }

};

//-----------------------------------------------------------------------------
pqComparativeTracksWidget::pqComparativeTracksWidget(QWidget* _parent)
:Superclass(_parent)
{
  this->Internal = new pqInternal(this);
  this->Internal->Timer.setSingleShot(true);
  QObject::connect(&this->Internal->Timer, SIGNAL(timeout()),
    this, SLOT(updateScene()));

  QVBoxLayout* vboxlayout = new QVBoxLayout(this);
  vboxlayout->addWidget(this->Internal->AnimationWidget);
}

//-----------------------------------------------------------------------------
pqComparativeTracksWidget::~pqComparativeTracksWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqComparativeTracksWidget::setComparativeView(vtkSMProxy* cvProxy)
{
  if (this->Internal->CVProxy == cvProxy)
    {
    return;
    }

  pqAnimationModel* model = this->Internal->model();

  this->Internal->VTKConnect->Disconnect();

  // Clean all tracks.
  while (model->count())
    {
    model->removeTrack(model->track(0));
    }

  this->Internal->CVProxy = cvProxy;
  if (!cvProxy)
    {
    return;
    }
 
  this->Internal->VTKConnect->Connect(
    cvProxy, vtkCommand::ModifiedEvent, 
    this, SLOT(updateSceneCallback()));

  // Update the panel to reflect the state of the view set.
}

//-----------------------------------------------------------------------------
void pqComparativeTracksWidget::updateSceneCallback()
{
  // ensures that we update only after all ModifiedEvent have been fired.
  this->Internal->Timer.start();
}


//-----------------------------------------------------------------------------
void pqComparativeTracksWidget::updateScene()
{
  int mode = pqSMAdaptor::getElementProperty(
    this->Internal->CVProxy->GetProperty("Mode")).toInt();

  // Clean all tracks.
  pqAnimationModel* model = this->Internal->model();
  while (model->count())
    {
    model->removeTrack(model->track(0));
    }
  
  this->updateTrack(0, this->Internal->CVProxy->GetProperty("XCues"));

  if (mode == vtkSMComparativeViewProxy::COMPARATIVE)
    {
    this->updateTrack(1, this->Internal->CVProxy->GetProperty("YCues"));
    }
}


//-----------------------------------------------------------------------------
void pqComparativeTracksWidget::updateTrack(int index, vtkSMProperty* smproperty)
{
  pqAnimationModel* model = this->Internal->model();
  while (model->count() <= index)
    {
    model->addTrack();  
    }

  pqAnimationTrack* track = model->track(index);
  while (track->count())
    {
    track->removeKeyFrame(track->keyFrame(0));
    }

  // Find the first enabled cue.
  vtkSMAnimationCueProxy* cueProxy = 0;
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(smproperty);
  for (unsigned int cc=0; cc < pp->GetNumberOfProxies(); cc++)
    {
    cueProxy = vtkSMAnimationCueProxy::SafeDownCast(pp->GetProxy(cc));
    if (cueProxy && cueProxy->GetEnabled())
      {
      break;
      }
    cueProxy = 0;
    }

  if (!cueProxy)
    {
    // no active track
    track->setProperty("...");
    return;
    }

  track->setProperty(this->Internal->cueName(cueProxy));
  pp = vtkSMProxyProperty::SafeDownCast(cueProxy->GetProperty("KeyFrames"));

  if (pp->GetNumberOfProxies() == 2)
    {
    // Update keyframes from this cue. For now, there can only be 1 keyframes in
    // each cue.
    QVariant startValue =
      pqSMAdaptor::getElementProperty(pp->GetProxy(0)->GetProperty("KeyValues"));
    QVariant endValue =
      pqSMAdaptor::getElementProperty(pp->GetProxy(1)->GetProperty("KeyValues"));

    pqAnimationKeyFrame* newFrame = track->addKeyFrame();
    newFrame->setStartTime(0);
    newFrame->setEndTime(1);
    newFrame->setStartValue(startValue);
    newFrame->setEndValue(endValue);
    }
}
