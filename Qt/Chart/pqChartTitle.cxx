/*=========================================================================

   Program: ParaView
   Module:    pqChartTitle.cxx

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

/// \file pqChartTitle.cxx
/// \date 11/17/2006

#include "pqChartTitle.h"

#include <QEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPrinter>


pqChartTitle::pqChartTitle(Qt::Orientation orient, QWidget *widgetParent)
  : QWidget(widgetParent), Text(), Bounds()
{
  this->Orient = orient;
  this->Align = Qt::AlignCenter;

  // Set up the default size policy.
  if(this->Orient == Qt::Horizontal)
    {
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
  else
    {
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
}

void pqChartTitle::setOrientation(Qt::Orientation orient)
{
  if(orient != this->Orient)
    {
    this->Orient = orient;
    if(this->Orient == Qt::Horizontal)
      {
      this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      }
    else
      {
      this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      }

    this->calculateSize();
    emit this->orientationChanged();
    }
}

void pqChartTitle::setText(const QString &text)
{
  if(text != this->Text)
    {
    this->Text = text;
    this->calculateSize();
    }
}

void pqChartTitle::drawTitle(QPainter &painter)
{
  QRect area;
  if(this->Orient == Qt::Vertical)
    {
    // Rotate the painter if the orientation is vertical.
    painter.translate(QPoint(0, this->height() - 1));
    painter.rotate(-90.0);

    // Set up the text area.
    if(this->height() - this->Bounds.height() < 0)
      {
      // TODO: Allow the user to move the drawing origin to see the
      // hidden parts of the text.
      area.setRect(0, 0, this->Bounds.height(), this->width());
      }
    else
      {
      area.setRect(0, 0, this->height(), this->width());
      }
    }
  else
    {
    // Set up the text area.
    if(this->width() - this->Bounds.width() < 0)
      {
      area.setRect(0, 0, this->Bounds.width(), this->height());
      }
    else
      {
      area.setRect(0, 0, this->width(), this->height());
      }
    }

  // If the painter is a printer, set the font.
  QPrinter *printer = dynamic_cast<QPrinter *>(painter.device());
  if(printer)
    {
    painter.setFont(QFont(this->font(), printer));
    }

  // Set up the painter and draw the text.
  painter.setPen(this->palette().color(QPalette::Text));
  painter.drawText(area, this->Align, this->Text);
}

bool pqChartTitle::event(QEvent *e)
{
  if(e->type() == QEvent::FontChange)
    {
    this->calculateSize();
    }

  return QWidget::event(e);
}

void pqChartTitle::paintEvent(QPaintEvent *e)
{
  if(this->Text.isEmpty() || !this->Bounds.isValid() || !e->rect().isValid())
    {
    return;
    }

  QPainter painter(this);
  this->drawTitle(painter);

  e->accept();
}

void pqChartTitle::calculateSize()
{
  // Use the font size and orientation to determine the size needed.
  QSize bounds;
  if(!this->Text.isEmpty())
    {
    QFontMetrics fm = this->fontMetrics();
    bounds.setWidth(fm.width(this->Text));
    bounds.setHeight(fm.height());
    if(this->Orient == Qt::Vertical)
      {
      bounds.transpose();
      }
    }

  // If the size has changed, update the layout.
  if(this->Bounds != bounds)
    {
    this->Bounds = bounds;
    this->updateGeometry();
    }
}


