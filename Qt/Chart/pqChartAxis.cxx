/*=========================================================================

   Program: ParaView
   Module:    pqChartAxis.cxx

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

/*!
 * \file pqChartAxis.cxx
 *
 * \brief
 *   The pqChartAxis class is used to display a chart axis.
 *
 * \author Mark Richardson
 * \date   May 12, 2005
 */

#include "pqChartAxis.h"
#include "pqChartLabel.h"
#include <QPainter>
#include <QFontMetrics>

#include <vtkstd/vector>
#include <math.h>

#define TICK_LENGTH_SMALL 3
#define TICK_LENGTH 5
#define TICK_MARGIN 8
#define LABEL_MARGIN 10


class pqChartAxisPair
{
public:
  pqChartAxisPair();
  ~pqChartAxisPair() {}

public:
  pqChartValue Value;
  int Pixel;
};


class pqChartAxisData : public vtkstd::vector<pqChartAxisPair *> {};


// The interval list is used to determine a suitable interval
// for a shifting axis.
static pqChartValue IntervalList[] = {
    pqChartValue((float)1.0),
    pqChartValue((float)2.0),
    pqChartValue((float)2.5),
    pqChartValue((float)5.0)};
static int IntervalListLength = 4;

static double MinIntLogPower = -1;
double pqChartAxis::MinLogValue = 0.0001;


pqChartAxisPair::pqChartAxisPair()
  : Value()
{
  this->Pixel = 0;
}


pqChartAxis::pqChartAxis(AxisLocation location, QObject *p)
  : QObject(p), Bounds(), AxisColor(Qt::black), GridColor(178, 178, 178),
    TickLabelColor(Qt::black), TickLabelFont(), ValueMin(), ValueMax(),
    TrueMin(), TrueMax()
{
  this->Location = location;
  this->Scale = pqChartAxis::Linear;
  this->Layout = pqChartAxis::BestInterval;
  this->GridType = pqChartAxis::Lighter;
  this->Label = new pqChartLabel();
  this->Data = new pqChartAxisData();
  this->AtMin = 0;
  this->AtMax = 0;
  this->Intervals = 0;
  this->PixelMin = 0;
  this->PixelMax = 0;
  this->Precision = 2;
  this->WidthMax = 0;
  this->Count = 0;
  this->Skip = 1;
  this->Visible = true;
  this->GridVisible = true;
  this->ExtraMaxPadding = false;
  this->ExtraMinPadding = false;
  this->Notation = 0;
}

pqChartAxis::~pqChartAxis()
{
  delete this->Label;
  
  if(this->Data)
    {
    this->cleanData();
    delete this->Data;
    }
}

void pqChartAxis::setValueRange(const pqChartValue &min,
    const pqChartValue &max)
{
  this->ValueMin = min;
  this->ValueMax = max;
  if(this->Scale == pqChartAxis::Logarithmic)
    {
    // A logarithmic scale axis cannot contain zero because it is
    // undefined. If the range includes zero, set the scale to linear.
    if((min < 0 && max > 0) || (max < 0 && min > 0))
      {
      this->Scale = pqChartAxis::Linear;
      }
    }

  if(this->Scale == pqChartAxis::Logarithmic)
    {
    if(max < min)
      {
      this->ValueMin = max;
      this->ValueMax = min;
      }

    // Adjust the values that are close to zero if they are
    // below the minimum log value.
    if(this->ValueMin < 0)
      {
      if(this->ValueMax.getType() != pqChartValue::IntValue &&
          this->ValueMax > -pqChartAxis::MinLogValue)
        {
        this->ValueMax = -pqChartAxis::MinLogValue;
        if(this->ValueMin.getType() != pqChartValue::DoubleValue)
          {
          this->ValueMax.convertTo(pqChartValue::FloatValue);
          }
        }
      }
    else
      {
      if(this->ValueMin.getType() != pqChartValue::IntValue &&
          this->ValueMin < pqChartAxis::MinLogValue)
        {
        this->ValueMin = pqChartAxis::MinLogValue;
        if(this->ValueMax.getType() != pqChartValue::DoubleValue)
          {
          this->ValueMin.convertTo(pqChartValue::FloatValue);
          }
        }
      }
    }

  this->TrueMin = this->ValueMin;
  this->TrueMax = this->ValueMax;
  this->calculateMaxWidth();
}

pqChartValue pqChartAxis::getValueRange() const
{
  return this->ValueMax - this->ValueMin;
}

void pqChartAxis::setMinValue(const pqChartValue &min)
{
  this->setValueRange(min, this->TrueMax);
}

void pqChartAxis::setMaxValue(const pqChartValue &max)
{
  this->setValueRange(this->TrueMin, max);
}

int pqChartAxis::getPixelRange() const
{
  if(this->PixelMax > this->PixelMin)
    return this->PixelMax - this->PixelMin;
  else
    return this->PixelMin - this->PixelMax;
}

int pqChartAxis::getPixelFor(const pqChartValue &value) const
{
  // Convert the value to a pixel location using:
  // px = ((vx - v1)*(p2 - p1))/(v2 - v1) + p1
  // If using a log scale, the values should be in exponents in
  // order to get a linear mapping.
  pqChartValue result;
  pqChartValue valueRange;
  if(this->Scale == pqChartAxis::Logarithmic)
    {
    // If the value is less than the minimum log number, return
    // the minimum pixel value.
    bool reversed = this->TrueMin < 0;
    if(reversed)
      {
      if(value >= -pqChartAxis::MinLogValue)
        {
        return this->PixelMax;
        }
      }
    else
      {
      if(value <= pqChartAxis::MinLogValue)
        {
        return this->PixelMin;
        }
      }

    // If the log scale uses integers, the first value may be zero.
    // In that case, use -1 instead of taking the log of zero.
    pqChartValue v1;
    if(this->TrueMin.getType() == pqChartValue::IntValue &&
        this->ValueMin == 0)
      {
      v1 = MinIntLogPower;
      }
    else
      {
      if(reversed)
        {
        v1 = log10(-this->ValueMin.getDoubleValue());
        }
      else
        {
        v1 = log10(this->ValueMin.getDoubleValue());
        }
      }

    if(this->TrueMin.getType() == pqChartValue::IntValue &&
        this->ValueMax == 0)
      {
      valueRange = MinIntLogPower;
      }
    else
      {
      if(reversed)
        {
        valueRange = log10(-this->ValueMax.getDoubleValue());
        }
      else
        {
        valueRange = log10(this->ValueMax.getDoubleValue());
        }
      }

    if(reversed)
      {
      result = log10(-value.getDoubleValue());
      }
    else
      {
      result = log10(value.getDoubleValue());
      }

    result -= v1;
    valueRange -= v1;
    }
  else
    {
    result = value - this->ValueMin;
    valueRange = this->ValueMax - this->ValueMin;
    }

  result *= this->PixelMax - this->PixelMin;
  if(valueRange != 0)
    {
    result /= valueRange;
    }

  return result.getIntValue() + this->PixelMin;
}

int pqChartAxis::getPixelForIndex(int index) const
{
  int pixel = 0;
  if(this->Data && index >= 0 && index < static_cast<int>(this->Data->size()))
    {
    pixel = (*this->Data)[index]->Pixel;
    }

  return pixel;
}

pqChartValue pqChartAxis::getValueFor(int pixel)
{
  // Convert the pixel location to a value using:
  // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
  // If using a log scale, the values should be in exponents in
  // order to get a linear mapping.
  pqChartValue v1;
  pqChartValue result;
  bool reversed = false;
  if(this->Scale == pqChartAxis::Logarithmic)
    {
    // If the log scale uses integers, the first value may be zero.
    // In that case, use -1 instead of taking the log of zero.
    reversed = this->TrueMin < 0;
    if(this->TrueMin.getType() == pqChartValue::IntValue &&
        this->ValueMin == 0)
      {
      v1 = MinIntLogPower;
      }
    else
      {
      if(reversed)
        {
        v1 = log10(-this->ValueMin.getDoubleValue());
        }
      else
        {
        v1 = log10(this->ValueMin.getDoubleValue());
        }
      }

    if(this->TrueMin.getType() == pqChartValue::IntValue &&
        this->ValueMax == 0)
      {
      result = MinIntLogPower;
      }
    else
      {
      if(reversed)
        {
        result = log10(-this->ValueMax.getDoubleValue());
        }
      else
        {
        result = log10(this->ValueMax.getDoubleValue());
        }
      }

    result -= v1;
    }
  else
    {
    v1 = this->ValueMin;
    result = this->ValueMax - this->ValueMin;
    }

  result *= pixel - this->PixelMin;
  int pixelRange = this->PixelMax - this->PixelMin;
  if(pixelRange != 0)
    {
    result /= pixelRange;
    }

  result += v1;
  if(this->Scale == pqChartAxis::Logarithmic)
    {
    result = pow((double)10.0, result.getDoubleValue());
    if(reversed)
      {
      result *= -1;
      }
    if(this->TrueMin.getType() != pqChartValue::DoubleValue)
      {
      result.convertTo(pqChartValue::FloatValue);
      }
    }

  return result;
}

pqChartValue pqChartAxis::getValueForIndex(int index) const
{
  pqChartValue value = 0;
  if(this->Data && index >= 0 && index < static_cast<int>(this->Data->size()))
    {
    value = (*this->Data)[index]->Value;
    }

  return value;
}

bool pqChartAxis::isValid() const
{
  if(this->ValueMax == this->ValueMin)
    return false;
  if(this->PixelMax == this->PixelMin)
    return false;
  return true;
}

bool pqChartAxis::isZeroInRange() const
{
  return (this->ValueMax >= 0 && this->ValueMin <= 0) ||
      (this->ValueMax <= 0 && this->ValueMin >= 0);
}

void pqChartAxis::setAxisColor(const QColor &color)
{
  if(this->AxisColor != color)
    {
    this->AxisColor = color;
    if(this->GridType == pqChartAxis::Lighter)
      this->GridColor = pqChartAxis::lighter(this->AxisColor);
    emit this->repaintNeeded();
    }
}

void pqChartAxis::setGridColor(const QColor &color)
{
  if(this->GridType == pqChartAxis::Lighter || this->GridColor != color)
    {
    this->GridColor = color;
    this->GridType = pqChartAxis::Lighter;
    emit this->repaintNeeded();
    }
}

void pqChartAxis::setGridColorType(AxisGridColor type)
{
  if(this->GridType != type)
    {
    this->GridType = type;
    if(this->GridType == pqChartAxis::Lighter)
      {
      this->GridColor = pqChartAxis::lighter(this->AxisColor);
      emit this->repaintNeeded();
      }
    }
}

void pqChartAxis::setTickLabelColor(const QColor &color)
{
  if(this->TickLabelColor != color)
    {
    this->TickLabelColor = color;
    emit this->repaintNeeded();
    }
}

void pqChartAxis::setTickLabelFont(const QFont &font)
{
  this->TickLabelFont = font;
  this->calculateMaxWidth();
}

void pqChartAxis::setPrecision(int precision)
{
  this->Precision = precision;
  if(this->Precision < 0)
    this->Precision = 0;
  this->calculateMaxWidth();
}

void pqChartAxis::setVisible(bool visible)
{
  if(this->Visible != visible)
    {
    this->Visible = visible;
    emit this->layoutNeeded();
    }
}

void pqChartAxis::setNumberOfIntervals(int intervals)
{
  this->Intervals = intervals;
  this->Layout = pqChartAxis::FixedInterval;
  emit this->layoutNeeded();
}

void pqChartAxis::setNeigbors(const pqChartAxis *atMin,
    const pqChartAxis *atMax)
{
  this->AtMin = atMin;
  this->AtMax = atMax;
}

const int pqChartAxis::getLayoutThickness() const
{
  switch(this->Location)
    {
    case pqChartAxis::Top:
    case pqChartAxis::Bottom:
      return TICK_MARGIN + QFontMetrics(this->TickLabelFont).height() + this->Label->getSizeRequest().height();
    case pqChartAxis::Left:
    case pqChartAxis::Right:
      return TICK_MARGIN + this->getMaxWidth() + this->Label->getSizeRequest().width();
    }
    
  return 0;
}

void pqChartAxis::layoutAxis(const QRect &area)
{
  if(this->WidthMax <= 0 || !area.isValid())
    return;

  QFontMetrics fm(this->TickLabelFont);
  int fontHeight = fm.height();

  QRect label_bounds;

  // Set up the bounding rectangle. Then, set up the pixel range
  // based on font, margin, visibility, bounds, and neighbors.
  if(this->Location == pqChartAxis::Top ||
    this->Location == pqChartAxis::Bottom)
    {
    this->Bounds.setLeft(area.left());
    this->Bounds.setRight(area.right());
    if(this->Location == pqChartAxis::Top)
      {
      this->Bounds.setTop(area.top());
      this->Bounds.setBottom(area.top());
      if(this->isVisible())
        {
        label_bounds = QRect(area.left(), area.top(), area.width(), area.top() + this->Label->getSizeRequest().height());
        this->Bounds.setTop(area.top() + this->Label->getSizeRequest().height());
        this->Bounds.setBottom(area.top() + TICK_MARGIN + fontHeight + this->Label->getSizeRequest().height());
        }
      }
    else
      {
      this->Bounds.setTop(area.bottom());
      this->Bounds.setBottom(area.bottom());
      if(this->isVisible())
        {
        this->Bounds.setTop(area.bottom() - TICK_MARGIN - fontHeight - this->Label->getSizeRequest().height());
        this->Bounds.setBottom(area.bottom() - this->Label->getSizeRequest().height());
        label_bounds = QRect(area.left(), this->Bounds.bottom(), area.width(), this->Label->getSizeRequest().height());
        }
      }

    if(this->isVisible())
      this->PixelMin = this->WidthMax/2;
    else
      this->PixelMin = 0;
    this->PixelMax = this->PixelMin;

    if(this->AtMin && this->AtMin->isVisible())
      {
      this->PixelMin = vtkstd::max(this->PixelMin, this->AtMin->getLayoutThickness());
      }

    if(this->AtMax && this->AtMax->isVisible())
      {
      this->PixelMax = vtkstd::max(this->PixelMax, this->AtMax->getLayoutThickness());
      }

    this->PixelMin = this->Bounds.left() + this->PixelMin;
    this->PixelMax = this->Bounds.right() - this->PixelMax;
    
    label_bounds.setLeft(this->PixelMin);
    label_bounds.setRight(this->PixelMax);
    }
  else
    {
    this->Bounds.setTop(area.top());
    this->Bounds.setBottom(area.bottom());
    if(this->Location == pqChartAxis::Left)
      {
      this->Bounds.setLeft(area.left());
      this->Bounds.setRight(area.left());
      if(this->isVisible())
        {
        label_bounds = QRect(area.left(), area.top(), this->Label->getSizeRequest().width(), area.height());
        this->Bounds.setLeft(area.left() + this->Label->getSizeRequest().width());
        this->Bounds.setRight(area.left() + this->Label->getSizeRequest().width() + TICK_MARGIN + this->WidthMax);
        }
      }
    else
      {
      this->Bounds.setLeft(area.right());
      this->Bounds.setRight(area.right());
      if(this->isVisible())
        {
        this->Bounds.setLeft(area.right() - TICK_MARGIN - this->WidthMax - this->Label->getSizeRequest().width());
        this->Bounds.setRight(area.right() - this->Label->getSizeRequest().width());
        label_bounds = QRect(area.right() - this->Label->getSizeRequest().width(), area.top(), this->Label->getSizeRequest().width(), area.height());
        }
      }

    if(this->isVisible())
      this->PixelMin = fontHeight/2;
    else
      this->PixelMin = 0;
    this->PixelMax = this->PixelMin;

    if(this->AtMin && this->AtMin->isVisible())
      {
      this->PixelMin = vtkstd::max(this->PixelMin, this->AtMin->getLayoutThickness());
      }
      
    if(this->AtMax && this->AtMax->isVisible())
      {
      this->PixelMax = vtkstd::max(this->PixelMax, this->AtMax->getLayoutThickness());
      }

    this->PixelMin = this->Bounds.bottom() - this->PixelMin;
    this->PixelMax = this->Bounds.top() + this->PixelMax;
    
    label_bounds.setTop(this->PixelMax);
    label_bounds.setBottom(this->PixelMin);
    }

  this->Label->setBounds(label_bounds);

  // Set up the remaining parameters.
  this->cleanData();
  if(this->Layout == pqChartAxis::FixedInterval)
    this->calculateFixedLayout();
  else if(this->Scale == pqChartAxis::Logarithmic)
    this->calculateLogInterval();
  else
    this->calculateInterval();
}

void pqChartAxis::drawAxis(QPainter *p, const QRect &area)
{
  if(!p || !p->isActive() || !this->isVisible() || !this->isValid() ||
      !this->Data)
    {
    return;
    }

  p->setFont(this->TickLabelFont);

  // Set up the grid area.
  QRect gridArea;
  if(this->AtMin || this->AtMax)
    {
    if(this->Location == pqChartAxis::Top ||
        this->Location == pqChartAxis::Bottom)
      {
      gridArea.setLeft(this->PixelMin);
      gridArea.setRight(this->PixelMax);
      if(this->AtMin && this->AtMin->isVisible() && this->AtMin->isValid())
        {
        gridArea.setTop(this->AtMin->PixelMax);
        gridArea.setBottom(this->AtMin->PixelMin);
        if(this->AtMax && this->AtMax->isVisible() && this->AtMax->isValid())
          {
          if(this->AtMax->PixelMax < this->AtMin->PixelMax)
            {
            gridArea.setTop(this->AtMax->PixelMax);
            }
          if(this->AtMax->PixelMin > this->AtMin->PixelMin)
            {
            gridArea.setBottom(this->AtMax->PixelMin);
            }
          }
        }
      else if(this->AtMax && this->AtMax->isVisible() && this->AtMax->isValid())
        {
        gridArea.setTop(this->AtMax->PixelMax);
        gridArea.setBottom(this->AtMax->PixelMin);
        }
      }
    else
      {
      gridArea.setBottom(this->PixelMin);
      gridArea.setTop(this->PixelMax);
      if(this->AtMin && this->AtMin->isVisible() && this->AtMin->isValid())
        {
        gridArea.setLeft(this->AtMin->PixelMin);
        gridArea.setRight(this->AtMin->PixelMax);
        if(this->AtMax && this->AtMax->isVisible() && this->AtMax->isValid())
          {
          if(this->AtMax->PixelMin < this->AtMin->PixelMin)
            {
            gridArea.setLeft(this->AtMax->PixelMin);
            }
          if(this->AtMax->PixelMax > this->AtMin->PixelMax)
            {
            gridArea.setRight(this->AtMax->PixelMax);
            }
          }
        }
      else if(this->AtMax && this->AtMax->isVisible() && this->AtMax->isValid())
        {
        gridArea.setLeft(this->AtMax->PixelMin);
        gridArea.setRight(this->AtMax->PixelMax);
        }
      }
    }

  // Only paint the axis and grid if either intersects the drawing
  // area. The bounds for some axes might not extend to the full
  // axis drawing area, so assume the axis is on an edge.
  bool isInArea = true;
  if(this->Location == pqChartAxis::Top)
    {
    isInArea = area.top() <= this->Bounds.bottom();
    }
  else if(this->Location == pqChartAxis::Left)
    {
    isInArea = area.left() <= this->Bounds.right();
    }
  else if(this->Location == pqChartAxis::Right)
    {
    isInArea = area.right() >= this->Bounds.left();
    }
  else
    {
    isInArea = area.bottom() >= this->Bounds.top();
    }

  if(!(isInArea || (this->GridVisible && gridArea.intersects(area))))
    {
    return;
    }

  bool vertical = this->Location == pqChartAxis::Left ||
      this->Location == pqChartAxis::Right;
  QString label;
  QFontMetrics fm(this->TickLabelFont);
  int fontAscent = fm.ascent();
  int halfAscent = fontAscent/2;
  int fontDescent = fm.descent();
  int x = 0;
  int y = 0;
  int tick = 0;
  int tickSmall = 0;

  // Set up the constant values based on the axis location.
  if(this->Location == pqChartAxis::Top)
    {
    y = this->Bounds.bottom();
    tick = y - TICK_LENGTH;
    tickSmall = y - TICK_LENGTH_SMALL;
    }
  else if(this->Location == pqChartAxis::Left)
    {
    x = this->Bounds.right();
    tick = x - TICK_LENGTH;
    tickSmall = x - TICK_LENGTH_SMALL;
    }
  else if(this->Location == pqChartAxis::Right)
    {
    x = this->Bounds.left();
    tick = x + TICK_LENGTH;
    tickSmall = x + TICK_LENGTH_SMALL;
    }
  else
    {
    y = this->Bounds.top();
    tick = y + TICK_LENGTH;
    tickSmall = y + TICK_LENGTH_SMALL;
    }

  // Draw the axis labels. Draw the axis grid lines if specified.
  int i = 0;
  p->setPen(this->AxisColor);
  pqChartAxisData::iterator iter = this->Data->begin();
  for( ; iter != this->Data->end(); ++iter, ++i)
    {
    // Make sure the label needs to be drawn.
    if((this->Layout == pqChartAxis::FixedInterval ||
        this->Scale == pqChartAxis::Logarithmic) && i > this->Count)
      {
      break;
      }
    if(!(*iter))
      {
      continue;
      }

    if(vertical)
      {
      y = (*iter)->Pixel;
      if(y - halfAscent > area.bottom())
        continue;
      else if(y + halfAscent < area.top())
        break;
      }
    else
      {
      x = (*iter)->Pixel;
      if(this->WidthMax > 0)
        {
        if(x + this->WidthMax/2 < area.left())
          continue;
        else if(x - this->WidthMax/2 > area.right())
          break;
        }
      }

    label = (*iter)->Value.getString(this->Precision,this->Notation);
    if(vertical)
      {
      if(this->GridVisible)
        {
        p->setPen(this->GridColor);
        p->drawLine(gridArea.left(), y, gridArea.right(), y);
        }

      if(this->Skip == 1 || i % this->Skip == 0)
        {
        p->setPen(this->AxisColor);
        p->drawLine(tick, y, x, y);
        y += halfAscent;
        p->setPen(this->TickLabelColor);
        if(this->Location == pqChartAxis::Left)
          p->drawText(x - fm.width(label) - TICK_MARGIN, y, label);
        else
          p->drawText(x + TICK_MARGIN, y, label);
        }
      else
        {
        p->setPen(this->AxisColor);
        p->drawLine(tickSmall, y, x, y);
        }
      }
    else
      {
      if(this->GridVisible)
        {
        p->setPen(this->GridColor);
        p->drawLine(x, gridArea.top(), x, gridArea.bottom());
        }

      if(this->Skip == 1 || i % this->Skip == 0)
        {
        p->setPen(this->AxisColor);
        p->drawLine(x, tick, x, y);
        x -= fm.width(label)/2;
        p->setPen(this->TickLabelColor);
        if(this->Location == pqChartAxis::Top)
          p->drawText(x, y - TICK_MARGIN - fontDescent, label);
        else
          p->drawText(x, y + TICK_MARGIN + fontAscent, label);
        }
      else
        {
        p->setPen(this->AxisColor);
        p->drawLine(x, tickSmall, x, y);
        }
      }
    }

  // Draw the axis line in last to cover the grid lines.
  p->setPen(this->AxisColor);
  if(vertical)
    {
    p->drawLine(x, this->PixelMin, x, this->PixelMax);
    }
  else
    {
    p->drawLine(this->PixelMin, y, this->PixelMax, y);
    }
    
  // Draw the axis label
  this->Label->draw(*p, area);
}

void pqChartAxis::drawAxisLine(QPainter *p)
{
  if(!p || !p->isActive() || !this->isVisible() || !this->isValid())
    {
    return;
    }

  int x = 0;
  int y = 0;
  bool vertical = this->Location == pqChartAxis::Left ||
      this->Location == pqChartAxis::Right;
  if(this->Location == pqChartAxis::Top)
    {
    y = this->Bounds.bottom();
    }
  else if(this->Location == pqChartAxis::Left)
    {
    x = this->Bounds.right();
    }
  else if(this->Location == pqChartAxis::Right)
    {
    x = this->Bounds.left();
    }
  else
    {
    y = this->Bounds.top();
    }

  p->setPen(this->AxisColor);
  if(vertical)
    {
    p->drawLine(x, this->PixelMin, x, this->PixelMax);
    }
  else
    {
    p->drawLine(this->PixelMin, y, this->PixelMax, y);
    }
}

QColor pqChartAxis::lighter(const QColor color, float factor)
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

void pqChartAxis::calculateMaxWidth()
{
  if(this->ValueMax == this->ValueMin)
    {
    return;
    }

  // If the axis uses logarithmic scale on integer values, the
  // values can be converted to floats.
  int length1 = 0;
  int length2 = 0;
  if(this->Scale == pqChartAxis::Logarithmic &&
      this->ValueMin.getType() == pqChartValue::IntValue)
    {
    pqChartValue value = this->ValueMax;
    value.convertTo(pqChartValue::FloatValue);
    length1 = value.getString(this->Precision,this->Notation).length();
    value = this->ValueMin;
    value.convertTo(pqChartValue::FloatValue);
    length2 = value.getString(this->Precision,this->Notation).length();
    }
  else
    {
    length1 = this->ValueMax.getString(this->Precision,this->Notation).length();
    length2 = this->ValueMin.getString(this->Precision,this->Notation).length();
    }

  if(length2 > length1)
    {
    length1 = length2;
    }

  // Use a string of '8's to determine the maximum font width
  // in case the font is not fixed-pitch.
  QFontMetrics fm(this->TickLabelFont);
  QString str;
  str.fill('8', length1);
  this->WidthMax = fm.width(str);

  // Let the observers know the axis needs to be layed out again.
  emit this->layoutNeeded();
}

void pqChartAxis::calculateInterval()
{
  if(!this->Data || !this->isValid())
    return;

  int allowed = 0;
  if(this->Location == pqChartAxis::Top ||
      this->Location == pqChartAxis::Bottom)
    {
    if(this->WidthMax == 0)
      {
      return;
      }

    allowed = this->getPixelRange()/(this->WidthMax + LABEL_MARGIN);
    }
  else
    {
    QFontMetrics fm(this->TickLabelFont);
    allowed = this->getPixelRange()/(2*fm.height());
    }

  // There is no need to calculate anything for one interval.
  pqChartAxisPair *pair = 0;
  if(allowed <= 1)
    {
    this->ValueMax = this->TrueMax;
    this->ValueMin = this->TrueMin;
    pair = new pqChartAxisPair();
    if(pair)
      {
      pair->Value = this->TrueMin;
      pair->Pixel = this->PixelMin;
      this->Data->push_back(pair);
      }

    pair = new pqChartAxisPair();
    if(pair)
      {
      pair->Value = this->TrueMax;
      pair->Pixel = this->PixelMax;
      this->Data->push_back(pair);
      }

    return;
    }

  // Find the value range. Convert integers to floating point
  // values to compare with the interval list.
  pqChartValue range = this->TrueMax - this->TrueMin;
  if(range.getType() == pqChartValue::IntValue)
    {
    range.convertTo(pqChartValue::FloatValue);
    }

  // Convert the value interval to exponent format for comparison.
  // Save the exponent for re-application.
  range /= allowed;
  QString rangeString;
  if(range.getType() == pqChartValue::DoubleValue)
    {
    rangeString.setNum(range.getDoubleValue(), 'e', 1);
    }
  else
    {
    rangeString.setNum(range.getFloatValue(), 'e', 1);
    }

  const char *temp = rangeString.toAscii().data();
  int exponent = 0;
  int index = rangeString.indexOf("e");
  if(index != -1)
    {
    exponent = rangeString.right(rangeString.length() - index - 1).toInt();
    rangeString.truncate((unsigned int)index);
    }

  // Set the new value for the range, which excludes exponent.
  range.setValue(rangeString.toFloat());

  // Search through the interval list for the closest one.
  // Convert the negative interval to match the positive
  // list values. Make sure the interval is not too small
  // for the chart label precision.
  bool negative = range < 0.0;
  if(negative)
    {
    range *= -1;
    }

  bool found = false;
  int minExponent = -this->Precision;
  if(this->TrueMax.getType() == pqChartValue::IntValue)
    {
    minExponent = 0;
    }
  // FIX: if the range is very small (exponent<0) we want to use more intervals, not fewer
  if(exponent < minExponent && exponent>0)
    {
    found = true;
    range = IntervalList[0];
    exponent = minExponent;
    }
  else
    {
    int i = 0;
    for( ; i < IntervalListLength; i++)
      {
      // Skip 2.5 if the precision is reached.
      if(exponent == minExponent && i == 2)
        {
        continue;
        }
      if(range <= IntervalList[i])
        {
        range = IntervalList[i];
        found = true;
        break;
        }
      }
    }

  if(!found)
    {
    range = IntervalList[0];
    exponent++;
    }

  if(negative)
    {
    range *= -1;
    }

  // After finding a suitable interval, convert it back to
  // a usable form.
  rangeString.setNum(range.getFloatValue(), 'f', 1);
  QString expString;
  expString.setNum(exponent);
  rangeString.append("e").append(expString);
  temp = rangeString.toAscii().data();
  if(this->TrueMax.getType() == pqChartValue::DoubleValue)
    {
    range.setValue(rangeString.toDouble());
    }
  else
    {
    range.setValue(rangeString.toFloat());
    }

  // Assign the pixel interval from the calculated value interval.
  pqChartValue interval;
  if(this->TrueMax.getType() == pqChartValue::IntValue)
    {
    interval.setValue(range.getIntValue());
    }
  else
    {
    interval = range;
    }

  // Adjust the displayed min/max to align to the interval.
  if(this->TrueMin == 0)
    {
    this->ValueMin = this->TrueMin;
    }
  else
    {
    int numIntervals = this->TrueMin/interval;
    this->ValueMin = interval * numIntervals;
    if(this->ValueMin > this->TrueMin)
      {
      this->ValueMin -= interval;
      }
    else if(this->ExtraMinPadding && this->ValueMin == this->TrueMin)
      {
      this->ValueMin -= interval;
      }
    }

  if(this->TrueMax == 0)
    {
    this->ValueMax = this->TrueMax;
    }
  else
    {
    int numIntervals = this->TrueMax/interval;
    this->ValueMax = interval * numIntervals;
    if(this->ValueMax < this->TrueMax)
      {
      this->ValueMax += interval;
      }
    else if(this->ExtraMaxPadding && this->ValueMax == this->TrueMax)
      {
      this->ValueMax += interval;
      }
    }

  // Fill in the data based on the interval.
  this->Skip = 1;
  int numberOfIntervals = 0;
  pqChartValue v = this->ValueMin;
  pqChartValue max = this->ValueMax;
  max += interval/2; // Account for round-off error.
  for( ; v < max; v += interval)
    {
    pair = new pqChartAxisPair();
    if(pair)
      {
      pair->Value = v;
      pair->Pixel = this->getPixelFor(v);
      this->Data->push_back(pair);
      }
    else
      {
      break;
      }
    numberOfIntervals++;
    }

  this->Intervals = numberOfIntervals;
}

void pqChartAxis::calculateLogInterval()
{
  if(!this->Data || !this->isValid())
    {
    return;
    }

  int needed = 0;
  if(this->Location == pqChartAxis::Top ||
      this->Location == pqChartAxis::Bottom)
    {
    if(this->WidthMax == 0)
      {
      return;
      }

    needed = this->WidthMax + LABEL_MARGIN;
    }
  else
    {
    QFontMetrics fm(this->TickLabelFont);
    needed = 2*fm.height();
    }

  // Adjust the min/max to a power of ten.
  int maxExp = -1;
  int minExp = -1;
  double logValue = 0.0;
  bool reversed = this->TrueMin < 0;
  if(reversed)
    {
    logValue = log10(-this->TrueMin.getDoubleValue());
    }
  else
    {
    logValue = log10(this->TrueMax.getDoubleValue());
    }

  maxExp = (int)logValue;
  if(logValue > (double)maxExp)
    {
    maxExp++;
    }

  if(!(this->TrueMin.getType() == pqChartValue::IntValue &&
      ((reversed && this->TrueMax == 0) || this->TrueMin == 0)))
    {
    if(reversed)
      {
      logValue = log10(-this->TrueMax.getDoubleValue());
      }
    else
      {
      logValue = log10(this->TrueMin.getDoubleValue());
      }

    // The log10 result can be off for certain values so adjust
    // the result to get a better integer exponent.
    if(logValue < 0.0)
      {
      logValue -= pqChartAxis::MinLogValue;
      }
    else
      {
      logValue += pqChartAxis::MinLogValue;
      }

    minExp = (int)logValue;
    }

  int pixelRange = this->getPixelRange();
  int allowed = pixelRange/needed;
  int subInterval = 0;
  this->Intervals = maxExp - minExp;
  this->Count = this->Intervals;
  if(reversed)
    {
    this->ValueMax = -pow((double)10.0, (double)minExp);
    this->ValueMax.convertTo(this->TrueMin.getType());
    }
  else
    {
    this->ValueMin = pow((double)10.0, (double)minExp);
    this->ValueMin.convertTo(this->TrueMin.getType());
    }

  if(allowed > this->Intervals)
    {
    this->Skip = 1;
    if(reversed)
      {
      this->ValueMin = -pow((double)10.0, (double)maxExp);
      this->ValueMin.convertTo(this->TrueMin.getType());
      }
    else
      {
      this->ValueMax = pow((double)10.0, (double)maxExp);
      this->ValueMax.convertTo(this->TrueMin.getType());
      }

    // If the number of allowed tick marks is greater than the
    // exponent range, there may be space for sub-intervals.
    int remaining = allowed/this->Intervals;
    if(remaining >= 20)
      {
      subInterval = 1;
      this->Count *= 9;
      }
    else if(remaining >= 10)
      {
      subInterval = 2;
      this->Count *= 5;
      }
    else if(remaining >= 3)
      {
      subInterval = 5;
      this->Count *= 2;
      }
    }
  else
    {
    // Set up the skip interval and number showing for the axis.
    if(pixelRange/this->Count < 2)
      {
      this->Count = pixelRange/2;
      if(this->Count == 0)
        {
        this->Count = 1;
        }

      if(reversed)
        {
        this->ValueMin = -pow((double)10.0, (double)(minExp + this->Count));
        this->ValueMin.convertTo(this->TrueMax.getType());
        }
      else
        {
        this->ValueMax = pow((double)10.0, (double)(minExp + this->Count));
        this->ValueMax.convertTo(this->TrueMax.getType());
        }
      }
    else
      {
      if(reversed)
        {
        this->ValueMin = -pow((double)10.0, (double)maxExp);
        this->ValueMin.convertTo(this->TrueMax.getType());
        }
      else
        {
        this->ValueMax = pow((double)10.0, (double)maxExp);
        this->ValueMax.convertTo(this->TrueMax.getType());
        }
      }

    needed *= this->Count - 1;
    this->Skip = needed/pixelRange;
    if(this->Skip == 0 || needed % pixelRange > 0)
      {
      this->Skip += 1;
      }
    }

  // Place the first value on the list using value min in case
  // the first value is int zero.
  pqChartAxisPair *pair = new pqChartAxisPair();
  if(!pair)
    {
    return;
    }

  this->Data->push_back(pair);
  if(reversed)
    {
    pair->Value = this->ValueMax;
    pair->Pixel = this->PixelMax;
    }
  else
    {
    pair->Value = this->ValueMin;
    pair->Pixel = this->PixelMin;
    }

  // Fill in the data based on the interval.
  pqChartAxisPair *subItem = 0;
  for(int i = 1; i <= this->Intervals; i++)
    {
    // Add entries for the sub-intervals if there are any. Don't
    // add sub-intervals for int values less than one.
    if(subInterval > 0 && !(pair->Value.getType() == pqChartValue::IntValue &&
        pair->Value == 0))
      {
      for(int j = subInterval; j < 10; j += subInterval)
        {
        subItem = new pqChartAxisPair();
        if(!pair)
          {
          break;
          }

        subItem->Value = pair->Value;
        subItem->Value *= j;
        subItem->Pixel = this->getPixelFor(subItem->Value);
        this->Data->push_back(subItem);
        }
      }

    pair = new pqChartAxisPair();
    if(!pair)
      {
      break;
      }

    pair->Value = pow((double)10.0, (double)(minExp + i));
    if(reversed)
      {
      pair->Value *= -1;
      }

    pair->Value.convertTo(this->TrueMin.getType());
    pair->Pixel = this->getPixelFor(pair->Value);
    this->Data->push_back(pair);
    }
}

void pqChartAxis::calculateFixedLayout()
{
  if(!this->Data || !this->isValid() || this->Intervals <= 0)
    {
    return;
    }

  // For a log scale that is equally spaced, use the exponent
  // as the interval.
  pqChartValue logMin;
  pqChartValue logMax;
  pqChartValue interval;
  bool reversed = false;
  if(this->Scale == pqChartAxis::Logarithmic)
    {
    reversed = this->TrueMin < 0;
    if(this->TrueMin.getType() == pqChartValue::IntValue &&
        this->TrueMin == 0)
      {
      logMin = MinIntLogPower;
      }
    else
      {
      if(reversed)
        {
        logMin = log10(-this->TrueMin.getDoubleValue());
        }
      else
        {
        logMin = log10(this->TrueMin.getDoubleValue());
        }
      }

    if(this->TrueMax.getType() == pqChartValue::IntValue &&
        this->TrueMax == 0)
      {
      logMax = MinIntLogPower;
      }
    else
      {
      if(reversed)
        {
        logMax = log10(-this->TrueMax.getDoubleValue());
        }
      else
        {
        logMax = log10(this->TrueMax.getDoubleValue());
        }
      }

    interval = (logMax - logMin)/this->Intervals;
    }
  else
    {
    interval = (this->TrueMax - this->TrueMin)/this->Intervals;
    }

  int needed = 0;
  if(this->Location == pqChartAxis::Top ||
      this->Location == pqChartAxis::Bottom)
    {
    if(this->WidthMax == 0)
      {
      return;
      }

    needed = this->WidthMax + LABEL_MARGIN;
    }
  else
    {
    QFontMetrics fm(this->TickLabelFont);
    needed = 2*fm.height();
    }

  // Determine the pixel length between each tick mark on the
  // axis. If the length is too small, only show a portion of
  // the values.
  int pixelRange = getPixelRange();
  this->Count = this->Intervals;
  if(pixelRange/this->Count < 2)
    {
    this->Count = pixelRange/2;
    if(this->Count == 0)
      {
      this->Count = 1;
      }

    if(this->Scale == pqChartAxis::Logarithmic)
      {
      pqChartValue newMax = logMin + (interval * this->Count);
      this->ValueMax = pow((double)10.0, newMax.getDoubleValue());
      if(reversed)
        {
        this->ValueMax *= -1;
        }
      if(this->TrueMin.getType() != pqChartValue::DoubleValue)
        {
        this->ValueMax.convertTo(pqChartValue::FloatValue);
        }
      }
    else
      {
      this->ValueMax = this->ValueMin + (interval * this->Count);
      }
    }
  else
    {
    this->ValueMax = this->TrueMax;
    }

  // Set the skip interval for the axis.
  needed *= this->Count - 1;
  this->Skip = needed/pixelRange;
  if(this->Skip == 0 || needed % pixelRange > 0)
    {
    this->Skip += 1;
    }

  int i = 0;
  pqChartAxisPair *pair = 0;
  if(this->Scale == pqChartAxis::Logarithmic)
    {
    // Place the first value on the list using value min in case
    // the first value is int zero.
    pair = new pqChartAxisPair();
    if(!pair)
      {
      return;
      }

    pair->Value = this->ValueMin;
    if(this->TrueMin.getType() == pqChartValue::IntValue)
      {
      pair->Value.convertTo(pqChartValue::FloatValue);
      }

    pair->Pixel = this->PixelMin;
    this->Data->push_back(pair);
    i = 1;
    }

  // Fill in the data based on the interval.
  pqChartValue v = this->ValueMin;
  for( ; i <= this->Intervals; i++)
    {
    pair = new pqChartAxisPair();
    if(!pair)
      {
      break;
      }

    if(this->Scale == pqChartAxis::Logarithmic)
      {
      logMin += interval;
      pair->Value = pow((double)10.0, logMin.getDoubleValue());
      if(reversed)
        {
        pair->Value *= -1;
        }
      if(this->TrueMin.getType() != pqChartValue::DoubleValue)
        {
        pair->Value.convertTo(pqChartValue::FloatValue);
        }
      }
    else
      {
      pair->Value = v;
      v += interval;
      }

    if(i == this->Intervals && this->Count == this->Intervals)
      {
      pair->Pixel = this->PixelMax;
      }
    else
      {
      pair->Pixel = this->getPixelFor(pair->Value);
      }

    this->Data->push_back(pair);
    }

  // If the axis is a reversed log scale of integer type, the max
  // may need to be adjusted to zero.
  if(reversed && this->Scale == pqChartAxis::Logarithmic && pair &&
      this->TrueMax.getType() == pqChartValue::IntValue && this->TrueMax == 0)
    {
    pair->Value = (float)0.0;
    }
}

void pqChartAxis::cleanData()
{
  if(this->Data)
    {
    pqChartAxisData::iterator iter = this->Data->begin();
    for( ; iter != this->Data->end(); ++iter)
      {
      delete *iter;
      *iter = 0;
      }

    this->Data->clear();
    }
}


