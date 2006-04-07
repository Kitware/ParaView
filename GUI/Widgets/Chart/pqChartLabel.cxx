/*=========================================================================

   Program:   ParaQ
   Module:    pqChartLabel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqChartLabel.h"
#include <QPainter>

pqChartLabel::pqChartLabel(QObject* p) :
  QObject(p),
  Color(Qt::black),
  Orientation(HORIZONTAL)
{
}

pqChartLabel::pqChartLabel(const QString& text, QObject* p) :
  QObject(p),
  Text(text),
  Color(Qt::black),
  Orientation(HORIZONTAL)
{
}

void pqChartLabel::setText(const QString& text)
{
  this->Text = text;
  emit layoutNeeded();
}

void pqChartLabel::setColor(const QColor& color)
{
  this->Color = color;
  emit repaintNeeded();
}

void pqChartLabel::setFont(const QFont& font)
{
  this->Font = font;
  emit layoutNeeded();
}

void pqChartLabel::setOrientation(const OrientationT orientation)
{
  this->Orientation = orientation;
  emit layoutNeeded();
}


const QRect pqChartLabel::getSizeRequest()
{
  if(this->Text.isEmpty())
    return QRect(0, 0, 0, 0);

  const QRect rect = QFontMetrics(this->Font).boundingRect(this->Text);

  switch(this->Orientation)
    {
    case HORIZONTAL:
      return QRect(0, 0, rect.width(), rect.height());
    case VERTICAL:
      return QRect(0, 0, rect.height(), rect.width());
    }
    
  return QRect(0, 0, 0, 0);
}

void pqChartLabel::setBounds(const QRect& bounds)
{
  this->Bounds = bounds;
  emit repaintNeeded();
}

const QRect pqChartLabel::getBounds() const
{
  return this->Bounds;
}

void pqChartLabel::draw(QPainter& painter, const QRect& /*area*/)
{
  if(this->Text.isEmpty())
    return;
  
  painter.save();
    painter.setRenderHint(QPainter::TextAntialiasing, false);
    painter.setFont(this->Font);
    painter.setPen(this->Color);
    switch(this->Orientation)
      {
      case HORIZONTAL:
        painter.drawText(this->Bounds, Qt::AlignCenter, this->Text);
        break;
      case VERTICAL:
        painter.translate(this->Bounds.left(), this->Bounds.bottom());
        painter.rotate(-90);
        painter.drawText(QRect(0, 0, this->Bounds.height(), this->Bounds.width()), Qt::AlignCenter, this->Text);
        break;
      }
  painter.restore();
}

