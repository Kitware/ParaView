// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAxesToolbar_h
#define pqAxesToolbar_h

#include "pqApplicationComponentsModule.h"
#include <QToolBar>

class pqView;

/**
 * pqAxesToolbar is the toolbar that has buttons for setting the center
 * rotation axes, toggling its visibility etc.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAxesToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqAxesToolbar(const QString& title, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  pqAxesToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
  {
    this->constructor();
  }
  ~pqAxesToolbar() override;

protected Q_SLOTS:
  void setView(pqView* view);
  void updateEnabledState();
  void showCenterAxes(bool);
  void showOrientationAxes(bool);
  void resetCenterOfRotationToCenterOfCurrentData();
  void pickCenterOfRotation(int, int);

private:
  Q_DISABLE_COPY(pqAxesToolbar)

  class pqInternals;
  pqInternals* Internals;
  void constructor();
};

#endif
