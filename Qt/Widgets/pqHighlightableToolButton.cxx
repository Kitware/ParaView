// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqHighlightableToolButton.h"

#include <iostream>

//-----------------------------------------------------------------------------
pqHighlightableToolButton::pqHighlightableToolButton(QWidget* parentObject)
  : Superclass(parentObject)
  , ResetPalette(this->palette())
{
}

//-----------------------------------------------------------------------------
pqHighlightableToolButton::~pqHighlightableToolButton() = default;

//-----------------------------------------------------------------------------
void pqHighlightableToolButton::highlight(bool clearHighlight)
{
  QPalette buttonPalette = this->ResetPalette;
  if (clearHighlight == false)
  {
    buttonPalette.setColor(QPalette::Active, QPalette::Button, QColor(161, 213, 135));
    buttonPalette.setColor(QPalette::Inactive, QPalette::Button, QColor(161, 213, 135));
  }
  this->setPalette(buttonPalette);
}
