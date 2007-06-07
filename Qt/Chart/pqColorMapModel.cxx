/*=========================================================================

   Program: ParaView
   Module:    pqColorMapModel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqColorMapModel.cxx
/// \date 3/9/2007

#include "pqColorMapModel.h"

#include "pqChartValue.h"
#include "pqChartPixelScale.h"

#include <QColor>
#include <QList>
#include <QPainter>
#include <QRect>
#include <QSize>


class pqColorMapModelItem
{
public:
  pqColorMapModelItem();
  pqColorMapModelItem(const pqChartValue &value, const QColor &color);
  pqColorMapModelItem(const pqChartValue &value, const QColor &color,
      const pqChartValue &opacity);
  ~pqColorMapModelItem() {}

  pqChartValue Value;
  QColor Color;
  pqChartValue Opacity;
};


class pqColorMapModelInternal : public QList<pqColorMapModelItem *> {};


//----------------------------------------------------------------------------
pqColorMapModelItem::pqColorMapModelItem()
  : Value(), Color()
{
}

pqColorMapModelItem::pqColorMapModelItem(const pqChartValue &value,
    const QColor &color)
  : Value(value), Color(color), Opacity((double)1.0)
{
}

pqColorMapModelItem::pqColorMapModelItem(const pqChartValue &value,
    const QColor &color, const pqChartValue &opacity)
  : Value(value), Color(color), Opacity(opacity)
{
}


//----------------------------------------------------------------------------
pqColorMapModel::pqColorMapModel(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqColorMapModelInternal();
  this->Space = pqColorMapModel::HsvSpace;
  this->InModify = false;
}

pqColorMapModel::pqColorMapModel(const pqColorMapModel &other)
  : QObject(0)
{
  this->Internal = new pqColorMapModelInternal();
  this->Space = other.Space;
  this->InModify = false;

  // Copy the list of points.
  QList<pqColorMapModelItem *>::ConstIterator iter = other.Internal->begin();
  for( ; iter != other.Internal->end(); ++iter)
    {
    this->Internal->append(new pqColorMapModelItem(
        (*iter)->Value, (*iter)->Color, (*iter)->Opacity));
    }
}

pqColorMapModel::~pqColorMapModel()
{
  this->InModify = true;
  this->removeAllPoints();
  delete this->Internal;
}

void pqColorMapModel::setColorSpace(pqColorMapModel::ColorSpace space)
{
  if(this->Space != space)
    {
    this->Space = space;
    if(!this->InModify)
      {
      emit this->colorSpaceChanged();
      }
    }
}

int pqColorMapModel::getColorSpaceAsInt() const
{
  switch(this->Space)
    {
    case pqColorMapModel::RgbSpace:
      return 0;
    case pqColorMapModel::WrappedHsvSpace:
      return 2;
    case pqColorMapModel::HsvSpace:
    default:
      return 1;
    }
}

void pqColorMapModel::setColorSpaceFromInt(int space)
{
  switch(space)
    {
    case 0:
      {
      this->setColorSpace(pqColorMapModel::RgbSpace);
      break;
      }
    case 1:
      {
      this->setColorSpace(pqColorMapModel::HsvSpace);
      break;
      }
    case 2:
      {
      this->setColorSpace(pqColorMapModel::WrappedHsvSpace);
      break;
      }
    }
}

int pqColorMapModel::getNumberOfPoints() const
{
  return this->Internal->size();
}

void pqColorMapModel::addPoint(const pqChartValue &value, const QColor &color)
{
  this->addPoint(value, color, pqChartValue((double)1.0));
}

void pqColorMapModel::addPoint(const pqChartValue &value, const QColor &color,
    const pqChartValue &opacity)
{
  // The list of points should be in ascending value order. Add the
  // new point according to its value.
  QList<pqColorMapModelItem *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter)
    {
    if(value == (*iter)->Value)
      {
      return;
      }
    else if(value < (*iter)->Value)
      {
      break;
      }
    }

  pqColorMapModelItem *item = new pqColorMapModelItem(value, color, opacity);
  if(iter == this->Internal->end())
    {
    // Add the point to the end of the list if it is greater than all
    // the current points.
    this->Internal->append(item);
    }
  else
    {
    this->Internal->insert(iter, item);
    }

  if(!this->InModify)
    {
    emit this->pointAdded(this->Internal->indexOf(item));
    }
}

void pqColorMapModel::removePoint(int index)
{
  if(index >= 0 && index < this->Internal->size())
    {
    if(!this->InModify)
      {
      emit this->removingPoint(index);
      }

    pqColorMapModelItem *item = this->Internal->takeAt(index);
    delete item;
    if(!this->InModify)
      {
      emit this->pointRemoved(index);
      }
    }
}

void pqColorMapModel::removeAllPoints()
{
  if(this->Internal->size() > 0)
    {
    QList<pqColorMapModelItem *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++ iter)
      {
      delete (*iter);
      }

    this->Internal->clear();
    if(!this->InModify)
      {
      emit this->pointsReset();
      }
    }
}

void pqColorMapModel::startModifyingData()
{
  this->InModify = true;
}

void pqColorMapModel::finishModifyingData()
{
  if(this->InModify)
    {
    this->InModify = false;
    emit this->pointsReset();
    }
}

void pqColorMapModel::getPointValue(int index, pqChartValue &value) const
{
  if(index >= 0 && index < this->Internal->size())
    {
    value = (*this->Internal)[index]->Value;
    }
}

void pqColorMapModel::setPointValue(int index, const pqChartValue &value)
{
  if(index >= 0 && index < this->Internal->size() &&
      (*this->Internal)[index]->Value != value)
    {
    (*this->Internal)[index]->Value = value;
    if(!this->InModify)
      {
      emit this->valueChanged(index, value);
      }
    }
}

void pqColorMapModel::getPointColor(int index, QColor &color) const
{
  if(index >= 0 && index < this->Internal->size())
    {
    color = (*this->Internal)[index]->Color;
    }
}

void pqColorMapModel::setPointColor(int index, const QColor &color)
{
  if(index >= 0 && index < this->Internal->size() &&
      (*this->Internal)[index]->Color != color)
    {
    (*this->Internal)[index]->Color = color;
    if(!this->InModify)
      {
      emit this->colorChanged(index, color);
      }
    }
}

void pqColorMapModel::getPointOpacity(int index, pqChartValue &opacity) const
{
  if(index >= 0 && index < this->Internal->size())
    {
    opacity = (*this->Internal)[index]->Opacity;
    }
}

void pqColorMapModel::setPointOpacity(int index, const pqChartValue &opacity)
{
  if(index >= 0 && index < this->Internal->size() &&
      (*this->Internal)[index]->Opacity != opacity)
    {
    (*this->Internal)[index]->Opacity = opacity;
    if(!this->InModify)
      {
      emit this->opacityChanged(index, opacity);
      }
    }
}

bool pqColorMapModel::isRangeNormalized() const
{
  if(this->Internal->size() > 1)
    {
    return this->Internal->first()->Value == (float)0.0 &&
        this->Internal->last()->Value == (float)1.0;
    }

  return false;
}

void pqColorMapModel::getValueRange(pqChartValue &min, pqChartValue &max) const
{
  if(this->Internal->size() > 0)
    {
    min = this->Internal->first()->Value;
    max = this->Internal->last()->Value;
    }
}

void pqColorMapModel::setValueRange(const pqChartValue &min,
    const pqChartValue &max)
{
  if(this->Internal->size() == 0)
    {
    return;
    }

  // Scale the current points to fit the given range.
  if(this->Internal->size() == 1)
    {
    this->Internal->first()->Value = min;
    }
  else
    {
    pqChartValue newMin, newRange;
    pqChartValue oldMin = this->Internal->first()->Value;
    pqChartValue oldRange = this->Internal->last()->Value - oldMin;
    if(max < min)
      {
      newMin = max;
      newRange = min - max;
      }
    else
      {
      newMin = min;
      newRange = max - min;
      }

    QList<pqColorMapModelItem *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      (*iter)->Value = (((*iter)->Value - oldMin) * newRange) / oldRange;
      (*iter)->Value += newMin;
      }
    }

  if(!this->InModify)
    {
    emit this->pointsReset();
    }
}

QPixmap pqColorMapModel::generateGradient(const QSize &size) const
{
  if(this->Internal->size() < 2 || size.width() <= 0 || size.height() <= 0)
    {
    return QPixmap();
    }

  // Create a pixmap and painter.
  QPixmap gradient(size);
  QPainter painter(&gradient);

  // Set up the pixel-value map for the image size.
  pqChartPixelScale pixelMap;
  pixelMap.setPixelRange(1, size.width() - 1);
  pixelMap.setValueRange(this->Internal->first()->Value,
      this->Internal->last()->Value);

  // Draw the first color.
  int i = 0;
  QColor next, previous;
  QList<pqColorMapModelItem *>::Iterator iter = this->Internal->begin();
  previous = (*iter)->Color;
  int imageHeight = gradient.height();
  painter.setPen(previous);
  painter.drawLine(0, 0, 0, imageHeight);

  // Loop through the points to draw the gradient(s).
  int px = 1;
  int p1 = pixelMap.getPixelFor((*iter)->Value);
  for(++i, ++iter; iter != this->Internal->end(); ++i, ++iter)
    {
    // Draw the colors between the previous and next color.
    next = (*iter)->Color;
    int p2 = pixelMap.getPixelFor((*iter)->Value);
    int w = p2 - p1;
    if(w > 0)
      {
      int x1 = px - 1;
      int x2 = x1 + w;
      for( ; px <= x2; px++)
        {
        // Use rgb or hsv space depending on the user option.
        if(px == x2)
          {
          painter.setPen(next);
          }
        else if(this->Space == pqColorMapModel::RgbSpace)
          {
          // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
          int r=0, g=0, b=0;
          r = ((px - x1)*(next.red() - previous.red()))/w + previous.red();
          g = ((px - x1)*(next.green() - previous.green()))/w +
              previous.green();
          b = ((px - x1)*(next.blue() - previous.blue()))/w + previous.blue();
          painter.setPen(QColor(r, g, b));
          }
        else if(this->Space == pqColorMapModel::HsvSpace ||
            this->Space == pqColorMapModel::WrappedHsvSpace)
          {
          // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
          int s=0, v=0;
          int h = next.hue();
          int h1 = previous.hue();
          if(this->Space == pqColorMapModel::WrappedHsvSpace &&
              (h - h1 > 180 || h1 - h > 180))
            {
            if(h1 > h)
              {
              h1 -= 360;
              }
            else
              {
              h -= 360;
              }
            }

          h = ((px - x1)*(h - h1))/w + h1;
          if(h < 0)
            {
            h += 360;
            }

          s = ((px - x1)*(next.saturation() - previous.saturation()))/w +
              previous.saturation();
          v = ((px - x1)*(next.value() - previous.value()))/w + previous.value();
          painter.setPen(QColor::fromHsv(h, s, v));
          }

        painter.drawLine(px, 0, px, imageHeight);
        }
      }

    previous = next;
    p1 = p2;
    }

  // Make sure the last pixel is drawn.
  if(px < pixelMap.getMaxPixel())
    {
    painter.drawLine(px, 0, px, imageHeight);
    }

  // Finally, add a border to the gradient.
  QRect border(0, 0, size.width() - 1, size.height() - 1);
  painter.setPen(QColor(100, 100, 100));
  painter.drawRect(border);

  return gradient;
}

pqColorMapModel &pqColorMapModel::operator=(const pqColorMapModel &other)
{
  this->Space = other.Space;

  // Remove the current points and copy the new points.
  bool oldModify = this->InModify;
  this->InModify = false;
  this->removeAllPoints();
  this->InModify = oldModify;
  QList<pqColorMapModelItem *>::ConstIterator iter = other.Internal->begin();
  for( ; iter != other.Internal->end(); ++iter)
    {
    this->Internal->append(new pqColorMapModelItem(
        (*iter)->Value, (*iter)->Color, (*iter)->Opacity));
    }

  if(!this->InModify)
    {
    emit this->pointsReset();
    }

  return *this;
}


