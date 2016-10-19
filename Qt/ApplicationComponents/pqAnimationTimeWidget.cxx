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

#include "pqPropertyLinks.h"
#include "pqPropertyLinksConnection.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTimeKeeperProxy.h"
#include "vtkWeakPointer.h"

#include <QDoubleValidator>

class pqAnimationTimeWidget::pqInternals
{
public:
  Ui::AnimationTimeWidget Ui;
  vtkWeakPointer<vtkSMProxy> AnimationScene;
  void* AnimationSceneVoidPtr;
  pqPropertyLinks Links;
  int CachedTimestepCount;
  int Precision;

  pqInternals(pqAnimationTimeWidget* self)
    : AnimationSceneVoidPtr(NULL)
    , CachedTimestepCount(-1)
    , Precision(17)
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
  virtual ~pqAnimationTimeWidgetLinks() {}

protected:
  virtual QVariant currentServerManagerValue(bool use_unchecked) const
  {
    Q_ASSERT(use_unchecked == false);
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
  this->connect(ui.timeValue, SIGNAL(textChangedAndEditingFinished()), SIGNAL(timeValueChanged()));
  this->connect(ui.radioButtonValue, SIGNAL(toggled(bool)), SIGNAL(playModeChanged()));
  this->connect(
    ui.radioButtonValue, SIGNAL(toggled(bool)), SLOT(updateTimestepCountLabelVisibility()));
  this->connect(
    ui.timestepValue, SIGNAL(valueChangedAndEditingFinished()), SLOT(timestepValueChanged()));
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

  internals.Links.addPropertyLink(
    this, "timeValue", SIGNAL(timeValueChanged()), ascene, ascene->GetProperty("AnimationTime"));
  internals.Links.addPropertyLink(
    this, "playMode", SIGNAL(playModeChanged()), ascene, ascene->GetProperty("PlayMode"));

  // In a ParaView application, it's safe to assume that the timekeeper an
  // animation scene is using doesn't change in the life span of the scene.
  vtkSMProxy* atimekeeper = vtkSMPropertyHelper(ascene, "TimeKeeper").GetAsProxy();
  Q_ASSERT(atimekeeper != NULL);

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
  ui.timeValue->setTextAndResetCursor(QString::number(time, 'g', this->Internals->Precision));
  bool prev = ui.timestepValue->blockSignals(true);
  int index = vtkSMTimeKeeperProxy::GetLowerBoundTimeStepIndex(this->timeKeeper(), time);
  ui.timestepValue->setValue(index);
  ui.timestepValue->blockSignals(prev);
}

//-----------------------------------------------------------------------------
double pqAnimationTimeWidget::timeValue() const
{
  Ui::AnimationTimeWidget& ui = this->Internals->Ui;
  return ui.timeValue->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqAnimationTimeWidget::setTimePrecision(int val)
{
  this->Internals->Precision = val;
}

//-----------------------------------------------------------------------------
int pqAnimationTimeWidget::timePrecision() const
{
  return this->Internals->Precision;
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
  }
  else if (value == "Snap To TimeSteps")
  {
    ui.radioButtonStep->setChecked(true);
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
void pqAnimationTimeWidget::timestepValueChanged()
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
