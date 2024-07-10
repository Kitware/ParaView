// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimationTimeWidget_h
#define pqAnimationTimeWidget_h

#include "pqComponentsModule.h"
#include "pqDoubleLineEdit.h" // for pqDoubleLineEdit::RealNumberNotation.

#include <QList>
#include <QScopedPointer>
#include <QVariant>
#include <QWidget>
#include <vector>

class pqAnimationScene;
class vtkSMProxy;

/**
 * pqAnimationTimeWidget is a widget that can be used to show/set the current
 * animation time.
 * The widget allow the user to do the following:
 * \li View and/or change the current time value (in seq modes), or
 *     current time step value (in snap-to-timesteps mode).
 */
class PQCOMPONENTS_EXPORT pqAnimationTimeWidget : public QWidget
{
  Q_OBJECT
  Q_ENUMS(RealNumberNotation)
  Q_PROPERTY(QString playMode READ playMode WRITE setPlayMode NOTIFY playModeChanged)
  Q_PROPERTY(int numberOfFrames READ numberOfFrames WRITE setNumberOfFrames)
  Q_PROPERTY(double startTime READ startTime WRITE setStartTime)
  Q_PROPERTY(double endTime READ endTime WRITE setEndTime)
  Q_PROPERTY(QString timeLabel READ timeLabel WRITE setTimeLabel)
  Q_PROPERTY(QList<QVariant> timestepValues READ timestepValues WRITE setTimestepValues)

  typedef QWidget Superclass;

public:
  pqAnimationTimeWidget(QWidget* parent = nullptr);
  ~pqAnimationTimeWidget() override;
  using RealNumberNotation = pqDoubleLineEdit::RealNumberNotation;

  /**
   * Provides access to the animation scene proxy currently
   * controlled/reflected by this widget.
   */
  vtkSMProxy* animationScene() const;

  /**
   * Set the animation scene which is reflected/controlled by this widget.
   */
  void setAnimationScene(pqAnimationScene* animationScene);

  ///@{
  /**
   * Get/Set the list of timestep. `QList<QVariant>` is a list of variants
   * convertible to double.
   */
  void setTimestepValues(const QList<QVariant>& list);
  const QList<QVariant>& timestepValues() const;
  ///@}

  ///@{
  /**
   * Get/Set the animation start time.
   */
  void setStartTime(double start);
  double startTime() const;
  ///@}

  ///@{
  /**
   * Get/Set the animation end time.
   */
  void setEndTime(double end);
  double endTime() const;
  ///@}

  ///@{
  /**
   * Get/Set the number of frames.
   */
  void setNumberOfFrames(int nbOfFrame);
  int numberOfFrames() const;
  ///@}

  /**
   * Set the current animation time
   */
  void setCurrentTime(double t);

  /**
   * Return the notation used to display the number.
   * \sa setNotation()
   */
  RealNumberNotation notation() const;

  /**
   * Return the precision used to display the number.
   * \sa setPrecision()
   */
  int precision() const;

  ///@{
  /**
   * Get/set the animation playback mode.
   */
  void setPlayMode(const QString& mode);
  QString playMode() const;
  ///@}

  ///@{
  /**
   * Get/set the label text to use for the "time" parameter.
   */
  void setTimeLabel(const QString& val);
  QString timeLabel() const;
  ///@}

Q_SIGNALS:
  void playModeChanged();
  void dummySignal();

public Q_SLOTS:

  /**
   * Re render the widget.
   * This can be helpful when notation/precision settings have changed.
   */
  void render();

protected Q_SLOTS:
  /**
   * Update the current time in the widget GUI
   */
  void updateCurrentTime(double t);

private:
  Q_DISABLE_COPY(pqAnimationTimeWidget)

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
