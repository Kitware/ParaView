/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
/*=========================================================================

  Library:   CTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

// Qt includes
#include <QDebug>
#include <QHBoxLayout>

// CTK includes
#include "ctkDoubleRangeSlider.h"
#include "ctkRangeSlider.h"

// STD includes
#include <limits>

//-----------------------------------------------------------------------------
class ctkDoubleRangeSliderPrivate
{
  Q_DECLARE_PUBLIC(ctkDoubleRangeSlider);

protected:
  ctkDoubleRangeSlider* const q_ptr;

public:
  ctkDoubleRangeSliderPrivate(ctkDoubleRangeSlider& object);

  int toInt(double _value) const;
  double minFromInt(int _value) const;
  double maxFromInt(int _value) const;
  double safeMinFromInt(int _value) const;
  double safeMaxFromInt(int _value) const;

  void init();
  void connectSlider();
  void updateMinOffset(double value);
  void updateMaxOffset(double value);

  ctkRangeSlider* Slider;
  double Minimum;
  double Maximum;
  bool SettingRange;

  // we should have a MinValueOffset and MinPositionOffset (and MinimumOffset?)
  double MinOffset;
  // we should have a MaxValueOffset and MaxPositionOffset (and MaximumOffset?)
  double MaxOffset;
  double SingleStep;
  double MinValue;
  double MaxValue;

private:
  ctkDoubleRangeSliderPrivate& operator=(const ctkDoubleRangeSliderPrivate&);
};

// --------------------------------------------------------------------------
ctkDoubleRangeSliderPrivate::ctkDoubleRangeSliderPrivate(ctkDoubleRangeSlider& object)
  : q_ptr(&object)
{
  // the initial values will be overwritten in
  // ctkDoubleRangeSliderPrivate::init()
  this->Slider = 0;
  this->Minimum = 0.;
  this->Maximum = 99.;
  this->SettingRange = false;
  this->MinOffset = 0.;
  this->MaxOffset = 0.;
  this->SingleStep = 1.;
  this->MinValue = 0.;
  this->MaxValue = 99.;
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSliderPrivate::init()
{
  Q_Q(ctkDoubleRangeSlider);
  this->Slider = new ctkRangeSlider(q);
  QHBoxLayout* l = new QHBoxLayout(q);
  l->addWidget(this->Slider);
  l->setContentsMargins(0, 0, 0, 0);

  this->Minimum = this->Slider->minimum();
  this->Maximum = this->Slider->maximum();
  this->MinValue = this->Slider->minimumValue();
  this->MaxValue = this->Slider->maximumValue();
  this->SingleStep = this->Slider->singleStep();

  q->setSizePolicy(this->Slider->sizePolicy());
  q->setAttribute(Qt::WA_WState_OwnSizePolicy, false);

  this->connectSlider();
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSliderPrivate::connectSlider()
{
  Q_Q(ctkDoubleRangeSlider);
  q->connect(this->Slider, SIGNAL(valuesChanged(int, int)), q, SLOT(onValuesChanged(int, int)));

  q->connect(this->Slider, SIGNAL(minimumPositionChanged(int)), q, SLOT(onMinPosChanged(int)));
  q->connect(this->Slider, SIGNAL(maximumPositionChanged(int)), q, SLOT(onMaxPosChanged(int)));
  q->connect(
    this->Slider, SIGNAL(positionsChanged(int, int)), q, SLOT(onPositionsChanged(int, int)));

  q->connect(this->Slider, SIGNAL(sliderPressed()), q, SIGNAL(sliderPressed()));
  q->connect(this->Slider, SIGNAL(sliderReleased()), q, SIGNAL(sliderReleased()));
  q->connect(this->Slider, SIGNAL(rangeChanged(int, int)), q, SLOT(onRangeChanged(int, int)));
}

// --------------------------------------------------------------------------
int ctkDoubleRangeSliderPrivate::toInt(double doubleValue) const
{
  double tmp = doubleValue / this->SingleStep;
  static const double minInt = std::numeric_limits<int>::min();
  static const double maxInt = std::numeric_limits<int>::max();
#ifndef QT_NO_DEBUG
  if (tmp < minInt || tmp > maxInt)
  {
    qWarning() << __FUNCTION__ << ": value " << doubleValue << " for singleStep "
               << this->SingleStep << " is out of integer bounds !";
  }
#endif
  tmp = qBound(minInt, tmp, maxInt);
  int intValue = qRound(tmp);
  // qDebug() << __FUNCTION__ << doubleValue << tmp << intValue;
  return intValue;
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSliderPrivate::minFromInt(int intValue) const
{
  double doubleValue = this->SingleStep * (this->MinOffset + intValue);
  return doubleValue;
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSliderPrivate::maxFromInt(int intValue) const
{
  double doubleValue = this->SingleStep * (this->MaxOffset + intValue);
  return doubleValue;
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSliderPrivate::safeMinFromInt(int intValue) const
{
  return qBound(this->Minimum, this->minFromInt(intValue), this->Maximum);
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSliderPrivate::safeMaxFromInt(int intValue) const
{
  return qBound(this->Minimum, this->maxFromInt(intValue), this->Maximum);
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSliderPrivate::updateMinOffset(double value)
{
  this->MinOffset = (value / this->SingleStep) - this->toInt(value);
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSliderPrivate::updateMaxOffset(double value)
{
  this->MaxOffset = (value / this->SingleStep) - this->toInt(value);
}

// --------------------------------------------------------------------------
ctkDoubleRangeSlider::ctkDoubleRangeSlider(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new ctkDoubleRangeSliderPrivate(*this))
{
  Q_D(ctkDoubleRangeSlider);
  d->init();
}

// --------------------------------------------------------------------------
ctkDoubleRangeSlider::ctkDoubleRangeSlider(Qt::Orientation _orientation, QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new ctkDoubleRangeSliderPrivate(*this))
{
  Q_D(ctkDoubleRangeSlider);
  d->init();
  this->setOrientation(_orientation);
}

// --------------------------------------------------------------------------
ctkDoubleRangeSlider::~ctkDoubleRangeSlider()
{
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setMinimum(double min)
{
  Q_D(ctkDoubleRangeSlider);
  double oldMin = d->Minimum;
  d->Minimum = min;
  if (d->Minimum >= d->MinValue)
  { // TBD: use same offset
    d->updateMinOffset(d->Minimum);
  }
  if (d->Minimum >= d->MaxValue)
  { // TBD: use same offset
    d->updateMaxOffset(d->Minimum);
  }
  d->SettingRange = true;
  d->Slider->setMinimum(d->toInt(min));
  d->SettingRange = false;
  if (d->Minimum != oldMin)
  {
    emit this->rangeChanged(d->Minimum, d->Maximum);
  }
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSlider::minimum() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->Minimum;
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setMaximum(double max)
{
  Q_D(ctkDoubleRangeSlider);
  double oldMax = d->Maximum;
  d->Maximum = max;
  if (d->Maximum <= d->MinValue)
  { // TBD: use same offset
    d->updateMinOffset(d->Maximum);
  }
  if (d->Maximum <= d->MaxValue)
  { // TBD: use same offset ?
    d->updateMaxOffset(d->Maximum);
  }
  d->SettingRange = true;
  d->Slider->setMaximum(d->toInt(max));
  d->SettingRange = false;
  if (d->Maximum != oldMax)
  {
    emit this->rangeChanged(d->Minimum, d->Maximum);
  }
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSlider::maximum() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->Maximum;
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setRange(double min, double max)
{
  Q_D(ctkDoubleRangeSlider);
  double oldMin = d->Minimum;
  double oldMax = d->Maximum;
  d->Minimum = min;
  d->Maximum = max;
  if (d->Minimum >= d->MinValue)
  { // TBD: use same offset
    d->updateMinOffset(d->Minimum);
  }
  if (d->Minimum >= d->MaxValue)
  { // TBD: use same offset
    d->updateMaxOffset(d->Minimum);
  }
  if (d->Maximum <= d->MinValue)
  { // TBD: use same offset
    d->updateMinOffset(d->Maximum);
  }
  if (d->Maximum <= d->MaxValue)
  { // TBD: use same offset ?
    d->updateMaxOffset(d->Maximum);
  }
  d->SettingRange = true;
  d->Slider->setRange(d->toInt(min), d->toInt(max));
  d->SettingRange = false;
  if (d->Minimum != oldMin || d->Maximum != oldMax)
  {
    emit this->rangeChanged(d->Minimum, d->Maximum);
  }
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSlider::minimumPosition() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->safeMinFromInt(d->Slider->minimumPosition());
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setMinimumPosition(double minPos)
{
  Q_D(ctkDoubleRangeSlider);
  d->Slider->setMinimumPosition(d->toInt(minPos));
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSlider::maximumPosition() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->safeMaxFromInt(d->Slider->maximumPosition());
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setMaximumPosition(double maxPos)
{
  Q_D(ctkDoubleRangeSlider);
  d->Slider->setMaximumPosition(d->toInt(maxPos));
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setPositions(double minPos, double maxPos)
{
  Q_D(ctkDoubleRangeSlider);
  d->Slider->setPositions(d->toInt(minPos), d->toInt(maxPos));
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSlider::minimumValue() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->MinValue;
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setMinimumValue(double newMinValue)
{
  Q_D(ctkDoubleRangeSlider);
  newMinValue = qBound(d->Minimum, newMinValue, d->Maximum);
  d->updateMinOffset(newMinValue);
  if (newMinValue >= d->MaxValue)
  {
    d->updateMaxOffset(newMinValue);
  }
  int newIntValue = d->toInt(newMinValue);
  if (newIntValue != d->Slider->minimumValue())
  {
    // d->Slider will emit a minimumValueChanged signal that is connected to
    // ctkDoubleSlider::onValueChanged
    d->Slider->setMinimumValue(newIntValue);
  }
  else
  {
    double oldValue = d->MinValue;
    d->MinValue = newMinValue;
    // don't emit a valuechanged signal if the new value is quite
    // similar to the old value.
    if (qAbs(newMinValue - oldValue) > (d->SingleStep * 0.000000001))
    {
      emit this->valuesChanged(newMinValue, this->maximumValue());
      emit this->minimumValueChanged(newMinValue);
    }
  }
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSlider::maximumValue() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->MaxValue;
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setMaximumValue(double newMaxValue)
{
  Q_D(ctkDoubleRangeSlider);
  newMaxValue = qBound(d->Minimum, newMaxValue, d->Maximum);
  d->updateMaxOffset(newMaxValue);
  if (newMaxValue <= d->MinValue)
  {
    d->updateMinOffset(newMaxValue);
  }
  int newIntValue = d->toInt(newMaxValue);
  if (newIntValue != d->Slider->maximumValue())
  {
    // d->Slider will emit a maximumValueChanged signal that is connected to
    // ctkDoubleSlider::onValueChanged
    d->Slider->setMaximumValue(newIntValue);
  }
  else
  {
    double oldValue = d->MaxValue;
    d->MaxValue = newMaxValue;
    // don't emit a valuechanged signal if the new value is quite
    // similar to the old value.
    if (qAbs(newMaxValue - oldValue) > (d->SingleStep * 0.000000001))
    {
      emit this->valuesChanged(this->minimumValue(), newMaxValue);
      emit this->maximumValueChanged(newMaxValue);
    }
  }
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setValues(double newMinVal, double newMaxVal)
{
  Q_D(ctkDoubleRangeSlider);
  // We can't call setMinimumValue() and setMaximumValue() as they would
  // generate an inconsistent state. when minimumValueChanged() is fired the
  // new max value wouldn't be updated yet.
  double newMinValue = qBound(d->Minimum, qMin(newMinVal, newMaxVal), d->Maximum);
  double newMaxValue = qBound(d->Minimum, qMax(newMinVal, newMaxVal), d->Maximum);
  d->updateMinOffset(newMinValue);
  d->updateMaxOffset(newMaxValue);
  int newMinIntValue = d->toInt(newMinValue);
  int newMaxIntValue = d->toInt(newMaxValue);
  if (newMinIntValue != d->Slider->minimumValue() || newMaxIntValue != d->Slider->maximumValue())
  {
    // d->Slider will emit a maximumValueChanged signal that is connected to
    // ctkDoubleSlider::onValueChanged
    d->Slider->setValues(newMinIntValue, newMaxIntValue);
  }
  else
  {
    double oldMinValue = d->MinValue;
    double oldMaxValue = d->MaxValue;
    d->MinValue = newMinValue;
    d->MaxValue = newMaxValue;
    // don't emit a valuechanged signal if the new value is quite
    // similar to the old value.
    bool minChanged = qAbs(newMinValue - oldMinValue) > (d->SingleStep * 0.000000001);
    bool maxChanged = qAbs(newMaxValue - oldMaxValue) > (d->SingleStep * 0.000000001);
    if (minChanged || maxChanged)
    {
      emit this->valuesChanged(newMinValue, newMaxValue);
      if (minChanged)
      {
        emit this->minimumValueChanged(newMinValue);
      }
      if (maxChanged)
      {
        emit this->maximumValueChanged(newMaxValue);
      }
    }
  }
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSlider::singleStep() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->SingleStep;
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setSingleStep(double newStep)
{
  Q_D(ctkDoubleRangeSlider);
  d->SingleStep = newStep;
  // The following can fire A LOT of signals that shouldn't be
  // fired.
  bool oldBlockSignals = this->blockSignals(true);
  d->updateMinOffset(d->MinValue);
  d->updateMaxOffset(d->MaxValue);
  // update the new values of the ctkRangeSlider
  double _minvalue = d->MinValue;
  double _maxvalue = d->MaxValue;
  // calling setMinimum or setMaximum can change the values MinimumValue
  // and MaximumValue, this is why we re-set them later.
  this->setMinimum(d->Minimum);
  this->setMaximum(d->Maximum);
  this->setMinimumValue(_minvalue);
  this->setMinimumPosition(_minvalue);
  this->setMaximumValue(_maxvalue);
  this->setMaximumPosition(_maxvalue);
  this->blockSignals(oldBlockSignals);
}

// --------------------------------------------------------------------------
double ctkDoubleRangeSlider::tickInterval() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->SingleStep * d->Slider->tickInterval();
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setTickInterval(double newTickInterval)
{
  Q_D(ctkDoubleRangeSlider);
  d->Slider->setTickInterval(d->toInt(newTickInterval));
}

// --------------------------------------------------------------------------
QSlider::TickPosition ctkDoubleRangeSlider::tickPosition() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->Slider->tickPosition();
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setTickPosition(QSlider::TickPosition newTickPosition)
{
  Q_D(ctkDoubleRangeSlider);
  d->Slider->setTickPosition(newTickPosition);
}

// --------------------------------------------------------------------------
bool ctkDoubleRangeSlider::hasTracking() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->Slider->hasTracking();
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setTracking(bool enable)
{
  Q_D(ctkDoubleRangeSlider);
  d->Slider->setTracking(enable);
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::triggerAction(QAbstractSlider::SliderAction action)
{
  Q_D(ctkDoubleRangeSlider);
  d->Slider->triggerAction(action);
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setOrientation(Qt::Orientation newOrientation)
{
  Q_D(ctkDoubleRangeSlider);
  if (this->orientation() == newOrientation)
  {
    return;
  }
  if (!testAttribute(Qt::WA_WState_OwnSizePolicy))
  {
    QSizePolicy sp = this->sizePolicy();
    sp.transpose();
    this->setSizePolicy(sp);
    this->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
  }
  // d->Slider will take care of calling updateGeometry
  d->Slider->setOrientation(newOrientation);
}

// --------------------------------------------------------------------------
Qt::Orientation ctkDoubleRangeSlider::orientation() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->Slider->orientation();
}

// --------------------------------------------------------------------------
bool ctkDoubleRangeSlider::symmetricMoves() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->Slider->symmetricMoves();
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setSymmetricMoves(bool symmetry)
{
  Q_D(ctkDoubleRangeSlider);
  d->Slider->setSymmetricMoves(symmetry);
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::onValuesChanged(int newMinValue, int newMaxValue)
{
  Q_D(ctkDoubleRangeSlider);
  double doubleNewMinValue = d->safeMinFromInt(newMinValue);
  double doubleNewMaxValue = d->safeMaxFromInt(newMaxValue);

  bool emitMinValueChanged = (d->MinValue != doubleNewMinValue);
  bool emitMaxValueChanged = (d->MaxValue != doubleNewMaxValue);

  if (!emitMinValueChanged && !emitMaxValueChanged)
  {
    return;
  }
  d->MinValue = doubleNewMinValue;
  d->MaxValue = doubleNewMaxValue;
  emit this->valuesChanged(d->MinValue, d->MaxValue);
  if (emitMinValueChanged)
  {
    emit this->minimumValueChanged(d->MinValue);
  }
  if (emitMaxValueChanged)
  {
    emit this->maximumValueChanged(d->MaxValue);
  }
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::onMinPosChanged(int newPosition)
{
  Q_D(const ctkDoubleRangeSlider);
  emit this->minimumPositionChanged(d->safeMinFromInt(newPosition));
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::onMaxPosChanged(int newPosition)
{
  Q_D(const ctkDoubleRangeSlider);
  emit this->maximumPositionChanged(d->safeMaxFromInt(newPosition));
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::onPositionsChanged(int min, int max)
{
  Q_D(const ctkDoubleRangeSlider);
  emit this->positionsChanged(d->safeMinFromInt(min), d->safeMaxFromInt(max));
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::onRangeChanged(int min, int max)
{
  Q_D(const ctkDoubleRangeSlider);
  if (!d->SettingRange)
  {
    this->setRange(d->minFromInt(min), d->maxFromInt(max));
  }
}

// --------------------------------------------------------------------------
ctkRangeSlider* ctkDoubleRangeSlider::slider() const
{
  Q_D(const ctkDoubleRangeSlider);
  return d->Slider;
}

// --------------------------------------------------------------------------
void ctkDoubleRangeSlider::setSlider(ctkRangeSlider* newslider)
{
  Q_D(ctkDoubleRangeSlider);
  newslider->setOrientation(d->Slider->orientation());
  newslider->setMinimum(d->Slider->minimum());
  newslider->setMaximum(d->Slider->maximum());
  newslider->setValues(d->Slider->minimumValue(), d->Slider->maximumValue());
  newslider->setSingleStep(d->Slider->singleStep());
  newslider->setTracking(d->Slider->hasTracking());
  newslider->setTickInterval(d->Slider->tickInterval());
  newslider->setTickPosition(d->Slider->tickPosition());
  delete d->Slider;
  qobject_cast<QHBoxLayout*>(this->layout())->addWidget(newslider);
  d->Slider = newslider;
  d->connectSlider();
}
