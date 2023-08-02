// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqVCRToolbar_h
#define pqVCRToolbar_h

#include "pqApplicationComponentsModule.h"
#include <QToolBar>

class pqVCRController;

/**
 * pqVCRToolbar is the toolbar with VCR controls.
 * Simply instantiate this and put it in your application UI file or
 * QMainWindow to use it.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqVCRToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqVCRToolbar(const QString& title, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  pqVCRToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
  {
    this->constructor();
  }
  ~pqVCRToolbar() override;

protected Q_SLOTS:
  void setTimeRanges(double, double);
  void onPlaying(bool, bool);

private:
  Q_DISABLE_COPY(pqVCRToolbar)

  void constructor();

  class pqInternals;
  pqInternals* UI;

  pqVCRController* Controller;
};

#endif
