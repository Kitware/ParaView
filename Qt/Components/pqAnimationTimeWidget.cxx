// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAnimationTimeWidget.h"
#include "ui_pqAnimationTimeWidget.h"

#include "pqAnimationScene.h"
#include "pqCoreUtilities.h"
#include "pqDoubleLineEdit.h"
#include "pqPropertyLinks.h"
#include "pqPropertyLinksConnection.h"
#include "pqTimer.h"
#include "pqUndoStack.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTimeKeeperProxy.h"
#include "vtkSMTrace.h"
#include "vtkWeakPointer.h"

#include <QDoubleValidator>
#include <QList>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QVariant>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <vector>

namespace
{
class WidgetState
{
  // Timesteps comes from TimeKeeper
  std::vector<double> TimeSteps;
  // Computed from AnimationSceneProperty, this is not the same as TimeSteps but as same usage here.
  // We store it appart to be safe with PropertyLinks.
  std::vector<double> SequenceTimeSteps;
  double Time = 0.0;
  bool PlayModeSnapToTimeSteps = false;

public:
  bool DisableModifications = false;

  double StartTime = 0;
  double EndTime = 1;

  bool playModeSnapToTimeSteps() const { return this->PlayModeSnapToTimeSteps; }
  double time() const { return this->Time; }

  /**
   * TimeSteps is up to date with TimeKeeper, while SequenceTimeSteps is computed from
   * AnimationScene properties. They differ in origin but are similar in use in this class. This
   * method allows to use the relevant one, hiding the switch logic.
   */
  const std::vector<double>& timeSteps() const
  {
    return playModeSnapToTimeSteps() ? this->TimeSteps : this->SequenceTimeSteps;
  }
  const QList<QVariant>& timeStepsAsVariantList() const
  {
    this->TimeStepsVariants.clear();
    std::transform(this->TimeSteps.begin(), this->TimeSteps.end(),
      std::back_inserter(this->TimeStepsVariants), [](const double& val) { return QVariant(val); });
    return this->TimeStepsVariants;
  }

  /**
   * Checks if the index in input has an associated timestep.
   * If so, the timestep is passed to the input timestep parameter and the function returns true.
   */
  bool checkTimeStepFromIndex(int index, double& timestep)
  {
    if (!this->DisableModifications && index >= 0 &&
      index < static_cast<int>(this->timeSteps().size()))
    {
      timestep = this->timeSteps()[index];
      return this->canTimeBeSet(timestep);
    }
    return false;
  }

  // sets the current time
  void setTime(double time) { this->Time = time; }

  // checks if the time value in input can be set to the current time
  bool canTimeBeSet(double time) { return (!this->DisableModifications && time != this->Time); }

  bool setPlayModeSnapToTimeSteps(bool val)
  {
    if (!this->DisableModifications && this->PlayModeSnapToTimeSteps != val)
    {
      this->PlayModeSnapToTimeSteps = val;
      return true;
    }
    return false;
  }

  void updateSequenceTimeSteps() { this->updateSequenceTimeSteps(this->numberOfFrames()); }

  void updateSequenceTimeSteps(int nbOfFrames)
  {
    this->SequenceTimeSteps.resize(nbOfFrames);
    if (nbOfFrames > 1)
    {
      double step = (this->EndTime - this->StartTime) / (nbOfFrames - 1);
      for (int idx = 0; idx < nbOfFrames; idx++)
      {
        this->SequenceTimeSteps[idx] = this->StartTime + idx * step;
      }
    }
    else if (nbOfFrames == 1)
    {
      this->SequenceTimeSteps[0] = this->StartTime;
    }
  }

  int numberOfFrames() { return static_cast<int>(this->SequenceTimeSteps.size()); }

  bool setTimeSteps(const QList<QVariant>& list)
  {
    if (this->DisableModifications)
    {
      return false;
    }

    std::vector<double> times(list.size());
    std::transform(
      list.begin(), list.end(), times.begin(), [](const QVariant& var) { return var.toDouble(); });
    if (this->TimeSteps != times)
    {
      this->TimeSteps = std::move(times);
      return true;
    }
    return false;
  }

private:
  mutable QList<QVariant> TimeStepsVariants;
};
}
class pqAnimationTimeWidget::pqInternals
{
public:
  Ui::AnimationTimeWidget Ui;
  vtkWeakPointer<vtkSMProxy> AnimationScene;
  void* AnimationSceneVoidPtr;
  pqPropertyLinks Links;

  WidgetState State;

  pqInternals(pqAnimationTimeWidget* self)
    : AnimationSceneVoidPtr(nullptr)
  {
    this->Ui.setupUi(self);
    this->Ui.timeValueComboBox->setValidator(new QDoubleValidator(self));
    this->Ui.timeValueComboBox->setLineEdit(new pqDoubleLineEdit(self));

    // setup a min size for input widgets so it do not resize to often
    // and so avoid a "flickering" effect.
    auto metrics = this->Ui.timeValueComboBox->fontMetrics();
    auto minW = metrics.horizontalAdvance("210.123456789");
    this->Ui.timeValueComboBox->setMinimumWidth(minW);
    metrics = this->Ui.timestepValue->fontMetrics();
    minW = metrics.horizontalAdvance("12345 (?) ");
    this->Ui.timestepValue->setMinimumWidth(minW);
  }

  /**
   * Update UI based on current state.
   * This should never fire any modification events.
   */
  void render(pqAnimationTimeWidget* self);

  // Returns the current timekeeper.
  vtkSMProxy* timeKeeper() const
  {
    return this->AnimationScene
      ? vtkSMPropertyHelper(this->AnimationScene, "TimeKeeper").GetAsProxy()
      : nullptr;
  }
};

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::pqInternals::render(pqAnimationTimeWidget* self)
{
  QSignalBlocker blocker(self);
  QScopedValueRollback<bool> rollback(this->State.DisableModifications, true);

  auto& ui = this->Ui;
  auto& state = this->State;

  ui.timestepCountLabel->setVisible(state.playModeSnapToTimeSteps());

  // update combo-box.
  ui.timeValueComboBox->clear();
  int currentIndex = -1;
  for (const auto& val : state.timeSteps())
  {
    if (val == state.time())
    {
      currentIndex = ui.timeValueComboBox->count();
    }
    else if (currentIndex == -1 && state.time() < val)
    {
      // `state.time()` is not part of the current timesteps, insert it.
      currentIndex = ui.timeValueComboBox->count();
      ui.timeValueComboBox->addItem(pqCoreUtilities::formatTime(state.time())), state.time();
    }

    ui.timeValueComboBox->addItem(pqCoreUtilities::formatTime(val), val);
  }

  if (currentIndex == -1)
  {
    // `state.time()` is not part of the current timesteps, append it.
    currentIndex = ui.timeValueComboBox->count();
    ui.timeValueComboBox->addItem(
      QString("%1 (?)").arg(pqCoreUtilities::formatTime(state.time())), state.time());
  }
  ui.timeValueComboBox->setCurrentIndex(currentIndex);

  // update spin-box.
  if (!state.timeSteps().empty())
  {
    const int count = static_cast<int>(state.timeSteps().size());
    ui.timestepValue->setMaximum(count - 1);
    int idx = 0;
    bool found = false;
    for (auto ts : state.timeSteps())
    {
      if (ts >= state.time())
      {
        found = true;
        break;
      }
      idx++;
    }

    if (found)
    {
      ui.timestepValue->setValue(idx);
      ui.timestepValue->setSuffix(state.timeSteps()[idx] == state.time() ? "" : " (?)");
      ui.timestepCountLabel->setText(tr("max is %1").arg(count - 1));
    }
  }
  else
  {
    ui.timestepValue->setMaximum(0);
    ui.timestepValue->setValue(0);
    ui.timestepValue->setPrefix(QString());
    ui.timestepCountLabel->setText(QString());
  }
}

//-----------------------------------------------------------------------------
pqAnimationTimeWidget::pqAnimationTimeWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqAnimationTimeWidget::pqInternals(this))
{
  this->setEnabled(false);

  auto& internals = (*this->Internals);
  auto& ui = internals.Ui;

  QObject::connect(ui.timeValueComboBox->lineEdit(), &QLineEdit::editingFinished,
    [this, &internals, &ui]()
    {
      // user has selected a timestep using the combo-box.
      const double time = ui.timeValueComboBox->currentData().toDouble();
      if (internals.State.canTimeBeSet(time))
      {
        this->setCurrentTime(time);
      }
    });

  QObject::connect(ui.timeValueComboBox, &QComboBox::currentTextChanged,
    [this, &internals, &ui]()
    {
      // user has selected a timestep using the combo-box.
      const double time = ui.timeValueComboBox->currentData().toDouble();
      if (internals.State.canTimeBeSet(time))
      {
        this->setCurrentTime(time);
      }
    });

  // the idiosyncrasies of QSpinBox make it so that we have to delay when we
  // respond to the "go-to-next" event (see paraview/paraview#18204).
  auto timer = new pqTimer(this);
  timer->setInterval(100);
  timer->setSingleShot(true);
  QObject::connect(timer, &QTimer::timeout,
    [this, &internals, &ui]()
    {
      // user has changed the time-step in the spinbox.
      const int timeIndex = ui.timestepValue->value();
      double time;
      if (internals.State.checkTimeStepFromIndex(timeIndex, time))
      {
        this->setCurrentTime(time);
      }
    });

  QObject::connect(
    ui.timestepValue, SIGNAL(valueChangedAndEditingFinished()), timer, SLOT(start()));
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setCurrentTime(double t)
{
  BEGIN_UNDO_EXCLUDE();

  vtkSMProxy* animationScene = this->Internals->AnimationScene;
  {
    // Use another scope to prevent modifications to the TimeKeeper from
    // being traced.
    SM_SCOPED_TRACE(PropertiesModified).arg("proxy", animationScene);
    vtkSMPropertyHelper(animationScene, "AnimationTime").Set(t);
  }
  animationScene->UpdateVTKObjects();

  END_UNDO_EXCLUDE();
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::updateCurrentTime(double t)
{
  this->Internals->State.setTime(t);
  this->Internals->render(this);
}

//-----------------------------------------------------------------------------
pqAnimationTimeWidget::~pqAnimationTimeWidget() = default;

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setAnimationScene(pqAnimationScene* ascene)
{
  vtkSMProxy* asceneProxy = ascene ? ascene->getProxy() : nullptr;

  pqInternals& internals = *this->Internals;
  if (internals.AnimationSceneVoidPtr == asceneProxy)
  {
    return;
  }

  internals.Links.clear();
  internals.AnimationScene = asceneProxy;
  internals.AnimationSceneVoidPtr = asceneProxy;
  this->setEnabled(asceneProxy != nullptr);
  if (!asceneProxy)
  {
    return;
  }

  // In a ParaView application, it's safe to assume that the timekeeper an
  // animation scene is using doesn't change in the life span of the scene.
  vtkSMProxy* atimekeeper = internals.timeKeeper();
  assert(atimekeeper != nullptr);

  QObject::connect(ascene, SIGNAL(animationTime(double)), this, SLOT(updateCurrentTime(double)));

  internals.Links.addTraceablePropertyLink(
    this, "playMode", SIGNAL(playModeChanged()), asceneProxy, asceneProxy->GetProperty("PlayMode"));

  internals.Links.addTraceablePropertyLink(this, "numberOfFrames", SIGNAL(dummySignal()),
    asceneProxy, asceneProxy->GetProperty("NumberOfFrames"));
  internals.Links.addTraceablePropertyLink(
    this, "startTime", SIGNAL(dummySignal()), asceneProxy, asceneProxy->GetProperty("StartTime"));
  internals.Links.addTraceablePropertyLink(
    this, "endTime", SIGNAL(dummySignal()), asceneProxy, asceneProxy->GetProperty("EndTime"));

  // uni-directional links.
  internals.Links.addPropertyLink(
    this, "timeLabel", SIGNAL(dummySignal()), atimekeeper, atimekeeper->GetProperty("TimeLabel"));

  internals.Links.addPropertyLink(this, "timestepValues", SIGNAL(dummySignal()), atimekeeper,
    atimekeeper->GetProperty("TimestepValues"));
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationTimeWidget::animationScene() const
{
  pqInternals& internals = *this->Internals;
  return internals.AnimationScene;
}

//-----------------------------------------------------------------------------
const QList<QVariant>& pqAnimationTimeWidget::timestepValues() const
{
  const auto& internals = (*this->Internals);
  return internals.State.timeStepsAsVariantList();
}

//-----------------------------------------------------------------------------
int pqAnimationTimeWidget::precision() const
{
  return pqDoubleLineEdit::globalPrecision();
}

//-----------------------------------------------------------------------------
pqAnimationTimeWidget::RealNumberNotation pqAnimationTimeWidget::notation() const
{
  return pqDoubleLineEdit::globalNotation();
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setPlayMode(const QString& value)
{
  auto& internals = (*this->Internals);
  internals.State.setPlayModeSnapToTimeSteps(value == "Snap To TimeSteps");
  internals.render(this);
}

//-----------------------------------------------------------------------------
QString pqAnimationTimeWidget::playMode() const
{
  auto& internals = (*this->Internals);
  return internals.State.playModeSnapToTimeSteps() == false ? "Sequence" : "Snap To TimeSteps";
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setStartTime(double start)
{
  this->Internals->State.StartTime = start;
  this->Internals->State.updateSequenceTimeSteps();
  this->Internals->render(this);
}

//-----------------------------------------------------------------------------
double pqAnimationTimeWidget::startTime() const
{
  return this->Internals->State.StartTime;
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setEndTime(double start)
{
  this->Internals->State.EndTime = start;
  this->Internals->State.updateSequenceTimeSteps();
  this->Internals->render(this);
}

//-----------------------------------------------------------------------------
double pqAnimationTimeWidget::endTime() const
{
  return this->Internals->State.EndTime;
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setNumberOfFrames(int nbOfFrames)
{
  this->Internals->State.updateSequenceTimeSteps(nbOfFrames);
  this->Internals->render(this);
}

//-----------------------------------------------------------------------------
int pqAnimationTimeWidget::numberOfFrames() const
{
  return this->Internals->State.numberOfFrames();
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setTimeLabel(const QString& val)
{
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  ui.timeLabel->setText(val + ":");
}

//-----------------------------------------------------------------------------
QString pqAnimationTimeWidget::timeLabel() const
{
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  QString txt = ui.timeLabel->text();
  return txt.left(txt.length() - 1);
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setTimestepValues(const QList<QVariant>& list)
{
  auto& internals = (*this->Internals);
  if (internals.State.setTimeSteps(list))
  {
    internals.render(this);
  }
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::render()
{
  this->Internals->render(this);
}
