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

#include <QHelpEvent>
#include <QToolTip>

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
  pqChartCoordinateList Coords; ///< Stores the coordinate list.
  pqChartCoordinate Min;        ///< Stores the minimum coordinate.
  pqChartCoordinate Max;        ///< Stores the maximum coordinate.
};


pqPiecewiseLineData::pqPiecewiseLineData()
  : Coords(), Min(), Max()
{
}

void pqPiecewiseLineData::calculateBounds()
{
  bool first = true;
  pqChartCoordinateList::Iterator iter = this->Coords.begin();
  for( ; iter != this->Coords.end(); ++iter)
    {
    if(first)
      {
      this->Min = *iter;
      this->Max = *iter;
      first = false;
      }
    else
      {
      if(iter->X > this->Max.X)
        this->Max.X = iter->X;
      else if(iter->X < this->Min.X)
        this->Min.X = iter->X;
      if(iter->Y > this->Max.Y)
        this->Max.Y = iter->Y;
      else if(iter->Y < this->Min.Y)
        this->Min.Y = iter->Y;
      }
    }
}


pqPiecewiseLine::pqPiecewiseLine(QObject *p)
  : pqLinePlot(p)
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
    return this->Data->Coords.getSize();
  return 0;
}

bool pqPiecewiseLine::getCoordinate(int index, pqChartCoordinate &coord) const
{
  if(index >= 0 && index < this->Data->Coords.getSize())
    {
    coord = this->Data->Coords[index];
    return true;
    }

  return false;
}

void pqPiecewiseLine::getMaxX(pqChartValue &value) const
{
  if(this->Data)
    value = this->Data->Max.X;
  else
    value = (int)0;
}

void pqPiecewiseLine::getMinX(pqChartValue &value) const
{
  if(this->Data)
    value = this->Data->Min.X;
  else
    value = (int)0;
}

void pqPiecewiseLine::getMaxY(pqChartValue &value) const
{
  if(this->Data)
    value = this->Data->Max.Y;
  else
    value = (int)0;
}

void pqPiecewiseLine::getMinY(pqChartValue &value) const
{
  if(this->Data)
    value = this->Data->Min.Y;
  else
    value = (int)0;
}

void pqPiecewiseLine::showTooltip(int index, QHelpEvent& event) const
{
  if(!this->Data)
    return;
    
  pqChartCoordinate data;
  if(!this->getCoordinate(index, data))
    return;
    
  QToolTip::showText(event.globalPos(), QString("%1, %2").arg(data.X.getDoubleValue()).arg(data.Y.getDoubleValue()));
}

void pqPiecewiseLine::setCoordinates(const pqChartCoordinateList &list)
{
  if(!this->Data)
    return;

  // Copy the new data from the list. Find the new bounding rectangle.
  this->Data->Coords = list;
  this->Data->calculateBounds();

  // Signal the line chart that a change has been made.
  this->setModified(true);
}

