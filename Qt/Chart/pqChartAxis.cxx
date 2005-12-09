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
#include <QPainter>
#include <QFontMetrics>

#include <math.h>


#define TICK_LENGTH_SMALL 3
#define TICK_LENGTH 5
#define TICK_MARGIN 8
#define LABEL_MARGIN 10


// The interval list is used to determine a suitable interval
// for a shifting axis.
static pqChartValue IntervalList[] = {
    pqChartValue((float)1.0),
    pqChartValue((float)2.0),
    pqChartValue((float)2.5),
    pqChartValue((float)5.0)};
static int IntervalListLength = 4;


pqChartAxis::pqChartAxis(AxisLocation location, QObject *parent)
  : QObject(parent), Bounds(), Axis(Qt::black), Grid(178, 178, 178),
    Font(), ValueMin(), ValueMax(), TrueMin(), TrueMax(), Interval()
{
  this->Location = location;
  this->Layout = pqChartAxis::BestInterval;
  this->GridType = pqChartAxis::Lighter;
  this->AtMin = 0;
  this->AtMax = 0;
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
}

void pqChartAxis::setValueRange(const pqChartValue &min,
    const pqChartValue &max)
{
  this->ValueMin = min;
  this->ValueMax = max;
  this->TrueMin = min;
  this->TrueMax = max;
  this->calculateMaxWidth();
}

pqChartValue pqChartAxis::getValueRange() const
{
  return this->ValueMax - this->ValueMin;
}

void pqChartAxis::setMinValue(const pqChartValue &min)
{
  this->ValueMin = min;
  this->TrueMin = min;
  this->calculateMaxWidth();
}

void pqChartAxis::setMaxValue(const pqChartValue &max)
{
  this->ValueMax = max;
  this->TrueMax = max;
  this->calculateMaxWidth();
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
  pqChartValue result = value - this->ValueMin;
  result *= this->PixelMax - this->PixelMin;
  pqChartValue valueRange = this->ValueMax - this->ValueMin;
  if(valueRange != 0)
    result /= valueRange;
  return result.getIntValue() + this->PixelMin;
}

pqChartValue pqChartAxis::getValueFor(int pixel)
{
  // Convert the pixel location to a value using:
  // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
  pqChartValue result = this->ValueMax - this->ValueMin;
  result *= pixel - this->PixelMin;
  int pixelRange = this->PixelMax - this->PixelMin;
  if(pixelRange != 0)
    result /= pixelRange;
  return result + this->ValueMin;
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

void pqChartAxis::setFont(const QFont &font)
{
  this->Font = font;
  this->calculateMaxWidth();
}

void pqChartAxis::setColor(const QColor &color)
{
  if(this->Axis != color)
    {
    this->Axis = color;
    if(this->GridType == pqChartAxis::Lighter)
      this->Grid = pqChartAxis::lighter(this->Axis);
    emit this->repaintNeeded();
    }
}

void pqChartAxis::setGridColor(const QColor &color)
{
  if(this->GridType != pqChartAxis::Lighter && this->Grid != color)
    {
    this->Grid = color;
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
      this->Grid = pqChartAxis::lighter(this->Axis);
      emit this->repaintNeeded();
      }
    }
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

void pqChartAxis::setInterval(const pqChartValue &interval)
{
  this->Interval = interval;
  this->Layout = pqChartAxis::FixedInterval;
}

void pqChartAxis::setNeigbors(const pqChartAxis *atMin,
    const pqChartAxis *atMax)
{
  this->AtMin = atMin;
  this->AtMax = atMax;
}

void pqChartAxis::layoutAxis(const QRect &area)
{
  if(this->WidthMax <= 0 || !area.isValid())
    return;

  QFontMetrics fm(this->Font);
  int fontHeight = fm.height();

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
        this->Bounds.setBottom(this->Bounds.bottom() + TICK_MARGIN + fontHeight);
      }
    else
      {
      this->Bounds.setTop(area.bottom());
      this->Bounds.setBottom(area.bottom());
      if(this->isVisible())
        this->Bounds.setTop(this->Bounds.top() - TICK_MARGIN - fontHeight);
      }

    if(this->isVisible())
      this->PixelMin = this->WidthMax/2;
    else
      this->PixelMin = 0;
    this->PixelMax = this->PixelMin;

    int axisWidth = 0;
    if(this->AtMin && this->AtMin->isVisible())
      {
      axisWidth = TICK_MARGIN + this->AtMin->getMaxWidth();
      if(this->PixelMin < axisWidth)
        this->PixelMin = axisWidth;
      }

    if(this->AtMax && this->AtMax->isVisible())
      {
      axisWidth = TICK_MARGIN + this->AtMax->getMaxWidth();
      if(this->PixelMax < axisWidth)
        this->PixelMax = axisWidth;
      }

    this->PixelMin = this->Bounds.left() + this->PixelMin;
    this->PixelMax = this->Bounds.right() - this->PixelMax;
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
        this->Bounds.setRight(this->Bounds.right() + TICK_MARGIN + this->WidthMax);
      }
    else
      {
      this->Bounds.setLeft(area.right());
      this->Bounds.setRight(area.right());
      if(this->isVisible())
        this->Bounds.setLeft(this->Bounds.left() - TICK_MARGIN - this->WidthMax);
      }

    if(this->isVisible())
      this->PixelMin = fontHeight/2;
    else
      this->PixelMin = 0;
    this->PixelMax = this->PixelMin;

    int axisHeight = TICK_MARGIN + fontHeight;
    if(this->AtMin && this->AtMin->isVisible() && this->PixelMin < axisHeight)
      this->PixelMin = axisHeight;
    if(this->AtMax && this->AtMax->isVisible() && this->PixelMax < axisHeight)
      this->PixelMax = axisHeight;

    this->PixelMin = this->Bounds.bottom() - this->PixelMin;
    this->PixelMax = this->Bounds.top() + this->PixelMax;
    }

  // Set up the remaining parameters.
  if(this->Layout == pqChartAxis::BestInterval)
    this->calculateInterval();
  else
    this->calculateFixedLayout();
}

void pqChartAxis::drawAxis(QPainter *p, const QRect &area)
{
  if(!p || !p->isActive() || !this->isValid() || this->Interval == 0 ||
      !this->isVisible())
    {
    return;
    }

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
            gridArea.setTop(this->AtMax->PixelMax);
          if(this->AtMax->PixelMin > this->AtMin->PixelMin)
            gridArea.setBottom(this->AtMax->PixelMin);
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
            gridArea.setLeft(this->AtMax->PixelMin);
          if(this->AtMax->PixelMax > this->AtMin->PixelMax)
            gridArea.setRight(this->AtMax->PixelMax);
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
    isInArea = area.top() <= this->Bounds.bottom();
  else if(this->Location == pqChartAxis::Left)
    isInArea = area.left() <= this->Bounds.right();
  else if(this->Location == pqChartAxis::Right)
    isInArea = area.right() >= this->Bounds.left();
  else
    isInArea = area.bottom() >= this->Bounds.top();
  if(!(isInArea || (this->GridVisible && gridArea.intersects(area))))
    return;

  bool vertical = this->Location == pqChartAxis::Left ||
      this->Location == pqChartAxis::Right;
  QString label;
  QFontMetrics fm(this->Font);
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
  p->setPen(this->Axis);
  pqChartValue v = this->ValueMin;
  pqChartValue max = this->ValueMax;
  max += this->Interval/2; // Account for round-off error.
  for( ; v < max; i++, v += this->Interval)
    {
    // Make sure the label needs to be drawn.
    if(vertical)
      {
      y = getPixelFor(v);
      if(y - halfAscent > area.bottom())
        continue;
      else if(y + halfAscent < area.top())
        break;
      }
    else
      {
      x = getPixelFor(v);
      if(this->WidthMax > 0)
        {
        if(x + this->WidthMax/2 < area.left())
          continue;
        else if(x - this->WidthMax/2 > area.right())
          break;
        }
      }

    label = v.getString(this->Precision);
    if(vertical)
      {
      if(this->GridVisible)
        {
        p->setPen(this->Grid);
        p->drawLine(gridArea.left(), y, gridArea.right(), y);
        p->setPen(this->Axis);
        }

      if(this->Skip == 1 || i % this->Skip == 0)
        {
        p->drawLine(tick, y, x, y);
        y += halfAscent;
        if(this->Location == pqChartAxis::Left)
          p->drawText(x - fm.width(label) - TICK_MARGIN, y, label);
        else
          p->drawText(x + TICK_MARGIN, y, label);
        }
      else
        p->drawLine(tickSmall, y, x, y);
      }
    else
      {
      if(this->GridVisible)
        {
        p->setPen(this->Grid);
        p->drawLine(x, gridArea.top(), x, gridArea.bottom());
        p->setPen(this->Axis);
        }

      if(this->Skip == 1 || i % this->Skip == 0)
        {
        p->drawLine(x, tick, x, y);
        x -= fm.width(label)/2;
        if(this->Location == pqChartAxis::Top)
          p->drawText(x, y - TICK_MARGIN - fontDescent, label);
        else
          p->drawText(x, y + TICK_MARGIN + fontAscent, label);
        }
      else
        p->drawLine(x, tickSmall, x, y);
      }
    }

  // Draw the axis line in last to cover the grid lines.
  if(vertical)
    p->drawLine(x, this->PixelMin, x, this->PixelMax);
  else
    p->drawLine(this->PixelMin, y, this->PixelMax, y);
}

void pqChartAxis::drawAxisLine(QPainter *p)
{
  if(!p || !p->isActive() || !this->isValid() || this->Interval == 0 ||
      !this->isVisible())
    {
    return;
    }

  int x = 0;
  int y = 0;
  bool vertical = this->Location == pqChartAxis::Left ||
      this->Location == pqChartAxis::Right;
  if(this->Location == pqChartAxis::Top)
    y = this->Bounds.bottom();
  else if(this->Location == pqChartAxis::Left)
    x = this->Bounds.right();
  else if(this->Location == pqChartAxis::Right)
    x = this->Bounds.left();
  else
    y = this->Bounds.top();

  p->setPen(this->Axis);
  if(vertical)
    p->drawLine(x, this->PixelMin, x, this->PixelMax);
  else
    p->drawLine(this->PixelMin, y, this->PixelMax, y);
}

QColor pqChartAxis::lighter(const QColor color, float factor)
{
  if(factor <= 0.0)
    return color;
  else if(factor >= 1.0)
    return Qt::white;

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
    return;

  int length1 = this->ValueMax.getString(this->Precision).length();
  int length2 = this->ValueMin.getString(this->Precision).length();
  if(length2 > length1)
    length1 = length2;

  // Use a string of '8's to determine the maximum font width
  // in case the font is not fixed-pitch.
  QFontMetrics fm(this->Font);
  QString str;
  str.fill('8', length1);
  this->WidthMax = fm.width(str);

  // Let the observers know the axis needs to be layed out again.
  emit this->layoutNeeded();
}

void pqChartAxis::calculateInterval()
{
  if(!this->isValid())
    return;

  int allowed = 0;
  if(this->Location == pqChartAxis::Top ||
      this->Location == pqChartAxis::Bottom)
    {
    if(this->WidthMax == 0)
      return;

    allowed = getPixelRange()/(this->WidthMax + LABEL_MARGIN);
    }
  else
    {
    QFontMetrics fm(this->Font);
    allowed = getPixelRange()/(2*fm.height());
    }

  // There is no need to calculate anything for one interval.
  if(allowed <= 1)
    {
    this->ValueMax = this->TrueMax;
    this->ValueMin = this->TrueMin;
    this->Interval = this->getValueRange();
    return;
    }

  // Find the value range. Convert integers to floating point
  // values to compare with the interval list.
  pqChartValue range = this->TrueMax - this->TrueMin;
  if(range.getType() == pqChartValue::IntValue)
    range.convertTo(pqChartValue::FloatValue);
  range /= allowed;

  // Convert the value interval to exponent format for comparison.
  // Save the exponent for re-application.
  QString rangeString;
  if(range.getType() == pqChartValue::DoubleValue)
    rangeString.setNum(range.getDoubleValue(), 'e', 1);
  else
    rangeString.setNum(range.getFloatValue(), 'e', 1);
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
    range *= -1;

  bool found = false;
  int minExponent = -this->Precision;
  if(this->TrueMax.getType() == pqChartValue::IntValue)
    minExponent = 0;

  if(exponent < minExponent)
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
        continue;
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
    range *= -1;

  // After finding a suitable interval, convert it back to
  // a usable form.
  rangeString.setNum(range.getFloatValue(), 'f', 1);
  QString expString;
  expString.setNum(exponent);
  rangeString.append("e").append(expString);
  temp = rangeString.toAscii().data();
  if(this->TrueMax.getType() == pqChartValue::DoubleValue)
    range.setValue(rangeString.toDouble());
  else
    range.setValue(rangeString.toFloat());

  // Assign the pixel interval from the calculated value interval.
  if(this->TrueMax.getType() == pqChartValue::IntValue)
    this->Interval.setValue(range.getIntValue());
  else
    this->Interval = range;

  // Adjust the displayed min/max to align to the interval.
  if(this->TrueMin == 0)
    this->ValueMin = this->TrueMin;
  else
    {
    int numIntervals = this->TrueMin/this->Interval;
    this->ValueMin = this->Interval * numIntervals;
    if(this->ValueMin > this->TrueMin)
      this->ValueMin -= this->Interval;
    else if(this->ExtraMinPadding && this->ValueMin == this->TrueMin)
      this->ValueMin -= this->Interval;
    }

  if(this->TrueMax == 0)
    this->ValueMax = this->TrueMax;
  else
    {
    int numIntervals = this->TrueMax/this->Interval;
    this->ValueMax = this->Interval * numIntervals;
    if(this->ValueMax < this->TrueMax)
      this->ValueMax += this->Interval;
    else if(this->ExtraMaxPadding && this->ValueMax == this->TrueMax)
      this->ValueMax += this->Interval;
    }
}

void pqChartAxis::calculateFixedLayout()
{
  if(!this->isValid() || this->Interval == 0)
    return;

  // Calculate the total number of intervals.
  int total = (this->TrueMax - this->TrueMin)/this->Interval;

  int needed = 0;
  if(this->Location == pqChartAxis::Top ||
      this->Location == pqChartAxis::Bottom)
    {
    if(this->WidthMax == 0)
      return;

    needed = this->WidthMax + LABEL_MARGIN;
    }
  else
    {
    QFontMetrics fm(this->Font);
    needed = 2*fm.height();
    }

  // Determine the pixel length between each tick mark on the
  // axis. If the length is too small, only show a portion of
  // the values.
  int pixelRange = getPixelRange();
  if(pixelRange/total < 2)
    {
    total = pixelRange/2;
    if(total == 0)
      total = 1;
    this->ValueMax = this->ValueMin + (this->Interval * total);
    }
  else
    this->ValueMax = this->TrueMax;

  // Set the skip interval for the axis.
  needed *= total - 1;
  this->Skip = needed/pixelRange;
  if(this->Skip == 0 || needed % pixelRange > 0)
    this->Skip += 1;

  // Save the total number of axis values showing.
  this->Count = total;
}


