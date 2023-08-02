// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTimeKeeper_h
#define pqTimeKeeper_h

#include "pqProxy.h"
#include <QPair>

class vtkSMProxy;

/**
 * pqTimeKeeper is pqProxy for "TimeKeeper" proxy. A timekeeper is
 * created by default per connection. pqServer keeps a pointer to the
 * connection's time keeper. A time keeper keeps track of the
 * global time and timesteps available currently.
 */
class PQCORE_EXPORT pqTimeKeeper : public pqProxy
{
  Q_OBJECT
public:
  pqTimeKeeper(const QString& group, const QString& name, vtkSMProxy* timekeeper, pqServer* server,
    QObject* parent = nullptr);
  ~pqTimeKeeper() override;

  /**
   * Returns the number of timestep values known to this time keeper.
   */
  int getNumberOfTimeStepValues() const;

  /**
   * Returns the timestep value at the given index.
   * index < getNumberOfTimeStepValues().
   */
  double getTimeStepValue(int index) const;

  /**
   * Returns the maximum index in the timestep values for the given time for
   * which timestep value[index] <= time.
   */
  int getTimeStepValueIndex(double time) const;

  /**
   * Returns the available timesteps.
   */
  QList<double> getTimeSteps() const;

  /**
   * Returns the time range.
   * Return (0,0) is getNumberOfTimeStepValues() == 0.
   */
  QPair<double, double> getTimeRange() const;

  /**
   * Returns the current time.
   */
  double getTime() const;

  /**
   * Update the current time.
   */
  void setTime(double time);

Q_SIGNALS:
  /**
   * Fired when the keeper updates the times.
   */
  void timeStepsChanged();

  /**
   * Fired when the current time changes.
   */
  void timeChanged();

  /**
   * Fired when the time range changes.
   */
  void timeRangeChanged();

private:
  Q_DISABLE_COPY(pqTimeKeeper)

  class pqInternals;
  pqInternals* Internals;
};

#endif
