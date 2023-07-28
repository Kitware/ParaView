// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSpinBox.h"

//-----------------------------------------------------------------------------
pqSpinBox::pqSpinBox(QWidget* _parent)
  : Superclass(_parent)
  , EditingFinishedPending(false)
{
  this->connect(this, SIGNAL(editingFinished()), SLOT(onEditingFinished()));
  this->connect(this, SIGNAL(valueChanged(int)), SLOT(onValueEdited()));
}

//-----------------------------------------------------------------------------
void pqSpinBox::stepBy(int steps)
{
  int old_value = this->value();
  this->Superclass::stepBy(steps);

  if (this->value() != old_value)
  {
    Q_EMIT this->editingFinished();
  }
}

//-----------------------------------------------------------------------------
void pqSpinBox::onValueEdited()
{
  this->EditingFinishedPending = true;
}

//-----------------------------------------------------------------------------
void pqSpinBox::onEditingFinished()
{
  if (this->EditingFinishedPending)
  {
    Q_EMIT this->valueChangedAndEditingFinished();
    this->EditingFinishedPending = false;
  }
}
