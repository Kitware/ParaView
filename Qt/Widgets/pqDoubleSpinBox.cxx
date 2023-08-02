// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDoubleSpinBox.h"

//-----------------------------------------------------------------------------
pqDoubleSpinBox::pqDoubleSpinBox(QWidget* _parent)
  : QDoubleSpinBox(_parent)
{
}

//-----------------------------------------------------------------------------
void pqDoubleSpinBox::stepBy(int steps)
{
  double old_value = this->value();
  this->Superclass::stepBy(steps);

  if (this->value() != old_value)
  {
    Q_EMIT this->editingFinished();
  }
}
