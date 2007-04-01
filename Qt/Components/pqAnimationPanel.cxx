/*=========================================================================

   Program: ParaView
   Module:    pqAnimationPanel.cxx

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

#include "pqAnimationPanel.h"
#include "ui_pqAnimationPanel.h"

#include "vtkClientServerID.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMVectorProperty.h"

#include <QDoubleValidator>
#include <QIntValidator>
#include <QMetaType>
#include <QPointer>
#include <QScrollArea>
#include <QtDebug>
#include <QToolBar>

#include "pqActiveView.h"
#include "pqAnimationCue.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqDoubleSpinBoxDomain.h"
#include "pqKeyFrameTimeValidator.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqRenderViewModule.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"
#include "pqSignalAdaptorKeyFrameTime.h"
#include "pqSignalAdaptorKeyFrameType.h"
#include "pqSignalAdaptorKeyFrameValue.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqTimeKeeper.h"

//-----------------------------------------------------------------------------
class pqAnimationPanel::pqInternals : public Ui::AnimationPanel
{
public:
  QPointer<pqAnimationManager> Manager;
  QPointer<pqProxy> CurrentProxy;
  QPointer<pqAnimationCue> ActiveCue;
  vtkSmartPointer<vtkSMProxy> ActiveKeyFrame;
  QPointer<pqSignalAdaptorKeyFrameValue> ValueAdaptor;
  QPointer<pqSignalAdaptorKeyFrameType> TypeAdaptor;
  QPointer<pqSignalAdaptorKeyFrameTime> TimeAdaptor;
  QPointer<pqKeyFrameTimeValidator> KeyFrameTimeValidator;
  QPointer<pqAnimationScene> ActiveScene;
  QPointer<pqRenderViewModule> ActiveView;

  QPointer<QLineEdit> ToolbarCurrentTimeWidget;
  QPointer<QSpinBox> ToolbarCurrentTimeIndexWidget;

  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqSignalAdaptorComboBox* PlayModeAdaptor;
  pqPropertyLinks KeyFrameLinks;
  pqPropertyLinks SceneLinks;
  struct PropertyInfo
    {
    vtkSmartPointer<vtkSMProxy> Proxy;
    QString Name;
    int Index;
    };

  pqInternals()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};

Q_DECLARE_METATYPE(pqAnimationPanel::pqInternals::PropertyInfo);

//-----------------------------------------------------------------------------
pqAnimationPanel::pqAnimationPanel(QWidget* _parent) : QWidget(_parent)
{
  this->Internal = new pqAnimationPanel::pqInternals();
  QVBoxLayout* vboxlayout = new QVBoxLayout(this);
  vboxlayout->setSpacing(0);
  vboxlayout->setMargin(0);
  vboxlayout->setObjectName("vboxLayout");

  QWidget* container = new QWidget(this);
  container->setObjectName("scrollWidget");
  container->setSizePolicy(QSizePolicy::MinimumExpanding,
    QSizePolicy::MinimumExpanding);

  QScrollArea* s = new QScrollArea(this);
  s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setWidgetResizable(true);
  s->setObjectName("scrollArea");
  s->setFrameShape(QFrame::NoFrame);
  s->setWidget(container);
  vboxlayout->addWidget(s);

  this->Internal->setupUi(container);
  this->Internal->cameraFrame->hide();

  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Internal->currentTime->setValidator(validator);
  this->Internal->startTime->setValidator(validator);
  this->Internal->endTime->setValidator(validator);

  this->Internal->PlayModeAdaptor = 
    new pqSignalAdaptorComboBox(this->Internal->playMode);

  this->Internal->KeyFrameTimeValidator = new pqKeyFrameTimeValidator(this);
  this->Internal->keyFrameTime->setValidator(
    this->Internal->KeyFrameTimeValidator);

  QObject::connect(pqApplicationCore::instance()->getSelectionModel(),
    SIGNAL(currentChanged(pqServerManagerModelItem*)),
    this, SLOT(onCurrentChanged(pqServerManagerModelItem*)));

  QObject::connect(
    this->Internal->currentTimeIndex, SIGNAL(valueChanged(int)),
    this, SLOT(setCurrentTimeByIndex(int)));
  QObject::connect(
    this->Internal->startTimeIndex, SIGNAL(valueChanged(int)),
    this, SLOT(setStartTimeByIndex(int)));
  QObject::connect(
    this->Internal->endTimeIndex, SIGNAL(valueChanged(int)),
    this, SLOT(setEndTimeByIndex(int)));

  QObject::connect(
    this->Internal->sourceName, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onCurrentSourceChanged(int)));

  QObject::connect(
    this->Internal->propertyName, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onCurrentPropertyChanged(int)));

  QObject::connect(
    this->Internal->addKeyFrame, SIGNAL(clicked()),
    this, SLOT(addKeyFrameCallback()));

  QObject::connect(
    this->Internal->deleteKeyFrame, SIGNAL(clicked()),
    this, SLOT(deleteKeyFrameCallback()));

  QObject::connect(
    this->Internal->keyFrameIndex, SIGNAL(valueChanged(int)),
    this, SLOT(showKeyFrameCallback(int)));

  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(preSourceRemoved(pqPipelineSource*)),
    this, SLOT(onSourceRemoved(pqPipelineSource*)));
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(sourceAdded(pqPipelineSource*)),
    this, SLOT(onSourceAdded(pqPipelineSource*)));

  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(nameChanged(pqServerManagerModelItem*)),
    this, SLOT(onNameChanged(pqServerManagerModelItem*)));

  QObject::connect(&pqActiveView::instance(),
    SIGNAL(changed(pqGenericViewModule*)),
    this, SLOT(onActiveViewChanged(pqGenericViewModule*)));

  QObject::connect(this->Internal->useCurrent,
    SIGNAL(clicked(bool)), 
    this, SLOT(resetCameraKeyFrameToCurrent()));

  this->Internal->ValueAdaptor = new pqSignalAdaptorKeyFrameValue(
    this->Internal->valueFrameLarge, this->Internal->valueFrame);

  this->Internal->TypeAdaptor = new pqSignalAdaptorKeyFrameType(
    this->Internal->interpolationType, 
    this->Internal->valueLabel,
    this->Internal->typeFrame);

  this->Internal->TimeAdaptor = new pqSignalAdaptorKeyFrameTime(
    this->Internal->keyFrameTime, "text",
    SIGNAL(textChanged(const QString&)));


  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqAnimationPanel::~pqAnimationPanel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::onSourceAdded(pqPipelineSource* src)
{
  this->Internal->sourceName->addItem(src->getSMName(), 
    QVariant(src->getProxy()->GetSelfID().ID));
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::onSourceRemoved(pqPipelineSource* source)
{
  int index = this->Internal->sourceName->findData(
    QVariant(source->getProxy()->GetSelfID().ID));
  if (index != -1)
    {
    this->Internal->sourceName->removeItem(index);
    pqAnimationManager* mgr = this->Internal->Manager;
    pqAnimationScene* scene = mgr->getScene(source->getServer());
    if (scene)
      {
      scene->removeCues(source->getProxy());

      // we also want to remove cues we might have
      // added for the proxies on proxy properties. 
      // We are assured that all those proxies are saved
      // as internal proxies for the source.
      QList<vtkSMProxy*> internalProxies = source->getHelperProxies();
      foreach (vtkSMProxy* proxy, internalProxies)
        {
        scene->removeCues(proxy);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::onNameChanged(pqServerManagerModelItem* item)
{
  pqPipelineSource* src = qobject_cast<pqPipelineSource*>(item);
  if (src)
    {
    int index = this->Internal->sourceName->findData(
        QVariant(src->getProxy()->GetSelfID().ID));
    if (index != -1 
      && src->getSMName() != this->Internal->sourceName->itemText(index))
      {
      this->Internal->sourceName->blockSignals(true);
      this->Internal->sourceName->insertItem(index, src->getSMName(),
        QVariant(src->getProxy()->GetSelfID().ID));
      this->Internal->sourceName->removeItem(index+1);
      this->Internal->sourceName->blockSignals(false);
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::setManager(pqAnimationManager* mgr)
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
void pqAnimationPanel::onActiveSceneChanged(pqAnimationScene* scene)
{
  if (this->Internal->ActiveScene)
    {
    QObject::disconnect(this->Internal->ActiveScene, 0, this, 0);
    QObject::disconnect(
      this->Internal->ActiveScene->getServer()->getTimeKeeper(), 0, this, 0);
    this->Internal->SceneLinks.removeAllPropertyLinks();
    }

  this->Internal->ActiveScene = scene;
  if (!scene)
    {
    this->Internal->playbackGroup->setEnabled(false);
    this->setActiveCue(0);
    this->updateEnableState();
    return;
    }

  this->Internal->playbackGroup->setEnabled(true);
  vtkSMProxy* sceneProxy = scene->getProxy();
  sceneProxy->UpdatePropertyInformation();

  // update domain to currentFrame before creating the link.
  this->onScenePlayModeChanged();

  this->Internal->SceneLinks.addPropertyLink(
    this->Internal->currentTime, "text", SIGNAL(textChanged(const QString&)),
    sceneProxy, sceneProxy->GetProperty("ClockTime"));
  this->Internal->SceneLinks.addPropertyLink(
    this->Internal->startTime, "text", SIGNAL(textChanged(const QString&)),
    sceneProxy, sceneProxy->GetProperty("ClockTimeRange"), 0);
  this->Internal->SceneLinks.addPropertyLink(
    this->Internal->endTime, "text", SIGNAL(textChanged(const QString&)),
    sceneProxy, sceneProxy->GetProperty("ClockTimeRange"), 1);
  this->Internal->SceneLinks.addPropertyLink(
    this->Internal->startTimeLock, "checked", SIGNAL(toggled(bool)),
    sceneProxy, sceneProxy->GetProperty("ClockTimeRangeLocks"), 0);
  this->Internal->SceneLinks.addPropertyLink(
    this->Internal->endTimeLock, "checked", SIGNAL(toggled(bool)),
    sceneProxy, sceneProxy->GetProperty("ClockTimeRangeLocks"), 1);

  this->Internal->SceneLinks.addPropertyLink(
    this->Internal->PlayModeAdaptor, "currentText", 
    SIGNAL(currentTextChanged(const QString&)),
    sceneProxy, sceneProxy->GetProperty("PlayMode"));

  this->Internal->SceneLinks.addPropertyLink(
    this->Internal->numberOfFrames, "value", SIGNAL(valueChanged(int)),
    sceneProxy, sceneProxy->GetProperty("NumberOfFrames"));
  this->Internal->SceneLinks.addPropertyLink(
    this->Internal->duration, "value", SIGNAL(valueChanged(int)),
    sceneProxy, sceneProxy->GetProperty("Duration"));
  this->Internal->SceneLinks.addPropertyLink(
    this->Internal->caching, "checked", SIGNAL(toggled(bool)),
    sceneProxy, sceneProxy->GetProperty("Caching"));

  QObject::connect(scene, SIGNAL(playModeChanged()),
    this, SLOT(onScenePlayModeChanged()));
  QObject::connect(scene, SIGNAL(cuesChanged()), 
    this, SLOT(onSceneCuesChanged()));

  // Whenever timesteps change, we want to update the ranges for the spin boxes
  // which show the timestep index.
  pqTimeKeeper* timekeeper = scene->getServer()->getTimeKeeper();
  QObject::connect(timekeeper, SIGNAL(timeStepsChanged()),
    this, SLOT(onTimeStepsChanged()));
  QObject::connect(timekeeper, SIGNAL(timeChanged()),
    this, SLOT(onTimeChanged()));
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::onTimeStepsChanged()
{
  int max = 0;
  if (this->Internal->ActiveScene)
    {
    pqTimeKeeper* timekeeper = 
      this->Internal->ActiveScene->getServer()->getTimeKeeper();
    max = timekeeper->getNumberOfTimeStepValues();
    max = (max >0)? (max-1):max;
    }

  this->Internal->currentTimeIndex->setRange(0, max);
  this->Internal->startTimeIndex->setRange(0, max);
  this->Internal->endTimeIndex->setRange(0, max);

  if (this->Internal->ToolbarCurrentTimeIndexWidget)
    {
    this->Internal->ToolbarCurrentTimeIndexWidget->setRange(0, max);
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::onSceneCuesChanged()
{
  if (this->Internal->ActiveCue && 
    !this->Internal->ActiveScene->contains(this->Internal->ActiveCue))
    {
    // Active cue has been removed, so clean the GUI.
    this->setActiveCue(0);
    this->updateEnableState();
    }

  if (!this->Internal->ActiveCue && this->Internal->CurrentProxy)
    {
    // It is possible that the scene has detected a new cue for the 
    // currently selected property, this call will show that.
    this->onCurrentPropertyChanged(
      this->Internal->propertyName->currentIndex());
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::onScenePlayModeChanged()
{
  vtkSMProxy* proxy = this->Internal->ActiveScene->getProxy();
  QString playmode = 
    pqSMAdaptor::getEnumerationProperty(proxy->GetProperty("PlayMode")).toString();

  this->Internal->currentTimeIndex->setEnabled(false);
  this->Internal->startTimeIndex->setEnabled(false);
  this->Internal->endTimeIndex->setEnabled(false);
  this->Internal->currentTime->setEnabled(true);
  this->Internal->startTime->setEnabled(true);
  this->Internal->endTime->setEnabled(true);
  if (this->Internal->ToolbarCurrentTimeWidget)
    {
    this->Internal->ToolbarCurrentTimeWidget->setEnabled(true);
    }
  if (this->Internal->ToolbarCurrentTimeIndexWidget)
    {
    this->Internal->ToolbarCurrentTimeIndexWidget->setEnabled(false);
    }

  if (playmode == "Sequence")
    {
    this->Internal->numberOfFrames->show();
    this->Internal->labelNumberOfFrames->show();
    this->Internal->labelDuration->hide();
    this->Internal->duration->hide();

    }
  else if (playmode == "Real Time")
    {
    this->Internal->numberOfFrames->hide();
    this->Internal->labelNumberOfFrames->hide();
    this->Internal->labelDuration->show();
    this->Internal->duration->show();
    }
  else // playmode == "Snap To TimeSteps"
    {
    this->Internal->numberOfFrames->hide();
    this->Internal->labelNumberOfFrames->hide();
    this->Internal->labelDuration->hide();
    this->Internal->duration->hide();
    this->Internal->currentTimeIndex->setEnabled(true);
    this->Internal->startTimeIndex->setEnabled(true);
    this->Internal->endTimeIndex->setEnabled(true);
    this->Internal->currentTime->setEnabled(false);
    this->Internal->startTime->setEnabled(false);
    this->Internal->endTime->setEnabled(false);
    if (this->Internal->ToolbarCurrentTimeWidget)
      {
      this->Internal->ToolbarCurrentTimeWidget->setEnabled(false);
      }
    if (this->Internal->ToolbarCurrentTimeIndexWidget)
      {
      this->Internal->ToolbarCurrentTimeIndexWidget->setEnabled(true);
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::setActiveCue(pqAnimationCue* cue)
{
  if (this->Internal->ActiveCue == cue)
    {
    return;
    }
  // Clean up old keyframe stuff.
  this->showKeyFrame(-1);

  if (this->Internal->ActiveCue)
    {
    QObject::disconnect(this->Internal->ActiveCue, 0, this, 0);
    }
  this->Internal->ActiveCue = cue;
  if (this->Internal->ActiveCue)
    {
    QObject::connect(this->Internal->ActiveCue, SIGNAL(keyframesModified()),
      this, SLOT(onKeyFramesModified()));
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::updateEnableState()
{
  bool enable = (this->Internal->CurrentProxy!= 0);

  this->Internal->propertyName->setEnabled(enable);

  bool has_cue = enable && (this->Internal->propertyName->currentIndex()>=0);
  this->Internal->addKeyFrame->setEnabled(has_cue);

  int num_keyframes = 
    (has_cue && this->Internal->ActiveCue)? 
    this->Internal->ActiveCue->getNumberOfKeyFrames() : 0;

  bool has_keyframe = (num_keyframes > 0);

  this->Internal->editorFrame->setEnabled(has_keyframe);
  this->Internal->deleteKeyFrame->setEnabled(has_keyframe);

  // Cannot change the interpolation type for the last keyframe.
  if (num_keyframes == (this->Internal->keyFrameIndex->value()+1))
    {
    this->Internal->interpolationType->setEnabled(false);
    this->Internal->typeFrame->setEnabled(false);
    }
  else
    {
    this->Internal->interpolationType->setEnabled(true);
    this->Internal->typeFrame->setEnabled(true);
    }
}

//-----------------------------------------------------------------------------
// Called when the pipeline brower selection changes.
void pqAnimationPanel::onCurrentChanged(pqServerManagerModelItem* current)
{
  pqProxy* src = qobject_cast<pqProxy*>(current);
  if (!src)
    {
    // We don't change the selection if the request is to select nothing 
    // at all.
    return;
    }
  this->onCurrentChanged(src);
}

//-----------------------------------------------------------------------------
// Called when pipeline browser selection changes or the source combobox
// selection changes.
void pqAnimationPanel::onCurrentChanged(pqProxy* src)
{
  if (this->Internal->CurrentProxy == src)
    {
    return;
    }

  // clear property listing.
  this->Internal->VTKConnect->Disconnect();
  this->Internal->propertyName->clear();
  this->setActiveCue(0);

  this->Internal->CurrentProxy = src;
  if (!src)
    {
    this->updateEnableState();
    return;
    }
  
  int index = this->Internal->sourceName->findData(
    QVariant(src->getProxy()->GetSelfID().ID));
  if (index == -1)
    {
    this->Internal->CurrentProxy = 0;
    this->updateEnableState();
    this->Internal->sourceName->setCurrentIndex(-1);
    return;
    }

  this->Internal->sourceName->setCurrentIndex(index);
  this->updateEnableState();
  this->buildPropertyList();
}

//-----------------------------------------------------------------------------
// called when the source combo-box changes.
void pqAnimationPanel::onCurrentSourceChanged(int index)
{
  pqProxy* src = 0;
  if (index != -1)
    {
    QString pname = this->Internal->sourceName->itemText(index);
    if (pname == "Camera" && this->Internal->ActiveView)
      {
      src = this->Internal->ActiveView;
      }
    else
      {
      src = pqApplicationCore::instance()->
        getServerManagerModel()-> getPQSource(pname);
      }
    }
#if 0
  pqApplicationCore::instance()->getSelectionModel()->setCurrentItem(
    src, pqServerManagerSelectionModel::ClearAndSelect);
#else
  // Since we decided not to update the application selection, we
  // explictly call this method otherwise it would have been called
  // as a side effect of changing the selection.
  this->onCurrentChanged(src);
#endif
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::onActiveViewChanged(pqGenericViewModule* view)
{
  pqRenderViewModule* rview = qobject_cast<pqRenderViewModule*>(view);
  if (this->Internal->ActiveView == rview)
    {
    return;
    }

  this->Internal->ActiveView = rview;
  if (rview && this->Internal->sourceName->findText("Camera") == -1)
    {
    this->Internal->sourceName->insertItem(0, "Camera",
      QVariant(rview->getProxy()->GetSelfID().ID));

    }
  if (!rview && this->Internal->sourceName->findText("Camera") != -1)
    {
    this->Internal->sourceName->removeItem(0);
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::buildPropertyList()
{
  this->Internal->VTKConnect->Disconnect();
  this->Internal->propertyName->clear();
  if (!this->Internal->CurrentProxy)
    {
    return;
    }

  if (this->Internal->CurrentProxy == this->Internal->ActiveView)
    {
    // Render modules are never actulally animated, we animate their camera.
    pqAnimationPanel::pqInternals::PropertyInfo info;
    info.Proxy = this->Internal->CurrentProxy->getProxy();
    info.Name = "Camera";
    info.Index =  -1;
    this->Internal->propertyName->addItem("Camera", QVariant::fromValue(info));
    return;
    }

  this->buildPropertyList(this->Internal->CurrentProxy->getProxy(), QString());

}

//-----------------------------------------------------------------------------
void pqAnimationPanel::buildPropertyList(vtkSMProxy* proxy, 
  const QString& labelPrefix)
{
  if (!proxy)
    {
    return;
    }

  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(proxy->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMVectorProperty* smproperty = 
      vtkSMVectorProperty::SafeDownCast(iter->GetProperty());
    if (!smproperty || !smproperty->GetAnimateable() || 
      smproperty->GetInformationOnly())
      {
      continue;
      }
    unsigned int num_elems = smproperty->GetNumberOfElements();
    if (smproperty->GetRepeatCommand())
      {
      num_elems = 1;
      }
    for (unsigned int cc=0; cc < num_elems; cc++)
      {
      pqAnimationPanel::pqInternals::PropertyInfo info;
      info.Proxy = proxy;
      info.Name = iter->GetKey();
      info.Index = smproperty->GetRepeatCommand()? -1 : static_cast<int>(cc);

      QString label = labelPrefix.isEmpty()? "" : labelPrefix + " - ";
      label += iter->GetProperty()->GetXMLLabel();
      label = (num_elems>1) ? label + " (" + QString::number(cc) + ")" 
        : label;
      this->Internal->propertyName->addItem(label, 
        QVariant::fromValue(info));
      }
    }

  // Now add properties of all proxies in properties that have
  // proxy lists.
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxyProperty* smproperty = 
      vtkSMProxyProperty::SafeDownCast(iter->GetProperty());
    if (smproperty && 
      pqSMAdaptor::getPropertyType(smproperty) == pqSMAdaptor::PROXYSELECTION)
      {
      vtkSMProxy* child_proxy = pqSMAdaptor::getProxyProperty(smproperty);
      QString newPrefix = labelPrefix.isEmpty()? "" : labelPrefix + ":";
      newPrefix += smproperty->GetXMLLabel();
      this->buildPropertyList(child_proxy, newPrefix);

      // if this property's value changes, we'll have to rebuild
      // the property names menu.
      this->Internal->VTKConnect->Connect(smproperty, vtkCommand::ModifiedEvent,
        this, SLOT(buildPropertyList()), 0, 0, Qt::QueuedConnection);
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::onCurrentPropertyChanged(int index)
{
  pqAnimationManager* mgr = this->Internal->Manager;
  pqAnimationScene* scene = mgr->getActiveScene();
  pqAnimationCue* cue = 0;

  if (scene)
    {
    pqAnimationPanel::pqInternals::PropertyInfo info =
      this->Internal->propertyName->itemData(index).value<
      pqAnimationPanel::pqInternals::PropertyInfo>();

    cue = mgr->getCue(scene, info.Proxy, 
      info.Name.toAscii().data(), info.Index);
    }

  this->setActiveCue(cue); 

  if (cue && cue->getNumberOfKeyFrames() > 0)
    {
    this->showKeyFrame(0);
    }
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::resetCameraKeyFrameToCurrent()
{
  vtkSMRenderModuleProxy* src = 
    this->Internal->ActiveView->getRenderModuleProxy();
  src->SynchronizeCameraProperties();

  vtkSMProxy* dest = this->Internal->ActiveKeyFrame;
  if (!src || !dest)
    {
    return;
    }

  const char* names[] = { "Position", "FocalPoint", "ViewUp", "ViewAngle",  0 };
  const char* snames[] = { "CameraPositionInfo", "CameraFocalPointInfo", 
    "CameraViewUpInfo",  "CameraViewAngle", 0 };
  for (int cc=0; names[cc] && snames[cc]; cc++)
    {
    vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
      src->GetProperty(snames[cc]));
    vtkSMDoubleVectorProperty* ddvp = vtkSMDoubleVectorProperty::SafeDownCast(
      dest->GetProperty(names[cc]));
    if (sdvp && ddvp)
      {
      ddvp->Copy(sdvp);
      ddvp->Modified();
      }
    }
  dest->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::addKeyFrameCallback()
{
  int index=0;
  if (this->Internal->ActiveCue && 
    this->Internal->ActiveCue->getNumberOfKeyFrames() > 0)
    {
    index = this->Internal->keyFrameIndex->value()+1;
    }
  emit this->beginUndo("Insert Key Frame");
  this->insertKeyFrame(index);
  if (index ==0 && 
    this->Internal->ActiveCue->getNumberOfKeyFrames() == 1)
    {
    this->Internal->ValueAdaptor->setValueToMin();

    this->insertKeyFrame(index+1);
    this->Internal->ValueAdaptor->setValueToMax();
    
    this->showKeyFrame(0);
    }
  emit this->endUndo();
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::deleteKeyFrameCallback()
{
  if (this->Internal->ActiveCue && 
    this->Internal->ActiveCue->getNumberOfKeyFrames() > 0)
    {
    int index = this->Internal->keyFrameIndex->value();
    this->deleteKeyFrame(index);
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::onKeyFramesModified()
{
  if (this->Internal->ActiveCue)
    {
    int num_keyframes = this->Internal->ActiveCue->getNumberOfKeyFrames();
    if (num_keyframes)
      {
      this->Internal->keyFrameIndex->setMaximum(num_keyframes-1);
      }
    int index = this->Internal->keyFrameIndex->value();
    this->showKeyFrame(index);
    }
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::insertKeyFrame(int index)
{
  emit this->beginUndo("Insert Key Frame");
  pqAnimationManager* mgr = this->Internal->Manager;
  pqAnimationScene* scene = mgr->getActiveScene();
  if (!scene)
    {
    scene = mgr->createActiveScene();
    this->setActiveCue(0); // just to be on safer side,
      // we created a new scene, we certainly can't have
      // an active cue.
    }

  if (!scene)
    {
    qDebug() << "Could not locate scene for the current server.";
    return;
    }

 pqAnimationCue* cue = this->Internal->ActiveCue;

  if (!cue)
    {
    // Need to create new cue for this property.
    pqAnimationPanel::pqInternals::PropertyInfo info =
      this->Internal->propertyName->itemData(
        this->Internal->propertyName->currentIndex()).value<
      pqAnimationPanel::pqInternals::PropertyInfo>();
    if (info.Proxy == this->Internal->ActiveView->getProxy())
      {
      cue = scene->createCue(info.Proxy,
        info.Name.toAscii().data(), info.Index, "CameraManipulator");
      cue->setKeyFrameType("CameraKeyFrame");
      }
    else
      {
      cue = scene->createCue(info.Proxy,
        info.Name.toAscii().data(), info.Index);
      }
    this->setActiveCue(cue);
    }

  // Now the actual add keyframe stuff.
  vtkSMProxy* kf = cue->insertKeyFrame(index);
  if (kf)
    {
    this->showKeyFrame(index);
    if (kf->IsA("vtkSMCameraKeyFrameProxy"))
      {
      this->resetCameraKeyFrameToCurrent();
      }
    else
      {
      this->Internal->ValueAdaptor->setValueToCurrent();
      }
    }

  
  emit this->endUndo();
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::deleteKeyFrame(int index)
{
  pqAnimationManager* mgr = this->Internal->Manager;
  pqAnimationScene* scene = mgr->getActiveScene();
  if (!scene)
    {
    qDebug() << "Could not locate scene for the current server. "
      << "deleteKeyFrame failed.";
    return;
    }

  emit this->beginUndo("Delete Key Frame");
  pqAnimationCue* cue = this->Internal->ActiveCue;
  if (cue)
    {
    cue->deleteKeyFrame(index);
    }
  emit this->endUndo();
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::showKeyFrameCallback(int index)
{
  this->showKeyFrame(index);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::showKeyFrame(int index)
{
  vtkSMProxy* toShowKf = 0;
  if (this->Internal->ActiveCue && index >= 0)
    {
    toShowKf = this->Internal->ActiveCue->getKeyFrame(index);
    }

  if (toShowKf == this->Internal->ActiveKeyFrame)
    {
    // nothing changed.
    return;
    }

  this->Internal->ActiveKeyFrame = toShowKf;

  // clean up old keyframe.
  this->Internal->KeyFrameLinks.removeAllPropertyLinks();
  this->Internal->ValueAdaptor->setAnimationCue(0);
  this->Internal->TimeAdaptor->setAnimationCue(0);
  this->Internal->TimeAdaptor->setAnimationScene(0);
  this->Internal->TypeAdaptor->setKeyFrameProxy(0);
  this->Internal->KeyFrameTimeValidator->setAnimationScene(0);
  if (!toShowKf)
    {
    // No keyframe to show.
    return;
    }

  // So interpolation options only for composite key frames.
  if (toShowKf->IsA("vtkSMCompositeKeyFrameProxy"))
    {
    this->Internal->interpolationType->blockSignals(true);
    this->Internal->interpolationType->clear();
    this->Internal->interpolationType->addItem(
      QIcon(":pqWidgets/Icons/pqRamp16.png"), "Ramp", "Ramp");
    this->Internal->interpolationType->addItem(
      QIcon(":pqWidgets/Icons/pqExponential16.png"), "Exponential", 
      "Exponential");
    this->Internal->interpolationType->addItem(
      QIcon(":pqWidgets/Icons/pqSinusoidal16.png"), "Sinusoid", "Sinusoid");
    this->Internal->interpolationType->addItem(
      QIcon(":pqWidgets/Icons/pqStep16.png"), "Step", "Boolean");
    this->Internal->interpolationType->setCurrentIndex(-1);
    this->Internal->interpolationType->blockSignals(false);
    }

  this->Internal->ValueAdaptor->setAnimationCue(this->Internal->ActiveCue);
  this->Internal->TimeAdaptor->setAnimationCue(this->Internal->ActiveCue);

  // pqKeyFrameTimeValidator ensures that the the keyframes order
  // cannot be changed. This works because when the keyframes
  // or their time changes, the domain for the KeyTime property
  // for all the keyframes is updated by the 
  // vtkSMKeyFrameAnimationCueManipulatorProxy.
  this->Internal->KeyFrameTimeValidator->setAnimationScene(
    this->Internal->ActiveScene);
  this->Internal->KeyFrameTimeValidator->setDomain(
    toShowKf->GetProperty("KeyTime")->GetDomain("range"));

  this->Internal->TimeAdaptor->setAnimationScene(
    this->Internal->ActiveScene);

  // Update and connect the type adaptor
  this->Internal->TypeAdaptor->setKeyFrameProxy(toShowKf);
  this->Internal->KeyFrameLinks.addPropertyLink(
    this->Internal->TypeAdaptor, "currentData",
    SIGNAL(currentTextChanged(const QString&)),
    toShowKf, toShowKf->GetProperty("Type"));

  if (toShowKf->GetXMLName() == QString("CameraKeyFrame"))
    {
    this->Internal->cameraFrame->show();
    this->Internal->KeyFrameLinks.addPropertyLink(
      this->Internal->position0, "value", SIGNAL(valueChanged(double)),
      toShowKf, toShowKf->GetProperty("Position"), 0);
    this->Internal->KeyFrameLinks.addPropertyLink(
      this->Internal->position1, "value", SIGNAL(valueChanged(double)),
      toShowKf, toShowKf->GetProperty("Position"), 1);
    this->Internal->KeyFrameLinks.addPropertyLink(
      this->Internal->position2, "value", SIGNAL(valueChanged(double)),
      toShowKf, toShowKf->GetProperty("Position"), 2);

    this->Internal->KeyFrameLinks.addPropertyLink(
      this->Internal->focalPoint0, "value", SIGNAL(valueChanged(double)),
      toShowKf, toShowKf->GetProperty("FocalPoint"), 0);
    this->Internal->KeyFrameLinks.addPropertyLink(
      this->Internal->focalPoint1, "value", SIGNAL(valueChanged(double)),
      toShowKf, toShowKf->GetProperty("FocalPoint"), 1);
    this->Internal->KeyFrameLinks.addPropertyLink(
      this->Internal->focalPoint2, "value", SIGNAL(valueChanged(double)),
      toShowKf, toShowKf->GetProperty("FocalPoint"), 2);

    this->Internal->KeyFrameLinks.addPropertyLink(
      this->Internal->viewUp0, "value", SIGNAL(valueChanged(double)),
      toShowKf, toShowKf->GetProperty("ViewUp"), 0);
    this->Internal->KeyFrameLinks.addPropertyLink(
      this->Internal->viewUp1, "value", SIGNAL(valueChanged(double)),
      toShowKf, toShowKf->GetProperty("ViewUp"), 1);  
    this->Internal->KeyFrameLinks.addPropertyLink(
      this->Internal->viewUp2, "value", SIGNAL(valueChanged(double)),
      toShowKf, toShowKf->GetProperty("ViewUp"), 2);

    this->Internal->KeyFrameLinks.addPropertyLink(
      this->Internal->viewAngle, "value", SIGNAL(valueChanged(double)),
      toShowKf, toShowKf->GetProperty("ViewAngle"), 0);
    }
  else
    {
    this->Internal->cameraFrame->hide();
    int animated_index = this->Internal->ActiveCue->getAnimatedPropertyIndex();
    if (animated_index == -1)
      {
      this->Internal->KeyFrameLinks.addPropertyLink(
        this->Internal->ValueAdaptor, "values",
        SIGNAL(valueChanged()),
        toShowKf, toShowKf->GetProperty("KeyValues"));
      }
    else
      {
      // Note the index here must be 0. It is the index in
      // the keyframe value property not the index in the
      // animated property.
      this->Internal->KeyFrameLinks.addPropertyLink(
        this->Internal->ValueAdaptor, "value",
        SIGNAL(valueChanged()),
        toShowKf, toShowKf->GetProperty("KeyValues"), 0);
      }
    }
  this->Internal->KeyFrameLinks.addPropertyLink(
    this->Internal->TimeAdaptor, "normalizedTime",
    SIGNAL(timeChanged()),
    toShowKf, toShowKf->GetProperty("KeyTime"));

  this->Internal->keyFrameIndex->setValue(index);
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::setStartTimeByIndex(int index)
{
  if (!this->Internal->ActiveScene)
    {
    return;
    }

  pqTimeKeeper* timekeeper = 
    this->Internal->ActiveScene->getServer()->getTimeKeeper();

  double time = timekeeper->getTimeStepValue(index);
  vtkSMProxy* proxy = this->Internal->ActiveScene->getProxy();
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("ClockTimeRange"), 0, time);
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::setEndTimeByIndex(int index)
{
  if (!this->Internal->ActiveScene)
    {
    return;
    }

  pqTimeKeeper* timekeeper = 
    this->Internal->ActiveScene->getServer()->getTimeKeeper();
  double time = timekeeper->getTimeStepValue(index);
  vtkSMProxy* proxy = this->Internal->ActiveScene->getProxy();
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("ClockTimeRange"), 1, time);
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::setCurrentTimeByIndex(int index)
{
  if (!this->Internal->ActiveScene)
    {
    return;
    }
  pqTimeKeeper* timekeeper = 
    this->Internal->ActiveScene->getServer()->getTimeKeeper();

  double time = timekeeper->getTimeStepValue(index);
  timekeeper->setTime(time);
  // This will trigger onTimeChanged() which should update the two
  // widgets showing current time index.
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::onTimeChanged()
{
  if (!this->Internal->ActiveScene)
    {
    return;
    }


  pqTimeKeeper* timekeeper = 
    this->Internal->ActiveScene->getServer()->getTimeKeeper();
  double current_time = timekeeper->getTime();

  vtkSMProxy* proxy = this->Internal->ActiveScene->getProxy();
  QString playmode = pqSMAdaptor::getEnumerationProperty(
    proxy->GetProperty("PlayMode")).toString();
  if (playmode == "Snap To TimeSteps")
    {
    int index = timekeeper->getTimeStepValueIndex(current_time);

    this->Internal->currentTimeIndex->blockSignals(true);
    this->Internal->currentTimeIndex->setValue(index);
    this->Internal->currentTimeIndex->blockSignals(false);

    if (this->Internal->ToolbarCurrentTimeIndexWidget)
      {
      this->Internal->ToolbarCurrentTimeIndexWidget->blockSignals(true);
      this->Internal->ToolbarCurrentTimeIndexWidget->setValue(index);
      this->Internal->ToolbarCurrentTimeIndexWidget->blockSignals(false);
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::updatePanelCurrentTime(const QString& str)
{
  if (this->Internal->currentTime->text() != str)
    {
    this->Internal->currentTime->setText(str);
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::updateToolbarCurrentTime(const QString& str)
{
  if (this->Internal->ToolbarCurrentTimeWidget->text() != str)
    {
    this->Internal->ToolbarCurrentTimeWidget->setText(str);
    }
}

//-----------------------------------------------------------------------------
void pqAnimationPanel::setCurrentTimeToolbar(QToolBar* toolbar)
{
  if (!toolbar)
    {
    return;
    }

  QLabel* label = new QLabel(toolbar);
  label->setText("Time: ");

  QLineEdit* timeedit = new QLineEdit(toolbar);
  timeedit->setObjectName("CurrentTime");
  timeedit->setValidator(new QDoubleValidator(toolbar));
  this->Internal->ToolbarCurrentTimeWidget = timeedit;

  QObject::connect(this->Internal->currentTime, SIGNAL(textChanged(const QString&)),
    this, SLOT(updateToolbarCurrentTime(const QString&)));
  QObject::connect(timeedit, SIGNAL(textChanged(const QString&)),
    this, SLOT(updatePanelCurrentTime(const QString&)));

  QSpinBox* sbtimeedit = new QSpinBox(toolbar);
  sbtimeedit->setObjectName("CurrentTimeIndex");
  this->Internal->ToolbarCurrentTimeIndexWidget = sbtimeedit;

  QObject::connect(
    this->Internal->ToolbarCurrentTimeIndexWidget, SIGNAL(valueChanged(int)),
    this, SLOT(setCurrentTimeByIndex(int)));

  toolbar->addWidget(label);
  toolbar->addWidget(timeedit);
  toolbar->addWidget(sbtimeedit);
}
