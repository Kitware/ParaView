/*=========================================================================

   Program: ParaView
   Module:  pqAnimationTimeWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
  std::vector<double> TimeSteps;
  double Time = 0.0;
  bool PlayModeSnapToTimeSteps = false;

public:
  bool DisableModifications = false;

  bool playModeSnapToTimeSteps() const { return this->PlayModeSnapToTimeSteps; }
  double time() const { return this->Time; }
  const std::vector<double>& timeSteps() const { return this->TimeSteps; }
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
      index < static_cast<int>(this->TimeSteps.size()))
    {
      timestep = this->TimeSteps[index];
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
    this->Ui.timeValue->setValidator(new QDoubleValidator(self));
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
  const auto& state = this->State;

  // first, update enabled state and visibilities based on playmode.
  if (state.playModeSnapToTimeSteps())
  {
    ui.timeValue->hide();
    ui.timeValueComboBox->show();
    ui.timestepCountLabel->show();
    ui.radioButtonStep->setChecked(true);
  }
  else
  {
    ui.timeValue->show();
    ui.timeValueComboBox->hide();
    ui.timestepCountLabel->hide();
    ui.radioButtonValue->setChecked(true);
  }

  ui.timeValue->setText(pqCoreUtilities::number(state.time()));

  // update combo-box.
  ui.timeValueComboBox->clear();
  int currentIndex = -1;
  const auto precision = ui.timeValue->precision();
  const auto notation = ui.timeValue->notation();
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
      ui.timeValueComboBox->addItem(
        QString("%1 (?)").arg(pqDoubleLineEdit::formatDouble(state.time(), notation, precision)),
        state.time());
    }

    ui.timeValueComboBox->addItem(pqDoubleLineEdit::formatDouble(val, notation, precision), val);
  }

  if (currentIndex == -1)
  {
    // `state.time()` is not part of the current timesteps, append it.
    currentIndex = ui.timeValueComboBox->count();
    ui.timeValueComboBox->addItem(
      QString("%1 (?)").arg(pqDoubleLineEdit::formatDouble(state.time(), notation, precision)),
      state.time());
  }
  ui.timeValueComboBox->setCurrentIndex(currentIndex);

  // update spin-box.
  if (state.timeSteps().size() > 0)
  {
    const int count = static_cast<int>(state.timeSteps().size());
    ui.timestepValue->setMaximum(count - 1);
    auto idx = vtkSMTimeKeeperProxy::GetLowerBoundTimeStepIndex(this->timeKeeper(), state.time());
    ui.timestepValue->setValue(idx);
    ui.timestepValue->setSuffix(state.timeSteps()[idx] == state.time() ? "" : " (?)");
    ui.timestepCountLabel->setText(QString("max is %1").arg(count - 1));
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
  ui.timeValueComboBox->setVisible(false);

  QObject::connect(
    ui.timeValue, &pqDoubleLineEdit::textChangedAndEditingFinished, [this, &internals, &ui]() {
      // user has manually changed the time-value in the line edit.
      const double time = ui.timeValue->text().toDouble();
      if (internals.State.canTimeBeSet(time))
      {
        setCurrentTime(time);
      }
    });

  QObject::connect(ui.timeValueComboBox, &QComboBox::currentTextChanged, [this, &internals, &ui]() {
    // user has selected a timestep using the combo-box.
    const double time = ui.timeValueComboBox->currentData().toDouble();
    if (internals.State.canTimeBeSet(time))
    {
      setCurrentTime(time);
    }
  });

  QObject::connect(ui.radioButtonValue, &QRadioButton::toggled, [this, &internals, &ui]() {
    const bool playModeSnapToTimeSteps = (ui.radioButtonValue->isChecked() == false);
    if (internals.State.setPlayModeSnapToTimeSteps(playModeSnapToTimeSteps))
    {
      internals.render(this);
      Q_EMIT this->playModeChanged();
    }
  });

  // the idiosyncrasies of QSpinBox make it so that we have to delay when we
  // respond to the "go-to-next" event (see paraview/paraview#18204).
  auto timer = new pqTimer(this);
  timer->setInterval(100);
  timer->setSingleShot(true);
  QObject::connect(timer, &QTimer::timeout, [this, &internals, &ui]() {
    // user has changed the time-step in the spinbox.
    const int timeIndex = ui.timestepValue->value();
    double time;
    if (internals.State.checkTimeStepFromIndex(timeIndex, time))
    {
      setCurrentTime(time);
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
void pqAnimationTimeWidget::setPrecision(int val)
{
  auto& internals = (*this->Internals);
  internals.Ui.timeValue->setPrecision(val);
  internals.render(this);
}

//-----------------------------------------------------------------------------
int pqAnimationTimeWidget::precision() const
{
  auto& internals = (*this->Internals);
  return internals.Ui.timeValue->precision();
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setNotation(pqAnimationTimeWidget::RealNumberNotation val)
{
  auto& internals = (*this->Internals);
  internals.Ui.timeValue->setNotation(val);
  internals.render(this);
}

//-----------------------------------------------------------------------------
pqAnimationTimeWidget::RealNumberNotation pqAnimationTimeWidget::notation() const
{
  auto& internals = (*this->Internals);
  return internals.Ui.timeValue->notation();
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setPlayMode(const QString& value)
{
  auto& internals = (*this->Internals);
  if (internals.State.setPlayModeSnapToTimeSteps(value == "Snap To TimeSteps"))
  {
    internals.render(this);
  }
}

//-----------------------------------------------------------------------------
QString pqAnimationTimeWidget::playMode() const
{
  auto& internals = (*this->Internals);
  return internals.State.playModeSnapToTimeSteps() == false ? "Sequence" : "Snap To TimeSteps";
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
void pqAnimationTimeWidget::setPlayModeReadOnly(bool val)
{
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  ui.radioButtonValue->setVisible(!val);
  ui.radioButtonStep->setVisible(!val);
}

//-----------------------------------------------------------------------------
bool pqAnimationTimeWidget::playModeReadOnly() const
{
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  return !ui.radioButtonStep->isVisible();
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

//=============================================================================
#if !defined(VTK_LEGACY_REMOVE)
void pqAnimationTimeWidget::setTimePrecision(int val)
{
  VTK_LEGACY_REPLACED_BODY(
    pqAnimationTimeWidget::setTimePrecision, "ParaView 5.9", pqAnimationTimeWidget::setPrecision);
  this->setPrecision(val);
}

int pqAnimationTimeWidget::timePrecision() const
{
  VTK_LEGACY_REPLACED_BODY(
    pqAnimationTimeWidget::timePrecision, "ParaView 5.9", pqAnimationTimeWidget::precision);
  return this->precision();
}

void pqAnimationTimeWidget::setTimeNotation(const QChar& val)
{
  VTK_LEGACY_REPLACED_BODY(
    pqAnimationTimeWidget::setTimeNotation, "ParaView 5.9", pqAnimationTimeWidget::setNotation);
  auto nval = RealNumberNotation::MixedNotation;
  switch (val.toLatin1())
  {
    case 'f':
    case 'F':
      nval = RealNumberNotation::FixedNotation;
      break;
    case 'g':
    case 'G':
      nval = RealNumberNotation::MixedNotation;
      break;
    default:
      nval = RealNumberNotation::ScientificNotation;
      break;
  }
  this->setNotation(nval);
}

QChar pqAnimationTimeWidget::timeNotation() const
{
  VTK_LEGACY_REPLACED_BODY(
    pqAnimationTimeWidget::timeNotation, "ParaView 5.9", pqAnimationTimeWidget::notation);
  switch (this->notation())
  {
    case RealNumberNotation::FixedNotation:
      return 'f';
    case RealNumberNotation::ScientificNotation:
      return 'e';
    case RealNumberNotation::MixedNotation:
    default:
      return 'g';
  }
}

void pqAnimationTimeWidget::setTimeStepCount(int)
{
  VTK_LEGACY_BODY(pqAnimationTimeWidget::setTimeStepCount, "ParaView 5.9");
}

int pqAnimationTimeWidget::timeStepCount() const
{
  VTK_LEGACY_BODY(pqAnimationTimeWidget::timeStepCount, "ParaView 5.9");
  return 0;
}
#endif // !defined(VTK_LEGACY_REMOVE)
//=============================================================================
