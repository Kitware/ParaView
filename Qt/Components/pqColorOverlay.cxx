// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorOverlay.h"

#include <QPainter>

//-----------------------------------------------------------------------------
pqColorOverlay::pqColorOverlay(QWidget* parent)
  : QWidget(parent)
{
  setAttribute(Qt::WA_NoSystemBackground);
  setAttribute(Qt::WA_TransparentForMouseEvents);
}

//-----------------------------------------------------------------------------
QColor pqColorOverlay::rgb() const
{
  return Rgba;
}

//-----------------------------------------------------------------------------
void pqColorOverlay::setRgb(int r, int g, int b)
{
  Rgba.setRgb(r, g, b);
  this->repaint();
}

//-----------------------------------------------------------------------------
void pqColorOverlay::setRgb(QColor rgb)
{
  Rgba.setRgb(rgb.red(), rgb.green(), rgb.blue());
  this->repaint();
}

//-----------------------------------------------------------------------------
int pqColorOverlay::opacity() const
{
  return Rgba.alpha();
}

//-----------------------------------------------------------------------------
void pqColorOverlay::setOpacity(int opacity)
{
  Rgba.setAlpha(opacity);
  this->repaint();
}

//-----------------------------------------------------------------------------
void pqColorOverlay::paintEvent(QPaintEvent*)
{
  QPainter(this).fillRect(rect(), QBrush{ Rgba });
}
