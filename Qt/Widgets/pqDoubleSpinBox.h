// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDoubleSpinBox_h
#define pqDoubleSpinBox_h

#include "pqWidgetsModule.h"
#include <QDoubleSpinBox>

/**
 * QDoubleSpinBox which fires editingFinished() signal when the value is changed
 * by steps (increments).
 */
class PQWIDGETS_EXPORT pqDoubleSpinBox : public QDoubleSpinBox
{
  Q_OBJECT
  typedef QDoubleSpinBox Superclass;

public:
  explicit pqDoubleSpinBox(QWidget* parent = nullptr);

  /**
   * Virtual function that is called whenever the user triggers a step.  We are
   * overriding this so that we can emit editingFinished() signal
   */
  void stepBy(int steps) override;

private:
  Q_DISABLE_COPY(pqDoubleSpinBox)
};

#endif
