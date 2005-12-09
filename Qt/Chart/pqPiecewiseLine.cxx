/*!
 * \file pqPiecewiseLine.cxx
 *
 * \brief
 *   The pqPiecewiseLine class is used to draw a piecewise linear
 *   function.
 *
 * \author Mark Richardson
 * \date   August 22, 2005
 */

#include "pqPiecewiseLine.h"
#include "pqChartValue.h"
#include "pqChartCoordinate.h"


/// \class pqPiecewiseLineData
/// \brief
///   The pqPiecewiseLineData class hides the private data of the
///   pqPiecewiseLine class
class pqPiecewiseLineData
{
public:
  pqPiecewiseLineData();
  ~pqPiecewiseLineData() {}

  /// Sets the min/max based on the current list.
  void calculateBounds();

public:
  pqChartCoordinateList coords; ///< Stores the coordinate list.
  pqChartCoordinate min;        ///< Stores the minimum coordinate.
  pqChartCoordinate max;        ///< Stores the maximum coordinate.
};


pqPiecewiseLineData::pqPiecewiseLineData()
  : coords(), min(), max()
{
}

void pqPiecewiseLineData::calculateBounds()
{
  bool first = true;
  pqChartCoordinateList::Iterator iter = this->coords.begin();
  for( ; iter != this->coords.end(); ++iter)
    {
    if(first)
      {
      this->min = *iter;
      this->max = *iter;
      first = false;
      }
    else
      {
      if(iter->X > this->max.X)
        this->max.X = iter->X;
      else if(iter->X < this->min.X)
        this->min.X = iter->X;
      if(iter->Y > this->max.Y)
        this->max.Y = iter->Y;
      else if(iter->Y < this->min.Y)
        this->min.Y = iter->Y;
      }
    }
}


pqPiecewiseLine::pqPiecewiseLine(QObject *parent)
  : pqLinePlot(parent)
{
  this->Data = new pqPiecewiseLineData();
}

pqPiecewiseLine::~pqPiecewiseLine()
{
  if(this->Data)
    delete this->Data;
}

int pqPiecewiseLine::getCoordinateCount() const
{
  if(this->Data)
    return this->Data->coords.getSize();
  return 0;
}

bool pqPiecewiseLine::getCoordinate(int index, pqChartCoordinate &coord) const
{
  if(index >= 0 && index < this->Data->coords.getSize())
    {
    coord = this->Data->coords[index];
    return true;
    }

  return false;
}

void pqPiecewiseLine::getMaxX(pqChartValue &value) const
{
  if(this->Data)
    value = this->Data->max.X;
  else
    value = (int)0;
}

void pqPiecewiseLine::getMinX(pqChartValue &value) const
{
  if(this->Data)
    value = this->Data->min.X;
  else
    value = (int)0;
}

void pqPiecewiseLine::getMaxY(pqChartValue &value) const
{
  if(this->Data)
    value = this->Data->max.Y;
  else
    value = (int)0;
}

void pqPiecewiseLine::getMinY(pqChartValue &value) const
{
  if(this->Data)
    value = this->Data->min.Y;
  else
    value = (int)0;
}

void pqPiecewiseLine::setCoordinates(const pqChartCoordinateList &list)
{
  if(!this->Data)
    return;

  // Copy the new data from the list. Find the new bounding rectangle.
  this->Data->coords = list;
  this->Data->calculateBounds();

  // Signal the line chart that a change has been made.
  this->setModified(true);
}


