#include "ImageLinePlot.h"

#include <pqImageTip.h>
#include <pqMarkerPen.h>

#include <QHelpEvent>
#include <QPen>

//////////////////////////////////////////////////////////////////////////////////////
// ImageLinePlot

ImageLinePlot::ImageLinePlot(pqMarkerPen* pen, const pqChartCoordinateList& coords, const QPixmap& image) :
  pqLinePlot(pen, coords),
  Image(image)
{
}

void ImageLinePlot::showChartTip(QHelpEvent& event) const
{
  if(this->Image.isNull())
    {
    pqLinePlot::showChartTip(event);
    return;
    }
  
  pqImageTip::showTip(this->Image, event.globalPos());
}

//////////////////////////////////////////////////////////////////////////////////////
// ImageLineErrorPlot

ImageLineErrorPlot::ImageLineErrorPlot(pqMarkerPen* pen, const QPen& whisker_pen, double whisker_size, const pqLineErrorPlot::CoordinatesT& coords, const QPixmap& image) :
  pqLineErrorPlot(pen, whisker_pen, whisker_size, coords),
  Image(image)
{
}

void ImageLineErrorPlot::showChartTip(QHelpEvent& event) const
{
  if(this->Image.isNull())
    {
    pqLineErrorPlot::showChartTip(event);
    return;
    }
  
  pqImageTip::showTip(this->Image, event.globalPos());
}
