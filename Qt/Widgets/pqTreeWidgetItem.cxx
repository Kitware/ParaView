// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTreeWidgetItem.h"

// Server Manager Includes.

// Qt Includes.

// ParaView Includes.

//-----------------------------------------------------------------------------
void pqTreeWidgetItem::setData(int column, int role, const QVariant& v)
{
  QVariant curValue = this->data(column, role);
  if (this->CallbackHandler)
  {
    if (!this->CallbackHandler->acceptChange(this, curValue, v, column, role))
    {
      // reject the change.
      return;
    }
  }
  if (this->CallbackHandler)
  {
    this->CallbackHandler->dataAboutToChange(this, column, role);
    if (Qt::CheckStateRole == role)
    {
      this->CallbackHandler->checkStateAboutToChange(this, column);
    }
  }
  this->Superclass::setData(column, role, v);
  if (this->CallbackHandler)
  {
    if (Qt::CheckStateRole == role)
    {
      this->CallbackHandler->checkStateChanged(this, column);
    }
    this->CallbackHandler->dataChanged(this, column, role);
  }
}
