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

#include "pqDoubleLineEdit.h"
#include "pqPropertyLinks.h"
#include "pqPropertyLinksConnection.h"
#include "pqTimer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTimeKeeperProxy.h"
#include "vtkWeakPointer.h"

#include <QDoubleValidator>

#include <cassert>

class pqAnimationTimeWidget::pqInternals
{
public:
  Ui::AnimationTimeWidget Ui;
  vtkWeakPointer<vtkSMProxy> AnimationScene;
  void* AnimationSceneVoidPtr;
  pqPropertyLinks Links;
  int CachedTimestepCount;
  int Precision;
  QChar TimeNotation;

  pqInternals(pqAnimationTimeWidget* self)
    : AnimationSceneVoidPtr(NULL)
    , CachedTimestepCount(-1)
    , Precision(6)
    , TimeNotation('g')
  {
    this->Ui.setupUi(self);
    this->Ui.timeValue->setValidator(new QDoubleValidator(self));
  }
};

namespace
{
/// Used to link the number of elements in a sm-property to the qt widget.
class pqAnimationTimeWidgetLinks : public pqPropertyLinksConnection
{
  typedef pqPropertyLinksConnection Superclass;

public:
  pqAnimationTimeWidgetLinks(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex, bool use_unchecked_modified_event,
    QObject* parentObject = 0)
    : Superclass(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
  }
  ~pqAnimationTimeWidgetLinks() override {}

protected:
  QVariant currentServerManagerValue(bool use_unchecked) const override
  {
    assert(use_unchecked == false);
    Q_UNUSED(use_unchecked);
    unsigned int count = vtkSMPropertyHelper(this->propertySM()).GetNumberOfElements();
    return QVariant(static_cast<int>(count));
  }

private:
  Q_DISABLE_COPY(pqAnimationTimeWidgetLinks)
};
}

//-----------------------------------------------------------------------------
pqAnimationTimeWidget::pqAnimationTimeWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqAnimationTimeWidget::pqInternals(this))
{
  this->setEnabled(false);

  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  ui.timeValueComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  this->connect(ui.timeValue, SIGNAL(textChangedAndEditingFinished()), SLOT(timeLineEditChanged()));
  this->connect(
    ui.timeValueComboBox, SIGNAL(currentTextChanged(QString)), SLOT(timeComboBoxChanged()));
  ui.timeValueComboBox->setVisible(false);
  this->connect(ui.radioButtonValue, SIGNAL(toggled(bool)), SLOT(timeRadioButtonToggled()));

  // the idiosyncrasies of QSpinBox make it so that we have to delay when we
  // respond to the "go-to-next" event (see paraview/paraview#18204).
  auto timer = new pqTimer(this);
  timer->setInterval(100);
  timer->setSingleShot(true);
  this->connect(timer, SIGNAL(timeout()), SLOT(timeSpinBoxChanged()));
  QObject::connect(
    ui.timestepValue, SIGNAL(valueChangedAndEditingFinished()), timer, SLOT(start()));
}

//-----------------------------------------------------------------------------
pqAnimationTimeWidget::~pqAnimationTimeWidget()
{
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setAnimationScene(vtkSMProxy* ascene)
{
  pqInternals& internals = *this->Internals;
  if (internals.AnimationSceneVoidPtr == ascene)
  {
    return;
  }

  internals.Links.clear();
  internals.AnimationScene = ascene;
  internals.AnimationSceneVoidPtr = ascene;
  this->setEnabled(ascene != NULL);
  if (!ascene)
  {
    return;
  }

  internals.Links.addTraceablePropertyLink(
    this, "timeValue", SIGNAL(timeValueChanged()), ascene, ascene->GetProperty("AnimationTime"));
  internals.Links.addTraceablePropertyLink(
    this, "playMode", SIGNAL(playModeChanged()), ascene, ascene->GetProperty("PlayMode"));

  // In a ParaView application, it's safe to assume that the timekeeper an
  // animation scene is using doesn't change in the life span of the scene.
  vtkSMProxy* atimekeeper = vtkSMPropertyHelper(ascene, "TimeKeeper").GetAsProxy();
  assert(atimekeeper != NULL);

  internals.Links.addPropertyLink<pqAnimationTimeWidgetLinks>(this, "timeStepCount",
    SIGNAL(dummySignal()), atimekeeper, atimekeeper->GetProperty("TimestepValues"));
  internals.Links.addPropertyLink(
    this, "timeLabel", SIGNAL(dummySignal()), atimekeeper, atimekeeper->GetProperty("TimeLabel"));
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationTimeWidget::animationScene() const
{
  pqInternals& internals = *this->Internals;
  return internals.AnimationScene;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationTimeWidget::timeKeeper() const
{
  pqInternals& internals = *this->Internals;
  return internals.AnimationScene
    ? vtkSMPropertyHelper(internals.AnimationScene, "TimeKeeper").GetAsProxy()
    : NULL;
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setTimeValue(double time)
{
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  ui.timeValue->setProperty("PQ_DOUBLE_VALUE", QVariant(time));

  QString textValue = this->formatDouble(time);

  ui.timeValue->setTextAndResetCursor(textValue);

  for (int index = 0; index < ui.timeValueComboBox->count(); index++)
  {
    if (ui.timeValueComboBox->itemData(index).toDouble() == time)
    {
      ui.timeValueComboBox->setCurrentIndex(index);
      break;
    }
  }

  bool prev = ui.timestepValue->blockSignals(true);
  int index = vtkSMTimeKeeperProxy::GetLowerBoundTimeStepIndex(this->timeKeeper(), time);
  ui.timestepValue->setValue(index);
  ui.timestepValue->blockSignals(prev);
}

//-----------------------------------------------------------------------------
double pqAnimationTimeWidget::timeValue() const
{
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  auto doubleValue = ui.timeValue->property("PQ_DOUBLE_VALUE");
  return doubleValue.isValid() ? doubleValue.toDouble() : ui.timeValue->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::timeLineEditChanged()
{
  auto& ui = this->Internals->Ui;
  const auto currentValue = ui.timeValue->text().toDouble();
  this->setTimeValue(currentValue);
  emit this->timeValueChanged();
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::timeComboBoxChanged()
{
  auto& ui = this->Internals->Ui;
  const auto currentValue = ui.timeValueComboBox->currentData().toDouble();
  this->setTimeValue(currentValue);
  emit this->timeValueChanged();
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setTimePrecision(int val)
{
  this->Internals->Precision = val;
  this->setTimeValue(this->timeValue());
  this->repopulateTimeComboBox();
}

//-----------------------------------------------------------------------------
int pqAnimationTimeWidget::timePrecision() const
{
  return this->Internals->Precision;
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setTimeNotation(const QChar& notation)
{
  QString possibilities = QString("eEfgG");
  if (possibilities.contains(notation) && this->Internals->TimeNotation != notation)
  {
    this->Internals->TimeNotation = notation;
    this->setTimeValue(this->timeValue());
    this->repopulateTimeComboBox();
  }
}

//-----------------------------------------------------------------------------
QChar pqAnimationTimeWidget::timeNotation() const
{
  return this->Internals->TimeNotation;
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setTimeStepCount(int value)
{
  pqInternals& internals = *this->Internals;
  internals.CachedTimestepCount = value;

  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  ui.timestepValue->setMaximum(value > 0 ? value - 1 : 0);
  ui.timestepCountLabel->setText(QString("(max is %1)").arg(value > 0 ? value - 1 : 0));
  ui.timestepCountLabel->setVisible((value > 0) && (this->playMode() == "Snap To TimeSteps"));

  bool prev = ui.timestepValue->blockSignals(true);
  ui.timestepValue->setValue(
    vtkSMTimeKeeperProxy::GetLowerBoundTimeStepIndex(this->timeKeeper(), this->timeValue()));
  ui.timestepValue->blockSignals(prev);

  this->repopulateTimeComboBox();
  this->updateTimestepCountLabelVisibility();
}

//-----------------------------------------------------------------------------
int pqAnimationTimeWidget::timeStepCount() const
{
  pqInternals& internals = *this->Internals;
  return internals.CachedTimestepCount;
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setPlayMode(const QString& value)
{
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  if (value == "Sequence" || value == "Real Time")
  {
    ui.radioButtonValue->setChecked(true);
    ui.timeValueComboBox->setVisible(false);
    ui.timeValue->setVisible(true);
  }
  else if (value == "Snap To TimeSteps")
  {
    ui.radioButtonStep->setChecked(true);
    ui.timeValueComboBox->setVisible(true);
    ui.timeValue->setVisible(false);
  }
  this->updateTimestepCountLabelVisibility();
}

//-----------------------------------------------------------------------------
QString pqAnimationTimeWidget::playMode() const
{
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  return ui.radioButtonValue->isChecked() ? "Sequence" : "Snap To TimeSteps";
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::updateTimestepCountLabelVisibility()
{
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  ui.timestepCountLabel->setVisible(
    (this->timeStepCount() > 0) && (this->playMode() == "Snap To TimeSteps"));
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::timeSpinBoxChanged()
{
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  int index = ui.timestepValue->value();
  vtkSMPropertyHelper helper(this->timeKeeper(), "TimestepValues");
  if (index >= 0 && static_cast<unsigned int>(index) < helper.GetNumberOfElements())
  {
    this->setTimeValue(helper.GetAsDouble(index));
    emit this->timeValueChanged();
  }
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
void pqAnimationTimeWidget::repopulateTimeComboBox()
{
  if (this->timeKeeper() == nullptr)
  {
    return;
  }

  double currentTimeValue = this->timeValue();
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;

  vtkSMPropertyHelper helper(this->timeKeeper(), "TimestepValues");
  bool prev = ui.timeValueComboBox->blockSignals(true);
  ui.timeValueComboBox->clear();

  for (unsigned int index = 0; index < helper.GetNumberOfElements(); index++)
  {
    QString textValue = this->formatDouble(helper.GetAsDouble(index));
    ui.timeValueComboBox->addItem(textValue, helper.GetAsDouble(index));
  }
  ui.timeValueComboBox->blockSignals(prev);

  this->setTimeValue(currentTimeValue);
}

//-----------------------------------------------------------------------------
QString pqAnimationTimeWidget::formatDouble(double value)
{
  QChar format = this->Internals->TimeNotation.toUpper();
  QTextStream::RealNumberNotation notation = QTextStream::ScientificNotation;
  if (format == QChar('F'))
  {
    notation = QTextStream::FixedNotation;
  }
  else if (format == QChar('G'))
  {
    notation = QTextStream::SmartNotation;
  }

  QString textValue = pqDoubleLineEdit::formatDouble(value, notation, this->Internals->Precision);

  return textValue;
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::timeRadioButtonToggled()
{
  this->setPlayMode(this->playMode());
  emit this->playModeChanged();
}
