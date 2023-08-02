// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqHighlightablePushButton.h"

#include <QColor>
#include <QPalette>

class pqHighlightablePushButton::pqInternals
{
public:
  QPalette ResetPalette;
};

//-----------------------------------------------------------------------------
pqHighlightablePushButton::pqHighlightablePushButton(QWidget* parentA)
  : Superclass(parentA)
  , Internals(new pqHighlightablePushButton::pqInternals())
{
  this->Internals->ResetPalette = this->palette();
}

//-----------------------------------------------------------------------------
pqHighlightablePushButton::pqHighlightablePushButton(const QString& textA, QWidget* parentA)
  : Superclass(textA, parentA)
  , Internals(new pqHighlightablePushButton::pqInternals())
{
  this->Internals->ResetPalette = this->palette();
}

//-----------------------------------------------------------------------------
pqHighlightablePushButton::pqHighlightablePushButton(
  const QIcon& iconA, const QString& textA, QWidget* parentA)
  : Superclass(iconA, textA, parentA)
  , Internals(new pqHighlightablePushButton::pqInternals())
{
  this->Internals->ResetPalette = this->palette();
}

//-----------------------------------------------------------------------------
pqHighlightablePushButton::~pqHighlightablePushButton() = default;

//-----------------------------------------------------------------------------
void pqHighlightablePushButton::highlight(bool clearHighlight)
{
  QPalette buttonPalette = this->Internals->ResetPalette;
  if (clearHighlight == false)
  {
    buttonPalette.setColor(QPalette::Active, QPalette::Button, QColor(161, 213, 135));
    buttonPalette.setColor(QPalette::Inactive, QPalette::Button, QColor(161, 213, 135));
  }
  this->setPalette(buttonPalette);
}
