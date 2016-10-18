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

#ifndef ctkDoubleRangeSlider_h
#define ctkDoubleRangeSlider_h

// Qt includes
#include <QSlider>
#include <QWidget>

// PQ includes
#include "pqWidgetsModule.h"

class ctkRangeSlider;
class ctkDoubleRangeSliderPrivate;

/**
* \ingroup Widgets
* ctkDoubleRangeSlider is a slider that controls 2 numbers as double.
* ctkDoubleRangeSlider internally aggregates a ctkRangeSlider (not in the
* API to prevent misuse). Only subclasses can have access to it.
* \sa ctkRangeSlider, ctkDoubleSlider, ctkRangeWidget
*/
class PQWIDGETS_EXPORT ctkDoubleRangeSlider : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
  Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
  Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
  Q_PROPERTY(double minimumValue READ minimumValue WRITE setMinimumValue)
  Q_PROPERTY(double maximumValue READ maximumValue WRITE setMaximumValue)
  Q_PROPERTY(double minimumPosition READ minimumPosition WRITE setMinimumPosition)
  Q_PROPERTY(double maximumPosition READ maximumPosition WRITE setMaximumPosition)
  Q_PROPERTY(bool tracking READ hasTracking WRITE setTracking)
  Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
  Q_PROPERTY(double tickInterval READ tickInterval WRITE setTickInterval)
  Q_PROPERTY(QSlider::TickPosition tickPosition READ tickPosition WRITE setTickPosition)
  Q_PROPERTY(bool symmetricMoves READ symmetricMoves WRITE setSymmetricMoves)
public:
  // Superclass typedef
  typedef QWidget Superclass;

  /**
  * Constructor, builds a ctkDoubleRangeSlider whose default values are the same
  * as ctkRangeSlider.
  */
  ctkDoubleRangeSlider(Qt::Orientation o, QWidget* par = 0);

  /**
  * Constructor, builds a ctkDoubleRangeSlider whose default values are the same
  * as ctkRangeSlider.
  */
  ctkDoubleRangeSlider(QWidget* par = 0);

  /**
  * Destructor
  */
  virtual ~ctkDoubleRangeSlider();

  /**
  *
  * This property holds the single step.
  * The smaller of two natural steps that an abstract sliders provides and
  * typically corresponds to the user pressing an arrow key
  */
  void setSingleStep(double ss);
  double singleStep() const;

  /**
  *
  * This property holds the interval between tickmarks.
  * This is a value interval, not a pixel interval. If it is 0, the slider
  * will choose between lineStep() and pageStep().
  * The default value is 0.
  */
  void setTickInterval(double ti);
  double tickInterval() const;

  /**
  *
  * This property holds the tickmark position for this slider.
  * The valid values are described by the QSlider::TickPosition enum.
  * The default value is QSlider::NoTicks.
  */
  void setTickPosition(QSlider::TickPosition position);
  QSlider::TickPosition tickPosition() const;

  /**
  *
  * This property holds the sliders's minimum value.
  * When setting this property, the maximum is adjusted if necessary to
  * ensure that the range remains valid. Also the slider's current values
  * are adjusted to be within the new range.
  */
  double minimum() const;
  void setMinimum(double min);

  /**
  *
  * This property holds the slider's maximum value.
  * When setting this property, the minimum is adjusted if necessary to
  * ensure that the range remains valid. Also the slider's current values
  * are adjusted to be within the new range.
  */
  double maximum() const;
  void setMaximum(double max);

  /**
  *
  * Sets the slider's minimum to min and its maximum to max.
  * If max is smaller than min, min becomes the only legal value.
  */
  void setRange(double min, double max);

  /**
  *
  * This property holds the slider's current minimum value.
  * The slider forces the minimum value to be within the legal range:
  * minimum <= minvalue <= maxvalue <= maximum.
  * Changing the minimumValue also changes the minimumPosition.
  */
  double minimumValue() const;

  /**
  *
  * This property holds the slider's current maximum value.
  * The slider forces the maximum value to be within the legal range:
  * minimum <= minvalue <= maxvalue <= maximum.
  * Changing the maximumValue also changes the maximumPosition.
  */
  double maximumValue() const;

  /**
  *
  * This property holds the current slider minimum position.
  * If tracking is enabled (the default), this is identical to minimumValue.
  */
  double minimumPosition() const;
  void setMinimumPosition(double minPos);

  /**
  *
  * This property holds the current slider maximum position.
  * If tracking is enabled (the default), this is identical to maximumValue.
  */
  double maximumPosition() const;
  void setMaximumPosition(double maxPos);

  /**
  *
  * Utility function that set the minimum position and
  * maximum position at once.
  */
  void setPositions(double minPos, double maxPos);

  /**
  *
  * This property holds whether slider tracking is enabled.
  * If tracking is enabled (the default), the slider emits the minimumValueChanged()
  * signal while the left/bottom handler is being dragged and the slider emits
  * the maximumValueChanged() signal while the right/top handler is being dragged.
  * If tracking is disabled, the slider emits the minimumValueChanged()
  * and maximumValueChanged() signals only when the user releases the slider.
  */
  void setTracking(bool enable);
  bool hasTracking() const;

  /**
  *
  * Triggers a slider action on the current slider. Possible actions are
  * SliderSingleStepAdd, SliderSingleStepSub, SliderPageStepAdd,
  * SliderPageStepSub, SliderToMinimum, SliderToMaximum, and SliderMove.
  */
  void triggerAction(QAbstractSlider::SliderAction action);

  /**
  *
  * This property holds the orientation of the slider.
  * The orientation must be Qt::Vertical (the default) or Qt::Horizontal.
  */
  Qt::Orientation orientation() const;
  void setOrientation(Qt::Orientation orientation);

  /**
  *
  * When symmetricMoves is true, moving a handle will move the other handle
  * symmetrically, otherwise the handles are independent. False by default
  */
  bool symmetricMoves() const;
  void setSymmetricMoves(bool symmetry);

Q_SIGNALS:
  /**
  *
  * This signal is emitted when the slider minimum value has changed,
  * with the new slider value as argument.
  */
  void minimumValueChanged(double minVal);

  /**
  *
  * This signal is emitted when the slider maximum value has changed,
  * with the new slider value as argument.
  */
  void maximumValueChanged(double maxVal);

  /**
  *
  * Utility signal that is fired when minimum or maximum values have changed.
  */
  void valuesChanged(double minVal, double maxVal);

  /**
  *
  * This signal is emitted when sliderDown is true and the slider moves.
  * This usually happens when the user is dragging the minimum slider.
  * The value is the new slider minimum position.
  * This signal is emitted even when tracking is turned off.
  */
  void minimumPositionChanged(double minPos);

  /**
  *
  * This signal is emitted when sliderDown is true and the slider moves.
  * This usually happens when the user is dragging the maximum slider.
  * The value is the new slider maximum position.
  * This signal is emitted even when tracking is turned off.
  */
  void maximumPositionChanged(double maxPos);

  /**
  *
  * Utility signal that is fired when minimum or maximum positions
  * have changed.
  */
  void positionsChanged(double minPos, double maxPos);

  /**
  *
  * This signal is emitted when the user presses one slider with the mouse,
  * or programmatically when setSliderDown(true) is called.
  */
  void sliderPressed();

  /**
  *
  * This signal is emitted when the user releases one slider with the mouse,
  * or programmatically when setSliderDown(false) is called.
  */
  void sliderReleased();

  /**
  *
  * This signal is emitted when the slider range has changed, with min being
  * the new minimum, and max being the new maximum.
  * Warning: don't confound with valuesChanged(double, double);
  * \sa QAbstractSlider::rangeChanged()
  */
  void rangeChanged(double min, double max);

public Q_SLOTS:
  /**
  *
  * This property holds the slider's current minimum value.
  * The slider forces the minimum value to be within the legal range:
  * minimum <= minvalue <= maxvalue <= maximum.
  * Changing the minimumValue also changes the minimumPosition.
  */
  void setMinimumValue(double minVal);

  /**
  *
  * This property holds the slider's current maximum value.
  * The slider forces the maximum value to be within the legal range:
  * minimum <= minvalue <= maxvalue <= maximum.
  * Changing the maximumValue also changes the maximumPosition.
  */
  void setMaximumValue(double maxVal);

  /**
  *
  * Utility function that set the minimum value and maximum value at once.
  */
  void setValues(double minVal, double maxVal);

protected Q_SLOTS:
  void onValuesChanged(int min, int max);

  void onMinPosChanged(int value);
  void onMaxPosChanged(int value);
  void onPositionsChanged(int min, int max);
  void onRangeChanged(int min, int max);

protected:
  ctkRangeSlider* slider() const;
  /**
  * Subclasses can change the internal slider
  */
  void setSlider(ctkRangeSlider* slider);

protected:
  QScopedPointer<ctkDoubleRangeSliderPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(ctkDoubleRangeSlider);
  Q_DISABLE_COPY(ctkDoubleRangeSlider)
};

#endif
