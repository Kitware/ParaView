/*!
 * \file pqHistogramColor.cxx
 *
 * \brief
 *   The pqHistogramColor and QHistogramColorParams classes are used to
 *   define the colors used on a pqHistogramChart.
 *
 * \author Mark Richardson
 * \date   May 18, 2005
 */

#include "pqHistogramColor.h"


QColor pqHistogramColor::getColor(int index, int total) const
{
  // Get a color that is on the pure hue line of the rgb color
  // cube. Use the index/total ratio to find the color. Only use
  // the hues from red to blue.
  QColor color;
  if(--total > 0)
    {
    int hueTotal = 1020; // 255 * 4
    int hueValue = (hueTotal * index)/total;
    int section = hueValue/255;
    int value = hueValue % 255;
    if(section == 0)
      color.setRgb(255, value, 0);
    else if(section == 1)
      color.setRgb(255 - value, 255, 0);
    else if(section == 2)
      color.setRgb(0, 255, value);
    else if(section == 3)
      color.setRgb(0, 255 - value, 255);
    else
      color.setRgb(value, 0, 255);
    }
  else
    color = Qt::red;

  return color;
}


