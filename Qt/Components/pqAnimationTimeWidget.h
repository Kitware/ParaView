/*=========================================================================

   Program: ParaView
   Module:  pqAnimationTimeWidget.h

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
#ifndef pqAnimationTimeWidget_h
#define pqAnimationTimeWidget_h

#include "pqComponentsModule.h"

#include <QScopedPointer>
#include <QWidget>
#include <vector>

class vtkSMProxy;

/**
* pqAnimationTimeWidget is a widget that can be used to show/set the current
* animation time.
* The widget allow the user to do the following:
* \li View and/or change the current time value (in seq/realtime modes), or
*     current time step value (in snap-to-timesteps mode).
* \li View and/or change the play mode (from seq to snap-to-timesteps). While
*     the widget behaves acceptably if the application externally changes the
*     animation play mode to realtime, the widget itself doesn't allow the
*     user to do that. This mode is optional. You can disabling allowing the
*     user to change the play mode by setting playModeReadOnly to true
*     (default is false).
*/
class PQCOMPONENTS_EXPORT pqAnimationTimeWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(double timeValue READ timeValue WRITE setTimeValue NOTIFY timeValueChanged)
  Q_PROPERTY(int timeStepCount READ timeStepCount WRITE setTimeStepCount)
  Q_PROPERTY(QString playMode READ playMode WRITE setPlayMode)
  Q_PROPERTY(bool playModeReadOnly READ playModeReadOnly WRITE setPlayModeReadOnly)
  Q_PROPERTY(QString timeLabel READ timeLabel WRITE setTimeLabel)

  typedef QWidget Superclass;

public:
  pqAnimationTimeWidget(QWidget* parent = 0);
  ~pqAnimationTimeWidget() override;

  /**
  * Provides access to the animation scene proxy currently
  * controlled/reflected by this widget.
  */
  vtkSMProxy* animationScene() const;

  /**
  * Get/set the current time value.
  */
  void setTimeValue(double time);
  double timeValue() const;

  /**
  * Get/set the precision with which time is reported.
  */
  void setTimePrecision(int val);
  int timePrecision() const;

  /**
   * Get/set the precision with which time is reported.
   */
  void setTimeNotation(const QChar& val);
  QChar timeNotation() const;

  /**
  * Get/set the number of timesteps.
  */
  void setTimeStepCount(int count);
  int timeStepCount() const;

  /**
  * Get/set the playmode.
  */
  void setPlayMode(const QString& mode);
  QString playMode() const;

  /**
  * Get/set whether the user should be able to change the animation
  * play mode using this widget.
  */
  void setPlayModeReadOnly(bool val);
  bool playModeReadOnly() const;

  /**
  * Get/set the label text to use for the "time" parameter.
  */
  void setTimeLabel(const QString& val);
  QString timeLabel() const;
signals:
  void timeValueChanged();
  void playModeChanged();
  void dummySignal();

public slots:
  /**
  * Set the animation scene proxy which is reflected/controlled by this
  * widget.
  */
  void setAnimationScene(vtkSMProxy* animationScene);

  /**
   * Clear the combobox and fill it with current time values.
   */
  void repopulateTimeComboBox();

protected:
  /**
   * Format a double with the requested notation and precision.
   */
  QString formatDouble(double value);

private slots:
  void updateTimestepCountLabelVisibility();

  /**
  * called when the user changes the timestepValue spinbox manually
  * to change the current timestep. We will update the current time and
  * result in triggering  timeValueChanged() if time indeed changed.
  */
  void timeSpinBoxChanged();

  /**
   * called when user changes the time value in the line edit.
   * we update the internal "full precision" time value and fire
   * `timeValueChanged`.
   */
  void timeLineEditChanged();

  /**
   * called when user changes the time value in the combobox.
   * we update the internal "full precision" time value and fire
   * `timeValueChanged`.
   */
  void timeComboBoxChanged();

  /**
   * called when user toggles the radio button.
   * we switch playmode and fire `playModeChanged`
   */
  void timeRadioButtonToggled();

private:
  Q_DISABLE_COPY(pqAnimationTimeWidget)

  // Returns the current timekeeper.
  vtkSMProxy* timeKeeper() const;

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
