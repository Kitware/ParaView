/*=========================================================================

   Program: ParaView
   Module:  pqColorChooserButton.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

// self includes
#include "pqColorChooserButton.h"

#include "cassert"

// Qt includes
#include <QColorDialog>
#include <QPainter>
#include <QResizeEvent>

//-----------------------------------------------------------------------------
pqColorChooserButton::pqColorChooserButton(QWidget* p)
  : QToolButton(p)
  , ShowAlphaChannel(false)
{
  this->Color[0] = 0.0;
  this->Color[1] = 0.0;
  this->Color[2] = 0.0;
  this->Color[3] = 1.0;
  this->IconRadiusHeightRatio = 0.75;
  this->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  this->connect(this, SIGNAL(clicked()), SLOT(chooseColor()));
}

//-----------------------------------------------------------------------------
QColor pqColorChooserButton::chosenColor() const
{
  QColor color;
  color.setRgbF(this->Color[0], this->Color[1], this->Color[2], this->Color[3]);

  return color;
}

//-----------------------------------------------------------------------------
QVariantList pqColorChooserButton::chosenColorRgbF() const
{
  QVariantList val;
  val << this->Color[0] << this->Color[1] << this->Color[2];
  return val;
}

//-----------------------------------------------------------------------------
QVariantList pqColorChooserButton::chosenColorRgbaF() const
{
  QVariantList val;
  val << this->Color[0] << this->Color[1] << this->Color[2] << this->Color[3];
  return val;
}

//-----------------------------------------------------------------------------
void pqColorChooserButton::setChosenColor(const QColor& color)
{
  if (color.isValid())
  {
    QVariantList val;
    val << color.redF() << color.greenF() << color.blueF() << color.alphaF();
    this->setChosenColorRgbaF(val);
  }
}

//-----------------------------------------------------------------------------
void pqColorChooserButton::setChosenColorRgbF(const QVariantList& val)
{
  assert(val.size() == 3);
  QColor color;
  color.setRgbF(val[0].toDouble(), val[1].toDouble(), val[2].toDouble());

  if (color.isValid())
  {
    if (val[0].toDouble() != this->Color[0] || val[1].toDouble() != this->Color[1] ||
      val[2].toDouble() != this->Color[2])
    {
      this->Color[0] = val[0].toDouble();
      this->Color[1] = val[1].toDouble();
      this->Color[2] = val[2].toDouble();

      this->setIcon(this->renderColorSwatch(color));
      Q_EMIT this->chosenColorChanged(color);
    }

    Q_EMIT this->validColorChosen(color);
  }
}

//-----------------------------------------------------------------------------
void pqColorChooserButton::setChosenColorRgbaF(const QVariantList& val)
{
  assert(val.size() == 4);
  QColor color;
  color.setRgbF(val[0].toDouble(), val[1].toDouble(), val[2].toDouble(), val[3].toDouble());

  if (color.isValid())
  {
    if (val[0].toDouble() != this->Color[0] || val[1].toDouble() != this->Color[1] ||
      val[2].toDouble() != this->Color[2] || val[3].toDouble() != this->Color[3])
    {
      this->Color[0] = val[0].toDouble();
      this->Color[1] = val[1].toDouble();
      this->Color[2] = val[2].toDouble();
      this->Color[3] = val[3].toDouble();

      this->setIcon(this->renderColorSwatch(color));
      Q_EMIT this->chosenColorChanged(color);
    }
    Q_EMIT this->validColorChosen(color);
  }
}

//-----------------------------------------------------------------------------
QIcon pqColorChooserButton::renderColorSwatch(const QColor& color)
{
  int radius = qRound(this->height() * this->IconRadiusHeightRatio);
  if (radius <= 10)
  {
    radius = 10;
  }

  QPixmap pix(radius, radius);
  pix.fill(QColor(0, 0, 0, 0));

  QPainter painter(&pix);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setBrush(QBrush(color));
  painter.drawEllipse(1, 1, radius - 2, radius - 2);
  painter.end();
  QIcon ret(pix);

  QPixmap pix2x(radius * 2, radius * 2);
  // Add a high-dpi version, just like a @2x.png file
  pix2x.setDevicePixelRatio(2.0);
  pix2x.fill(QColor(0, 0, 0, 0));

  QPainter painter2x(&pix2x);
  painter2x.setRenderHint(QPainter::Antialiasing, true);
  painter2x.setBrush(QBrush(color));
  painter2x.drawEllipse(2, 2, radius - 4, radius - 4);
  painter2x.end();

  ret.addPixmap(pix2x);
  return ret;
}

//-----------------------------------------------------------------------------
void pqColorChooserButton::chooseColor()
{
  QColorDialog::ColorDialogOptions opts;
  if (this->ShowAlphaChannel)
  {
    opts |= QColorDialog::ShowAlphaChannel;
  }

  this->setChosenColor(QColorDialog::getColor(this->chosenColor(), this, "Select Color", opts));
}

//-----------------------------------------------------------------------------
void pqColorChooserButton::resizeEvent(QResizeEvent* rEvent)
{
  (void)rEvent;

  QColor color = this->chosenColor();
  this->setIcon(this->renderColorSwatch(color));
}
