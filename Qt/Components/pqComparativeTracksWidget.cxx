/*=========================================================================

   Program: ParaView
   Module:    pqComparativeTracksWidget.cxx

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
#include "pqComparativeTracksWidget.h"

// Server Manager Includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMProxyProperty.h"

// Qt Includes.
#include <QDialog>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QTimer>
#include <QVBoxLayout>

// ParaView Includes.
#include "pqAnimationCue.h"
#include "pqAnimationKeyFrame.h"
#include "pqAnimationModel.h"
#include "pqAnimationTrack.h"
#include "pqAnimationWidget.h"
#include "pqApplicationCore.h"
#include "pqKeyFrameEditor.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

class pqComparativeTracksWidget::pqInternal
{
public:
  pqAnimationWidget* AnimationWidget;
  vtkEventQtSlotConnect* VTKConnect;
  vtkSmartPointer<vtkSMComparativeViewProxy> CVProxy;
  QTimer Timer;
  QMap<pqAnimationTrack*, vtkSmartPointer<vtkSMAnimationCueProxy> > TrackMap;

  pqInternal(QWidget* parent)
    {
    this->AnimationWidget = new pqAnimationWidget(parent);
    this->AnimationWidget->createDeleteHeader()->hide();
    this->AnimationWidget->enabledHeader()->hide();
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

  pqAnimationCue* findCue(pqAnimationTrack* track)
    {
    if (!this->TrackMap.contains(track))
      {
      return false;
      }

    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    return smmodel->findItem<pqAnimationCue*>(this->TrackMap[track]);
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

  QObject::connect(this->Internal->AnimationWidget,
    SIGNAL(trackSelected(pqAnimationTrack*)),
    this, SLOT(trackSelected(pqAnimationTrack*)));
}

//-----------------------------------------------------------------------------
pqComparativeTracksWidget::~pqComparativeTracksWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqComparativeTracksWidget::setComparativeView(vtkSMProxy* cvProxy)
{
  if (this->Internal->CVProxy.GetPointer() == cvProxy)
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


  this->Internal->CVProxy = vtkSMComparativeViewProxy::SafeDownCast(cvProxy);
  if (!this->Internal->CVProxy)
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
  if (!this->Internal->CVProxy)
    {
    return;
    }
  int mode = pqSMAdaptor::getElementProperty(
    this->Internal->CVProxy->GetProperty("Mode")).toInt();

  // Clean all tracks.
  this->Internal->TrackMap.clear();
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

  this->Internal->TrackMap[track] = cueProxy;
  track->setProperty(this->Internal->cueName(cueProxy));
  pp = vtkSMProxyProperty::SafeDownCast(cueProxy->GetProperty("KeyFrames"));

  if (pp && pp->GetNumberOfProxies() == 2)
    {
    // Update keyframes from this cue. For now, there can only be 1 keyframes in
    // each cue.
    QVariant startValue =
      pqSMAdaptor::getElementProperty(pp->GetProxy(0)->GetProperty("KeyValues"));
    QVariant endValue =
      pqSMAdaptor::getElementProperty(pp->GetProxy(1)->GetProperty("KeyValues"));

    pqAnimationKeyFrame* newFrame = track->addKeyFrame();
    newFrame->setNormalizedStartTime(0);
    newFrame->setNormalizedEndTime(1);
    newFrame->setStartValue(startValue);
    newFrame->setEndValue(endValue);

    QVariant interpolation =
      pqSMAdaptor::getEnumerationProperty(pp->GetProxy(0)->GetProperty("Type"));
    if(interpolation == "Boolean")
      interpolation = "Step";
    else if(interpolation == "Sinusoid")
      interpolation = "Sinusoidal";
    QString icon =
      QString(":pqWidgets/Icons/pq%1%2.png").arg(interpolation.toString()).arg(16);
    newFrame->setIcon(QIcon(icon));
    }
}

//-----------------------------------------------------------------------------
void pqComparativeTracksWidget::trackSelected(pqAnimationTrack* track)
{
  pqAnimationCue* cue = this->Internal->findCue(track);
  if(!cue)
    {
    return;
    }

  QDialog dialog;
  dialog.resize(500, 400);
  dialog.setWindowTitle(tr("Comparative Visualization Keyframes"));
  QVBoxLayout* l = new QVBoxLayout(&dialog);
  pqKeyFrameEditor* editor = new pqKeyFrameEditor(0, cue, QString(), &dialog);
  editor->setValuesOnly(true);
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                              | QDialogButtonBox::Cancel);
  l->addWidget(editor);
  l->addWidget(buttons);

  connect(&dialog, SIGNAL(accepted()), editor, SLOT(writeKeyFrameData()));
  connect(buttons, SIGNAL(accepted()), &dialog, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));

  dialog.exec();
}
  
