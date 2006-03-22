/*!
 * \file pqChartMouseBox.cxx
 *
 * \brief
 *   The pqChartMouseBox class stores the data for a mouse zoom or
 *   selection box.
 *
 * \author Mark Richardson
 * \date   September 28, 2005
 */

#include "pqChartMouseBox.h"


pqChartMouseBox::pqChartMouseBox()
  : Box(), Last()
{
}

void pqChartMouseBox::adjustBox(const QPoint &current)
{
  // Determine the new area. The last point should be kept as one
  // of the corners.
  if(current.x() < this->Last.x())
    {
    if(current.y() < this->Last.y())
      {
      this->Box.setTopLeft(current);
      this->Box.setBottomRight(this->Last);
      }
    else
      {
      this->Box.setBottomLeft(current);
      this->Box.setTopRight(this->Last);
      }
    }
  else
    {
    if(current.y() < this->Last.y())
      {
      this->Box.setTopRight(current);
      this->Box.setBottomLeft(this->Last);
      }
    else
      {
      this->Box.setBottomRight(current);
      this->Box.setTopLeft(this->Last);
      }
    }
}

void pqChartMouseBox::resetBox()
{
  this->Box.setCoords(0, 0, 0, 0);
}


