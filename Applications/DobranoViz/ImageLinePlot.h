#ifndef _ImageLinePlot_h
#define _ImageLinePlot_h

#include <pqLinePlot.h>
#include <pqLineErrorPlot.h>

#include <QPixmap>

/// Line plot that displays an (optional) image as its tooltip (instead of the default data tooltip)
class ImageLinePlot :
  public pqLinePlot
{
public:
  ImageLinePlot(pqMarkerPen* pen, const pqChartCoordinateList& coords, const QPixmap& image);

  virtual void showChartTip(QHelpEvent& event) const;
  
private:
  QPixmap Image;
};

/// Line plot with error bars that displays an (optional) image as its tooltip (instead of the default data tooltip)
class ImageLineErrorPlot :
  public pqLineErrorPlot
{
public:
  ImageLineErrorPlot(pqMarkerPen* pen, const QPen& whisker_pen, double whisker_size, const pqLineErrorPlot::CoordinatesT& coords, const QPixmap& image);

  virtual void showChartTip(QHelpEvent& event) const;
  
private:
  QPixmap Image;
};

#endif
