/*=========================================================================

   Program: ParaView
   Module:    pqChartAxisOptions.cxx

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

/// \file pqChartAxisOptions.cxx
/// \date 1/31/2007

#include "pqChartAxisOptions.h"

#include <math.h>


pqChartAxisOptions::pqChartAxisOptions(QObject *parentObject)
  : QObject(parentObject), AxisColor(Qt::black), GridColor(Qt::lightGray),
    LabelColor(Qt::black), LabelFont()
{
  this->Notation = pqChartValue::StandardOrExponential;
  this->GridType = pqChartAxisOptions::Lighter;
  this->Precision = 2;
  this->Visible = true;
  this->ShowLabels = true;
  this->ShowGrid = true;
}

pqChartAxisOptions::pqChartAxisOptions(const pqChartAxisOptions &other)
  : QObject(other.parent()), AxisColor(other.AxisColor),
    LabelColor(other.LabelColor), LabelFont(other.LabelFont)
{
  this->Notation = other.Notation;
  this->Precision = other.Precision;
  this->Visible = other.Visible;
  this->ShowLabels = other.ShowLabels;
  this->ShowGrid = other.ShowGrid;
}

void pqChartAxisOptions::setVisible(bool visible)
{
  if(this->Visible != visible)
    {
    this->Visible = visible;
    emit this->visibilityChanged();
    }
}

void pqChartAxisOptions::setLabelsVisible(bool visible)
{
  if(this->ShowLabels != visible)
    {
    this->ShowLabels = visible;
    emit this->visibilityChanged();
    }
}

void pqChartAxisOptions::setGridVisible(bool visible)
{
  if(this->ShowGrid != visible)
    {
    this->ShowGrid = visible;
    emit this->gridChanged();
    }
}

void pqChartAxisOptions::setAxisColor(const QColor &color)
{
  if(this->AxisColor != color)
    {
    this->AxisColor = color;
    emit this->colorChanged();
    }
}

void pqChartAxisOptions::setLabelColor(const QColor &color)
{
  if(this->LabelColor != color)
    {
    this->LabelColor = color;
    emit this->colorChanged();
    }
}

void pqChartAxisOptions::setLabelFont(const QFont &font)
{
  if(this->LabelFont != font)
    {
    this->LabelFont = font;
    emit this->fontChanged();
    }
}

void pqChartAxisOptions::setPrecision(int precision)
{
  if(this->Precision != precision)
    {
    this->Precision = precision;
    emit this->presentationChanged();
    }
}

void pqChartAxisOptions::setNotation(pqChartValue::NotationType notation)
{
  if(this->Notation != notation)
    {
    this->Notation = notation;
    emit this->presentationChanged();
    }
}

void pqChartAxisOptions::setGridColorType(
    pqChartAxisOptions::AxisGridColor type)
{
  if(this->GridType != type)
    {
    this->GridType = type;
    emit this->gridChanged();
    }
}

QColor pqChartAxisOptions::getGridColor() const
{
  if(this->GridType == pqChartAxisOptions::Lighter)
    {
    return pqChartAxisOptions::lighter(this->AxisColor);
    }

  return this->GridColor;
}

void pqChartAxisOptions::setGridColor(const QColor &color)
{
  if(this->GridColor != color)
    {
    this->GridColor = color;
    if(this->GridType == pqChartAxisOptions::Specified)
      {
      emit this->gridChanged();
      }
    }
}

pqChartAxisOptions &pqChartAxisOptions::operator=(
    const pqChartAxisOptions &other)
{
  this->Notation = other.Notation;
  this->AxisColor = other.AxisColor;
  this->LabelColor = other.LabelColor;
  this->LabelFont = other.LabelFont;
  this->Precision = other.Precision;
  this->Visible = other.Visible;
  this->ShowLabels = other.ShowLabels;
  return *this;
}

QColor pqChartAxisOptions::lighter(const QColor color, float factor)
{
  if(factor <= 0.0)
    {
    return color;
    }
  else if(factor >= 1.0)
    {
    return Qt::white;
    }

  // Find the distance between the current color and white.
  float r = color.red();
  float g = color.green();
  float b = color.blue();
  float d = sqrt(((255.0 - r) * (255.0 - r)) + ((255.0 - g) * (255.0 - g)) +
      ((255.0 - b) * (255.0 - b)));
  float f = factor * d;
  float s = d - f;

  // For a point on a line distance f from p1 and distance s
  // from p2, the equation is:
  // px = (fx2 + sx1)/(f + s)
  // py = (fy2 + sy1)/(f + s)
  // px = (fz2 + sz1)/(f + s)
  r = ((f * 255.0) + (s * r))/(d);
  g = ((f * 255.0) + (s * g))/(d);
  b = ((f * 255.0) + (s * b))/(d);
  return QColor((int)r, (int)g, (int)b);
}


