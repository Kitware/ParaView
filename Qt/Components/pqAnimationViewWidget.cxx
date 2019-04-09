/*=========================================================================

   Program: ParaView
   Module:    pqAnimationViewWidget.cxx

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

=========================================================================*/

#include "pqAnimationViewWidget.h"
#include "ui_pqPythonAnimationCue.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QSignalMapper>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtDebug>

#include "pqActiveObjects.h"
#include "pqAnimatablePropertiesComboBox.h"
#include "pqAnimatableProxyComboBox.h"
#include "pqAnimationCue.h"
#include "pqAnimationKeyFrame.h"
#include "pqAnimationModel.h"
#include "pqAnimationScene.h"
#include "pqAnimationTimeWidget.h"
#include "pqAnimationTrack.h"
#include "pqAnimationWidget.h"
#include "pqApplicationCore.h"
#include "pqComboBoxDomain.h"
#include "pqCoreUtilities.h"
#include "pqKeyFrameEditor.h"
#include "pqOrbitCreatorDialog.h"
#include "pqPipelineTimeKeyFrameEditor.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSetName.h"
#include "pqSignalAdaptors.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"

#include "vtkCamera.h"
#include "vtkPVConfig.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMTrace.h"

#include <cassert>

#if VTK_MODULE_ENABLE_ParaView_pqPython
#include "pqPythonSyntaxHighlighter.h"
#endif

//-----------------------------------------------------------------------------
class pqAnimationViewWidget::pqInternal
{
public:
  pqInternal()
    : AnimationWidget(NULL)
    , AnimationTimeWidget(NULL)
    , PlayMode(NULL)
    , StartTime(NULL)
    , StartTimeLabel(NULL)
    , EndTime(NULL)
    , EndTimeLabel(NULL)
    , DurationLabel(NULL)
    , Duration(NULL)
    , CreateSource(NULL)
    , CreateProperty(NULL)
    , LockEndTime(NULL)
    , LockStartTime(NULL)
    , SelectedCueProxy(NULL)
    , SelectedDataProxy(NULL)
  {
  }

  ~pqInternal() {}

  QPointer<pqAnimationScene> Scene;
  pqAnimationWidget* AnimationWidget;
  pqAnimationTimeWidget* AnimationTimeWidget;
  QSignalMapper KeyFramesChanged;
  typedef QMap<QPointer<pqAnimationCue>, pqAnimationTrack*> TrackMapType;
  TrackMapType TrackMap;
  QPointer<QDialog> Editor;
  QComboBox* PlayMode;
  QLineEdit* StartTime;
  QLabel* StartTimeLabel;
  QLineEdit* EndTime;
  QLabel* EndTimeLabel;
  QLabel* DurationLabel;
  QLineEdit* Duration;
  pqPropertyLinks Links;
  pqPropertyLinks DurationLink;
  pqAnimatableProxyComboBox* CreateSource;
  pqAnimatablePropertiesComboBox* CreateProperty;
  QToolButton* LockEndTime;
  QToolButton* LockStartTime;
  vtkSMProxy* SelectedCueProxy;
  vtkSMProxy* SelectedDataProxy;

  pqAnimationTrack* findTrack(pqAnimationCue* cue)
  {
    TrackMapType::iterator iter;
    iter = this->TrackMap.find(cue);
    if (iter != this->TrackMap.end())
    {
      return iter.value();
    }
    return NULL;
  }
  pqAnimationCue* findCue(pqAnimationTrack* track)
  {
    TrackMapType::iterator iter;
    for (iter = this->TrackMap.begin(); iter != this->TrackMap.end(); ++iter)
    {
      if (iter.value() == track)
      {
        return iter.key();
      }
    }
    return NULL;
  }
  QString cueName(pqAnimationCue* cue)
  {
    if (this->cameraCue(cue))
    {
      return "Camera";
    }
    else if (this->pythonCue(cue))
    {
      return "Python";
    }
    else
    {
      pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();

      vtkSMProxy* pxy = cue->getAnimatedProxy();
      vtkSMProperty* pty = cue->getAnimatedProperty();
      QString p = pty->GetXMLLabel();
      if (pqSMAdaptor::getPropertyType(pty) == pqSMAdaptor::MULTIPLE_ELEMENTS)
      {
        p = QString("%1 (%2)").arg(p).arg(cue->getAnimatedPropertyIndex());
      }

      if (pqProxy* animation_pqproxy = model->findItem<pqProxy*>(pxy))
      {
        return QString("%1 - %2").arg(animation_pqproxy->getSMName()).arg(p);
      }

      // could be a helper proxy
      QString helper_key;
      if (pqProxy* pqproxy = pqProxy::findProxyWithHelper(pxy, helper_key))
      {
        vtkSMProperty* prop = pqproxy->getProxy()->GetProperty(helper_key.toLocal8Bit().data());
        if (prop)
        {
          return QString("%1 - %2 - %3").arg(pqproxy->getSMName()).arg(prop->GetXMLLabel()).arg(p);
        }
        return QString("%1 - %2").arg(pqproxy->getSMName()).arg(p);
      }
    }
    return QString("<unrecognized>");
  }
  // returns if this is a cue for animating a camera
  bool cameraCue(pqAnimationCue* cue)
  {
    if (cue && QString("CameraAnimationCue") == cue->getProxy()->GetXMLName())
    {
      return true;
    }
    return false;
  }

  /// returns true if the cue is a python cue.
  bool pythonCue(pqAnimationCue* cue)
  {
    if (QString("PythonAnimationCue") == cue->getProxy()->GetXMLName())
    {
      return true;
    }
    return false;
  }

  int numberOfTicks()
  {
    vtkSMProxy* pxy = this->Scene->getProxy();
    QString mode = pqSMAdaptor::getEnumerationProperty(pxy->GetProperty("PlayMode")).toString();

    int num = 0;

    if (mode == "Sequence")
    {
      num = pqSMAdaptor::getElementProperty(pxy->GetProperty("NumberOfFrames")).toInt();
    }
    else if (mode == "Snap To TimeSteps")
    {
      num = this->Scene->getTimeSteps().size();
    }
    return num;
  }

  QList<double> ticks()
  {
    vtkSMProxy* pxy = this->Scene->getProxy();
    QString mode = pqSMAdaptor::getEnumerationProperty(pxy->GetProperty("PlayMode")).toString();
    if (mode == "Snap To TimeSteps")
    {
      return this->Scene->getTimeSteps();
    }
    return QList<double>();
  }
};

//-----------------------------------------------------------------------------
pqAnimationViewWidget::pqAnimationViewWidget(QWidget* _parent)
  : QWidget(_parent)
{
  this->Internal = new pqAnimationViewWidget::pqInternal();
  QVBoxLayout* vboxlayout = new QVBoxLayout(this);
  vboxlayout->setMargin(2);
  vboxlayout->setSpacing(2);

  QHBoxLayout* hboxlayout = new QHBoxLayout;
  vboxlayout->addLayout(hboxlayout);
  hboxlayout->setMargin(0);
  hboxlayout->setSpacing(2);

  hboxlayout->addWidget(new QLabel("Mode:", this));
  this->Internal->PlayMode = new QComboBox(this) << pqSetName("PlayMode");
  this->Internal->PlayMode->addItem("Snap to Timesteps");
  hboxlayout->addWidget(this->Internal->PlayMode);
  this->Internal->AnimationTimeWidget = new pqAnimationTimeWidget(this);
  this->Internal->AnimationTimeWidget->setPlayModeReadOnly(true);
  hboxlayout->addWidget(this->Internal->AnimationTimeWidget);

  this->Internal->StartTimeLabel = new QLabel("Start Time:", this);
  hboxlayout->addWidget(this->Internal->StartTimeLabel);
  this->Internal->StartTime = new QLineEdit(this) << pqSetName("StartTime");
  this->Internal->StartTime->setMinimumWidth(30);
  this->Internal->StartTime->setValidator(new QDoubleValidator(this->Internal->StartTime));
  hboxlayout->addWidget(this->Internal->StartTime);
  this->Internal->LockStartTime = new QToolButton(this) << pqSetName("LockStartTime");
  this->Internal->LockStartTime->setIcon(QIcon(":pqWidgets/Icons/pqLock24.png"));
  this->Internal->LockStartTime->setToolTip(
    "<html>Lock the start time to keep ParaView from changing it "
    "as available data times change</html>");
  this->Internal->LockStartTime->setStatusTip(
    "<html>Lock the start time to keep ParaView from changing it "
    "as available data times change</html>");
  this->Internal->LockStartTime->setCheckable(true);
  hboxlayout->addWidget(this->Internal->LockStartTime);
  this->Internal->EndTimeLabel = new QLabel("End Time:", this);
  hboxlayout->addWidget(this->Internal->EndTimeLabel);
  this->Internal->EndTime = new QLineEdit(this) << pqSetName("EndTime");
  this->Internal->EndTime->setMinimumWidth(30);
  this->Internal->EndTime->setValidator(new QDoubleValidator(this->Internal->EndTime));
  hboxlayout->addWidget(this->Internal->EndTime);
  this->Internal->LockEndTime = new QToolButton(this) << pqSetName("LockEndTime");
  this->Internal->LockEndTime->setIcon(QIcon(":pqWidgets/Icons/pqLock24.png"));
  this->Internal->LockEndTime->setToolTip(
    "<html>Lock the end time to keep ParaView from changing it"
    " as available data times change</html>");
  this->Internal->LockEndTime->setStatusTip(
    "<html>Lock the end time to keep ParaView from changing it"
    " as available data times change</html>");
  this->Internal->LockEndTime->setCheckable(true);
  hboxlayout->addWidget(this->Internal->LockEndTime);
  this->Internal->DurationLabel = new QLabel(this);
  hboxlayout->addWidget(this->Internal->DurationLabel);
  this->Internal->Duration = new QLineEdit(this) << pqSetName("Duration");
  this->Internal->Duration->setMinimumWidth(30);
  this->Internal->Duration->setValidator(
    new QIntValidator(1, static_cast<int>(~0u >> 1), this->Internal->Duration));
  hboxlayout->addWidget(this->Internal->Duration);
  hboxlayout->addStretch();

  this->Internal->AnimationWidget = new pqAnimationWidget(this) << pqSetName("pqAnimationWidget");
  this->Internal->AnimationWidget->animationModel()->setInteractive(true);
  this->Internal->AnimationWidget->animationModel()->setTimePrecision(
    vtkPVGeneralSettings::GetInstance()->GetAnimationTimePrecision());
  this->Internal->AnimationWidget->animationModel()->setTimeNotation(
    vtkPVGeneralSettings::GetInstance()->GetAnimationTimeNotation());

  pqCoreUtilities::connect(vtkPVGeneralSettings::GetInstance(), vtkCommand::ModifiedEvent, this,
    SLOT(generalSettingsChanged()));

  QWidget* w = this->Internal->AnimationWidget->createDeleteWidget();

  this->Internal->CreateSource = new pqAnimatableProxyComboBox(w) << pqSetName("ProxyCombo");
#if VTK_MODULE_ENABLE_ParaView_pqPython
  this->Internal->CreateSource->addProxy(0, "Python", NULL);
#endif
  this->Internal->CreateProperty = new pqAnimatablePropertiesComboBox(w)
    << pqSetName("PropertyCombo");
  this->Internal->CreateSource->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  this->Internal->CreateProperty->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  QHBoxLayout* l = new QHBoxLayout(w);
  l->setMargin(0);
  l->addSpacing(6);
  l->addWidget(this->Internal->CreateSource);
  l->addWidget(this->Internal->CreateProperty);
  l->addStretch();

  QObject::connect(&this->Internal->KeyFramesChanged, SIGNAL(mapped(QObject*)), this,
    SLOT(keyFramesChanged(QObject*)));
  QObject::connect(this->Internal->AnimationWidget, SIGNAL(trackSelected(pqAnimationTrack*)), this,
    SLOT(trackSelected(pqAnimationTrack*)));
  QObject::connect(this->Internal->AnimationWidget, SIGNAL(deleteTrackClicked(pqAnimationTrack*)),
    this, SLOT(deleteTrack(pqAnimationTrack*)));
  QObject::connect(this->Internal->AnimationWidget, SIGNAL(enableTrackClicked(pqAnimationTrack*)),
    this, SLOT(toggleTrackEnabled(pqAnimationTrack*)));
  QObject::connect(
    this->Internal->AnimationWidget, SIGNAL(createTrackClicked()), this, SLOT(createTrack()));

  QObject::connect(this->Internal->AnimationWidget->animationModel(),
    SIGNAL(currentTimeSet(double)), this, SLOT(setCurrentTime(double)));
  QObject::connect(this->Internal->AnimationWidget->animationModel(),
    SIGNAL(keyFrameTimeChanged(pqAnimationTrack*, pqAnimationKeyFrame*, int, double)), this,
    SLOT(setKeyFrameTime(pqAnimationTrack*, pqAnimationKeyFrame*, int, double)));

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(setActiveView(pqView*)));

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(setCurrentSelection(pqPipelineSource*)));

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)), this,
    SLOT(onSceneCuesChanged()));

  QObject::connect(this->Internal->CreateSource, SIGNAL(currentProxyChanged(vtkSMProxy*)), this,
    SLOT(setCurrentProxy(vtkSMProxy*)));

  vboxlayout->addWidget(this->Internal->AnimationWidget);
}

//-----------------------------------------------------------------------------
pqAnimationViewWidget::~pqAnimationViewWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::setScene(pqAnimationScene* scene)
{
  if (this->Internal->Scene)
  {
    this->Internal->Links.removeAllPropertyLinks();
    QObject::disconnect(this->Internal->Scene, 0, this, 0);

    pqComboBoxDomain* d0 = this->Internal->PlayMode->findChild<pqComboBoxDomain*>("ComboBoxDomain");
    if (d0)
    {
      delete d0;
    }
    pqSignalAdaptorComboBox* adaptor =
      this->Internal->PlayMode->findChild<pqSignalAdaptorComboBox*>("ComboBoxAdaptor");
    if (adaptor)
    {
      delete adaptor;
    }
  }
  this->Internal->Scene = scene;
  if (this->Internal->Scene)
  {
    pqComboBoxDomain* d0 =
      new pqComboBoxDomain(this->Internal->PlayMode, scene->getProxy()->GetProperty("PlayMode"));
    d0->setObjectName("ComboBoxDomain");
    pqSignalAdaptorComboBox* adaptor = new pqSignalAdaptorComboBox(this->Internal->PlayMode);
    adaptor->setObjectName("ComboBoxAdaptor");
    this->Internal->Links.addTraceablePropertyLink(adaptor, "currentText",
      SIGNAL(currentTextChanged(const QString&)), scene->getProxy(),
      scene->getProxy()->GetProperty("PlayMode"));

    // connect time
    this->Internal->Links.addTraceablePropertyLink(this->Internal->AnimationTimeWidget, "timeValue",
      SIGNAL(timeValueChanged()), scene->getProxy(),
      scene->getProxy()->GetProperty("AnimationTime"));
    this->Internal->AnimationTimeWidget->setAnimationScene(scene->getProxy());
    // connect start time
    this->Internal->Links.addTraceablePropertyLink(this->Internal->StartTime, "text",
      SIGNAL(editingFinished()), scene->getProxy(), scene->getProxy()->GetProperty("StartTime"));
    // connect end time
    this->Internal->Links.addTraceablePropertyLink(this->Internal->EndTime, "text",
      SIGNAL(editingFinished()), scene->getProxy(), scene->getProxy()->GetProperty("EndTime"));
    // connect lock start time.
    this->Internal->Links.addTraceablePropertyLink(this->Internal->LockStartTime, "checked",
      SIGNAL(toggled(bool)), scene->getProxy(), scene->getProxy()->GetProperty("LockStartTime"));
    this->Internal->Links.addTraceablePropertyLink(this->Internal->LockEndTime, "checked",
      SIGNAL(toggled(bool)), scene->getProxy(), scene->getProxy()->GetProperty("LockEndTime"));

    QObject::connect(scene, SIGNAL(cuesChanged()), this, SLOT(onSceneCuesChanged()));
    QObject::connect(scene, SIGNAL(clockTimeRangesChanged()), this, SLOT(updateSceneTimeRange()));
    QObject::connect(scene, SIGNAL(timeStepsChanged()), this, SLOT(updateTicks()));
    QObject::connect(scene, SIGNAL(frameCountChanged()), this, SLOT(updateTicks()));
    QObject::connect(scene, SIGNAL(animationTime(double)), this, SLOT(updateSceneTime()));
    QObject::connect(scene, SIGNAL(playModeChanged()), this, SLOT(updatePlayMode()));
    QObject::connect(scene, SIGNAL(playModeChanged()), this, SLOT(updateTicks()));
    QObject::connect(scene, SIGNAL(playModeChanged()), this, SLOT(updateSceneTime()));
    QObject::connect(scene, SIGNAL(timeLabelChanged()), this, SLOT(onTimeLabelChanged()));

    this->updateSceneTimeRange();
    this->updateSceneTime();
    this->updatePlayMode();
    this->updateTicks();
    this->onTimeLabelChanged();
  }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::onSceneCuesChanged()
{
  if (!this->Internal->Scene)
  {
    // No scene, so do nothing
    return;
  }

  QSet<pqAnimationCue*> cues = this->Internal->Scene->getCues();
  pqAnimationModel* animModel = this->Internal->AnimationWidget->animationModel();

  pqInternal::TrackMapType oldCues = this->Internal->TrackMap;
  pqInternal::TrackMapType::iterator iter;

  // add new tracks
  foreach (pqAnimationCue* cue, cues)
  {
    if (cue == NULL)
    {
      continue;
    }
    QString completeName = this->Internal->cueName(cue);

    iter = this->Internal->TrackMap.find(cue);

    if (iter == this->Internal->TrackMap.end())
    {
      pqAnimationTrack* track = animModel->addTrack();
      if (completeName.startsWith("TimeKeeper"))
      {
        track->setDeletable(false);
      }
      this->Internal->TrackMap.insert(cue, track);
      track->setProperty(completeName);
      this->Internal->KeyFramesChanged.setMapping(cue, cue);
      QObject::connect(
        cue, SIGNAL(keyframesModified()), &this->Internal->KeyFramesChanged, SLOT(map()));
      QObject::connect(cue, SIGNAL(enabled(bool)), track, SLOT(setEnabled(bool)));
      track->setEnabled(cue->isEnabled());

      // this ensures that the already present keyframes are loaded currently
      // (which happens when loading state files).
      this->keyFramesChanged(cue);
    }
    else
    {
      oldCues.remove(cue);
    }
  }

  // remove old tracks
  for (iter = oldCues.begin(); iter != oldCues.end(); iter++)
  {
    animModel->removeTrack(iter.value());
    this->Internal->TrackMap.remove(iter.key());
    if (iter.key())
    {
      QObject::disconnect(
        iter.key(), SIGNAL(keyframesModified()), &this->Internal->KeyFramesChanged, SLOT(map()));
    }
  }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::keyFramesChanged(QObject* cueObject)
{
  pqAnimationCue* cue = qobject_cast<pqAnimationCue*>(cueObject);
  pqAnimationTrack* track = this->Internal->findTrack(cue);

  QList<vtkSMProxy*> keyFrames = cue->getKeyFrames();

  bool camera = this->Internal->cameraCue(cue);

  // clean out old ones
  while (track->count())
  {
    track->removeKeyFrame(track->keyFrame(0));
  }

  for (int j = 0; j < keyFrames.count() - 1; j++)
  {
    QIcon icon;
    QVariant startValue;
    QVariant endValue;

    double startTime =
      pqSMAdaptor::getElementProperty(keyFrames[j]->GetProperty("KeyTime")).toDouble();
    double endTime =
      pqSMAdaptor::getElementProperty(keyFrames[j + 1]->GetProperty("KeyTime")).toDouble();

    if (!camera)
    {
      QVariant interpolation =
        pqSMAdaptor::getEnumerationProperty(keyFrames[j]->GetProperty("Type"));
      if (interpolation == "Boolean")
        interpolation = "Step";
      else if (interpolation == "Sinusoid")
        interpolation = "Sinusoidal";
      QString iconstr =
        QString(":pqWidgets/Icons/pq%1%2.png").arg(interpolation.toString()).arg(16);
      icon = QIcon(iconstr);

      startValue = pqSMAdaptor::getElementProperty(keyFrames[j]->GetProperty("KeyValues"));
      endValue = pqSMAdaptor::getElementProperty(keyFrames[j + 1]->GetProperty("KeyValues"));
    }

    pqAnimationKeyFrame* newFrame = track->addKeyFrame();
    newFrame->setNormalizedStartTime(startTime);
    newFrame->setNormalizedEndTime(endTime);
    newFrame->setStartValue(startValue);
    newFrame->setEndValue(endValue);
    newFrame->setIcon(QIcon(icon));
  }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::updateSceneTimeRange()
{
  pqAnimationModel* animModel = this->Internal->AnimationWidget->animationModel();
  QPair<double, double> timeRange = this->Internal->Scene->getClockTimeRange();
  animModel->setStartTime(timeRange.first);
  animModel->setEndTime(timeRange.second);
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::updateSceneTime()
{
  double time = this->Internal->Scene->getAnimationTime();

  pqAnimationModel* animModel = this->Internal->AnimationWidget->animationModel();
  animModel->setCurrentTime(time);
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::setCurrentTime(double t)
{
  vtkSMProxy* animationScene = this->Internal->Scene->getProxy();
  {
    // Use another scope to prevent modifications to the TimeKeeper from
    // being traced.
    SM_SCOPED_TRACE(PropertiesModified).arg("proxy", animationScene);
    vtkSMPropertyHelper(animationScene, "AnimationTime").Set(t);
  }
  animationScene->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::setKeyFrameTime(
  pqAnimationTrack* track, pqAnimationKeyFrame* kf, int edge, double time)
{
  pqAnimationCue* cue = this->Internal->findCue(track);
  if (!cue)
  {
    return;
  }
  QList<vtkSMProxy*> keyFrames = cue->getKeyFrames();
  int i = 0;
  for (i = 0; i < track->count(); i++)
  {
    if (track->keyFrame(i) == kf)
    {
      break;
    }
  }
  if (edge)
  {
    i++;
  }

  if (i < keyFrames.size())
  {
    QPair<double, double> timeRange = this->Internal->Scene->getClockTimeRange();
    double normTime = (time - timeRange.first) / (timeRange.second - timeRange.first);
    pqSMAdaptor::setElementProperty(keyFrames[i]->GetProperty("KeyTime"), normTime);
    keyFrames[i]->UpdateVTKObjects();
  }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::trackSelected(pqAnimationTrack* track)
{
  pqAnimationCue* cue = this->Internal->findCue(track);
  if (!cue)
  {
    return;
  }

  if (this->Internal->Editor)
  {
    this->Internal->Editor->raise();
    return;
  }

  if (track->property().toString().startsWith("TimeKeeper"))
  {
    this->Internal->Editor = new pqPipelineTimeKeyFrameEditor(this->Internal->Scene, cue, NULL);
    this->Internal->Editor->resize(600, 400);
  }
  else if (this->Internal->pythonCue(cue))
  {
    QDialog dialog(this);
    Ui::PythonAnimationCue ui;
    ui.setupUi(&dialog);
#if VTK_MODULE_ENABLE_ParaView_pqPython
    new pqPythonSyntaxHighlighter(ui.script, ui.script);
#endif
    ui.script->setPlainText(vtkSMPropertyHelper(cue->getProxy(), "Script").GetAsString());
    if (dialog.exec() == QDialog::Accepted)
    {
      vtkSMPropertyHelper(cue->getProxy(), "Script")
        .Set(ui.script->toPlainText().toLocal8Bit().data());
      cue->getProxy()->UpdateVTKObjects();
    }
    return;
  }
  else
  {
    int mode = -1;
    if (cue->getProxy()->GetProperty("Mode"))
    {
      mode = vtkSMPropertyHelper(cue->getProxy(), "Mode").GetAsInt();
    }

    this->Internal->Editor = new QDialog;
    QVBoxLayout* l = new QVBoxLayout(this->Internal->Editor);
    QDialogButtonBox* buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // FOLLOW_DATA mode
    if (mode == 2)
    {
      // show a combo-box allowing the user to select the data source to follow
      QFormLayout* layout_ = new QFormLayout;
      pqAnimatableProxyComboBox* comboBox = new pqAnimatableProxyComboBox(this->Internal->Editor);
      this->Internal->SelectedCueProxy = cue->getProxy();
      this->Internal->SelectedDataProxy =
        vtkSMPropertyHelper(cue->getProxy(), "DataSource").GetAsProxy();
      comboBox->setCurrentIndex(comboBox->findProxy(this->Internal->SelectedDataProxy));
      connect(comboBox, SIGNAL(currentProxyChanged(vtkSMProxy*)), this,
        SLOT(selectedDataProxyChanged(vtkSMProxy*)));
      connect(
        this->Internal->Editor, SIGNAL(accepted()), this, SLOT(changeDataProxyDialogAccepted()));
      layout_->addRow("Data Source to Follow:", comboBox);
      l->addLayout(layout_);
      this->Internal->Editor->setWindowTitle(tr("Select Data Source"));
    }
    else
    {
      pqKeyFrameEditor* editor = new pqKeyFrameEditor(this->Internal->Scene, cue,
        QString("Editing ") + this->Internal->cueName(cue), this->Internal->Editor);

      l->addWidget(editor);

      connect(this->Internal->Editor, SIGNAL(accepted()), editor, SLOT(writeKeyFrameData()));
      this->Internal->Editor->setWindowTitle(tr("Animation Keyframes"));
      this->Internal->Editor->resize(600, 400);
    }

    connect(buttons, SIGNAL(accepted()), this->Internal->Editor, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this->Internal->Editor, SLOT(reject()));
    l->addWidget(buttons);
  }

  this->Internal->Editor->setAttribute(Qt::WA_QuitOnClose, false);
  this->Internal->Editor->setAttribute(Qt::WA_DeleteOnClose);

  this->Internal->Editor->show();
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::updatePlayMode()
{
  pqAnimationModel* animModel = this->Internal->AnimationWidget->animationModel();
  vtkSMProxy* pxy = this->Internal->Scene->getProxy();

  QString mode = pqSMAdaptor::getEnumerationProperty(pxy->GetProperty("PlayMode")).toString();

  this->Internal->DurationLink.removeAllPropertyLinks();

  this->Internal->AnimationTimeWidget->setPlayMode(mode);

  if (mode == "Real Time")
  {
    animModel->setMode(pqAnimationModel::Real);

    this->Internal->Duration->setVisible(true);
    this->Internal->DurationLabel->setVisible(true);
    this->Internal->StartTime->setVisible(true);
    this->Internal->StartTimeLabel->setVisible(true);
    this->Internal->EndTime->setVisible(true);
    this->Internal->EndTimeLabel->setVisible(true);
    this->Internal->LockStartTime->setVisible(true);
    this->Internal->LockEndTime->setVisible(true);

    this->Internal->StartTime->setEnabled(true);
    this->Internal->EndTime->setEnabled(true);
    this->Internal->AnimationTimeWidget->setEnabled(true);
    this->Internal->Duration->setEnabled(true);
    this->Internal->DurationLabel->setEnabled(true);
    this->Internal->DurationLabel->setText("Duration (s):");
    this->Internal->DurationLink.addTraceablePropertyLink(this->Internal->Duration, "text",
      SIGNAL(editingFinished()), this->Internal->Scene->getProxy(),
      this->Internal->Scene->getProxy()->GetProperty("Duration"));
  }
  else if (mode == "Sequence")
  {
    animModel->setMode(pqAnimationModel::Sequence);

    this->Internal->Duration->setVisible(true);
    this->Internal->DurationLabel->setVisible(true);
    this->Internal->StartTime->setVisible(true);
    this->Internal->StartTimeLabel->setVisible(true);
    this->Internal->EndTime->setVisible(true);
    this->Internal->EndTimeLabel->setVisible(true);
    this->Internal->LockStartTime->setVisible(true);
    this->Internal->LockEndTime->setVisible(true);

    this->Internal->StartTime->setEnabled(true);
    this->Internal->EndTime->setEnabled(true);
    this->Internal->Duration->setEnabled(true);
    this->Internal->DurationLabel->setEnabled(true);
    this->Internal->DurationLabel->setText("No. Frames:");
    this->Internal->DurationLink.addTraceablePropertyLink(this->Internal->Duration, "text",
      SIGNAL(editingFinished()), this->Internal->Scene->getProxy(),
      this->Internal->Scene->getProxy()->GetProperty("NumberOfFrames"));
  }
  else if (mode == "Snap To TimeSteps")
  {
    animModel->setMode(pqAnimationModel::Custom);

    this->Internal->Duration->setVisible(false);
    this->Internal->DurationLabel->setVisible(false);
    this->Internal->StartTime->setVisible(false);
    this->Internal->StartTimeLabel->setVisible(false);
    this->Internal->EndTime->setVisible(false);
    this->Internal->EndTimeLabel->setVisible(false);
    this->Internal->LockStartTime->setVisible(false);
    this->Internal->LockEndTime->setVisible(false);

    this->Internal->Duration->setEnabled(false);
    this->Internal->DurationLabel->setEnabled(false);
    this->Internal->StartTime->setEnabled(false);
    this->Internal->EndTime->setEnabled(false);
  }
  else
  {
    qWarning("Unrecognized play mode");
  }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::updateTicks()
{
  pqAnimationModel* animModel = this->Internal->AnimationWidget->animationModel();
  if (animModel->mode() == pqAnimationModel::Custom)
  {
    QList<double> ticks = this->Internal->ticks();
    double* dticks = new double[ticks.size() + 1];
    for (int cc = 0; cc < ticks.size(); cc++)
    {
      dticks[cc] = ticks[cc];
    }
    animModel->setTickMarks(ticks.size(), dticks);
    delete[] dticks;
  }
  else
  {
    animModel->setTicks(this->Internal->numberOfTicks());
  }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::toggleTrackEnabled(pqAnimationTrack* track)
{
  pqAnimationCue* cue = this->Internal->findCue(track);
  if (!cue)
  {
    return;
  }
  BEGIN_UNDO_SET("Toggle Animation Track");
  cue->setEnabled(!track->isEnabled());
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::deleteTrack(pqAnimationTrack* track)
{
  pqAnimationCue* cue = this->Internal->findCue(track);
  if (!cue)
  {
    return;
  }
  BEGIN_UNDO_SET("Remove Animation Track");
  this->Internal->Scene->removeCue(cue);
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::setActiveView(pqView* view)
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);
  this->Internal->CreateSource->removeProxy("Camera");
  if (rview && this->Internal->CreateSource->findText("Camera") == -1)
  {
    this->Internal->CreateSource->addProxy(0, "Camera", rview->getProxy());
  }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::setCurrentSelection(pqPipelineSource* pxy)
{
  if (pxy)
  {
    int idx = this->Internal->CreateSource->findProxy(pxy->getProxy());
    if (idx != -1)
    {
      this->Internal->CreateSource->setCurrentIndex(idx);
    }
  }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::setCurrentProxy(vtkSMProxy* pxy)
{
  if (vtkSMRenderViewProxy::SafeDownCast(pxy))
  {
    this->Internal->CreateProperty->setSourceWithoutProperties(pxy);
    // add camera animation modes as properties for creating the camera
    // animation track.
    this->Internal->CreateProperty->addSMProperty("Orbit", "orbit", 0);
    this->Internal->CreateProperty->addSMProperty("Follow Path", "path", 0);
    this->Internal->CreateProperty->addSMProperty("Follow Data", "data", 0);
    this->Internal->CreateProperty->addSMProperty("Interpolate camera locations", "camera", 0);
  }
  else
  {
    this->Internal->CreateProperty->setSource(pxy);
  }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::createTrack()
{
  vtkSMRenderViewProxy* ren =
    vtkSMRenderViewProxy::SafeDownCast(this->Internal->CreateSource->getCurrentProxy());
  // Need to create new cue for this property.
  vtkSMProxy* curProxy = this->Internal->CreateProperty->getCurrentProxy();
  QString pname = this->Internal->CreateProperty->getCurrentPropertyName();
  int pindex = this->Internal->CreateProperty->getCurrentIndex();

  // used for camera tracks.
  QString mode = this->Internal->CreateProperty->getCurrentPropertyName();

  if (ren)
  {
    curProxy = ren;
    pname = QString();
    pindex = 0;
  }

  if (!curProxy)
  {
// curProxy == NULL is only used for "Python" track for now. Of course,
// we only support that when python is enabled.
// we allow creating as many python tracks as needed, hence we don't check
// if there exists a track already (which is the case with others).
#if VTK_MODULE_ENABLE_ParaView_pqPython
    this->createPythonTrack();
#endif
    return;
  }

  // check that we don't already have one
  foreach (pqAnimationCue* cue, this->Internal->TrackMap.keys())
  {
    if (cue->getAnimatedProxy() == NULL)
    {
      continue; // skip Python tracks.
    }
    if (cue->getAnimatedProxy() == curProxy &&
      cue->getAnimatedProxy()->GetPropertyName(cue->getAnimatedProperty()) == pname &&
      cue->getAnimatedPropertyIndex() == pindex)
    {
      return;
    }
  }

  pqOrbitCreatorDialog creator(this);

  // if mode=="orbit" show up a dialog allowing the user to customize the
  // orbit.
  if (ren && mode == "orbit")
  {
    creator.setNormal(ren->GetActiveCamera()->GetViewUp());
    creator.setOrigin(ren->GetActiveCamera()->GetPosition());
    if (creator.exec() != QDialog::Accepted)
    {
      return;
    }
  }

  BEGIN_UNDO_SET("Add Animation Track");

  // This will create the cue and initialize it with default keyframes.
  pqAnimationCue* cue = this->Internal->Scene->createCue(curProxy, pname.toLocal8Bit().data(),
    pindex, ren ? "CameraAnimationCue" : "KeyFrameAnimationCue");

  SM_SCOPED_TRACE(CreateAnimationTrack).arg("cue", cue->getProxy());

  if (ren)
  {
    if (mode == "path" || mode == "orbit")
    {
      // Setup default animation to revolve around the selected objects (if any)
      // in a plane normal to the current view-up vector.
      pqSMAdaptor::setElementProperty(
        cue->getProxy()->GetProperty("Mode"), 1); // PATH-based animation.
    }
    else if (mode == "data")
    {
      pqSMAdaptor::setElementProperty(
        cue->getProxy()->GetProperty("Mode"), 2); // DATA-based animation.

      // set the data source for the follow-data animation
      pqPipelineSource* source = pqActiveObjects::instance().activeSource();
      if (source)
      {
        pqSMAdaptor::setProxyProperty(
          cue->getProxy()->GetProperty("DataSource"), source->getProxy());
      }
    }
    else
    {
      pqSMAdaptor::setElementProperty(
        cue->getProxy()->GetProperty("Mode"), 0); // non-PATH-based animation.
    }
    cue->getProxy()->UpdateVTKObjects();

    if (mode == "orbit")
    {
      // update key frame parameters based on the orbit points.
      vtkSMProxy* kf = cue->getKeyFrame(0);
      pqSMAdaptor::setMultipleElementProperty(
        kf->GetProperty("PositionPathPoints"), creator.orbitPoints(7));
      pqSMAdaptor::setMultipleElementProperty(kf->GetProperty("FocalPathPoints"), creator.center());
      pqSMAdaptor::setElementProperty(kf->GetProperty("ClosedPositionPath"), 1);
      kf->UpdateVTKObjects();
    }
  }

  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::createPythonTrack()
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  BEGIN_UNDO_SET("Add Animation Track");

  pqAnimationCue* cue = this->Internal->Scene->createCue("PythonAnimationCue");
  assert(cue != NULL);
  (void)cue;
  END_UNDO_SET();
#else
  qCritical() << "Python support not enabled. Please recompile ParaView "
                 "with Python enabled.";
#endif
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::onTimeLabelChanged()
{
  QString timeName = "Time";
  if (this->Internal->Scene)
  {
    timeName =
      pqSMAdaptor::getElementProperty(
        this->Internal->Scene->getServer()->getTimeKeeper()->getProxy()->GetProperty("TimeLabel"))
        .toString();
  }

  // Update labels
  this->Internal->StartTimeLabel->setText(QString("Start %1:").arg(timeName));
  this->Internal->EndTimeLabel->setText(QString("End %1:").arg(timeName));
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::selectedDataProxyChanged(vtkSMProxy* proxy)
{
  this->Internal->SelectedDataProxy = proxy;
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::changeDataProxyDialogAccepted()
{
  if (!this->Internal->SelectedCueProxy)
  {
    return;
  }

  // set the proxy property
  vtkSMProxy* currentDataProxy =
    vtkSMPropertyHelper(this->Internal->SelectedCueProxy, "DataSource").GetAsProxy();
  if (this->Internal->SelectedDataProxy != currentDataProxy)
  {
    vtkSMPropertyHelper(this->Internal->SelectedCueProxy, "DataSource")
      .Set(this->Internal->SelectedDataProxy);
    this->Internal->SelectedCueProxy->UpdateVTKObjects();
  }
}

//-----------------------------------------------------------------------------
void pqAnimationViewWidget::generalSettingsChanged()
{
  this->Internal->AnimationWidget->animationModel()->setTimePrecision(
    vtkPVGeneralSettings::GetInstance()->GetAnimationTimePrecision());
  this->Internal->AnimationWidget->animationModel()->setTimeNotation(
    vtkPVGeneralSettings::GetInstance()->GetAnimationTimeNotation());
}
