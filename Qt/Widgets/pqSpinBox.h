// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpinBox_h
#define pqSpinBox_h

#include "pqWidgetsModule.h"
#include <QSpinBox>

/**
 * QSpinBox which fires editingFinished() signal when the value is changed
 * by steps (increments).
 * Also, this adds a new signal valueChangedAndEditingFinished() which is fired
 * after editingFinished() signal is fired and the value in the spin box indeed
 * changed.
 */
class PQWIDGETS_EXPORT pqSpinBox : public QSpinBox
{
  Q_OBJECT
  typedef QSpinBox Superclass;

public:
  explicit pqSpinBox(QWidget* parent = nullptr);

  /**
   * Virtual function that is called whenever the user triggers a step.  We are
   * overriding this so that we can emit editingFinished() signal
   */
  void stepBy(int steps) override;

Q_SIGNALS:
  /**
   * Unlike QSpinBox::editingFinished() which gets fired whenever the widget
   * looses focus irrespective of if the value was indeed edited,
   * valueChangedAndEditingFinished() is fired only when the value was changed
   * as well.
   */
  void valueChangedAndEditingFinished();

private Q_SLOTS:
  void onValueEdited();
  void onEditingFinished();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqSpinBox)
  bool EditingFinishedPending;
};

#endif
