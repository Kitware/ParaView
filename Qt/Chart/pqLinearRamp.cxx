/*!
 * \file pqLinearRamp.cxx
 *
 * \brief
 *   The pqLinearRamp class is used to draw a linear ramp function.
 *
 * \author Mark Richardson
 * \date   August 3, 2005
 */

#include "pqLinearRamp.h"
#include "pqChartValue.h"


pqLinearRamp::pqLinearRamp(QObject *p)
  : pqLinePlot(p), Point1(), Point2()
{
}

bool pqLinearRamp::getCoordinate(int index, pqChartCoordinate &coord) const
{
  if(index == 0)
    coord = this->Point1;
  else if(index == 1)
    coord = this->Point2;
  else
    return false;

  return true;
}

void pqLinearRamp::getMaxX(pqChartValue &value) const
{
  if(this->Point1.X > this->Point2.X)
    value = this->Point1.X;
  else
    value = this->Point2.X;
}

void pqLinearRamp::getMinX(pqChartValue &value) const
{
  if(this->Point1.X < this->Point2.X)
    value = this->Point1.X;
  else
    value = this->Point2.X;
}

void pqLinearRamp::getMaxY(pqChartValue &value) const
{
  if(this->Point1.Y > this->Point2.Y)
    value = this->Point1.Y;
  else
    value = this->Point2.Y;
}

void pqLinearRamp::getMinY(pqChartValue &value) const
{
  if(this->Point1.Y < this->Point2.Y)
    value = this->Point1.Y;
  else
    value = this->Point2.Y;
}

void pqLinearRamp::setEndPoints(const pqChartCoordinate &p1,
    const pqChartCoordinate &p2)
{
  this->Point1 = p1;
  this->Point2 = p2;
  this->setModified(true);
}

void pqLinearRamp::setEndPoint1(const pqChartCoordinate &p1)
{
  this->Point1 = p1;
  this->setModified(true);
}

void pqLinearRamp::setEndPoint2(const pqChartCoordinate &p2)
{
  this->Point2 = p2;
  this->setModified(true);
}


