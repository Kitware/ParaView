#ifndef _pqLinePlot_h
#define _pqLinePlot_h

#include "pqChartExport.h"
#include "pqAbstractPlot.h"

class pqMarkerPen;

/// Displays a line plot
class QTCHART_EXPORT pqLinePlot :
  public pqAbstractPlot
{
public:
  /// pqLinePlot assumes ownership of the given pen
  pqLinePlot(pqMarkerPen* pen, const pqChartCoordinateList& coords);
  /// pqLinePlot assumes ownership of the given pen
  pqLinePlot(pqMarkerPen* pen, const pqChartCoordinate& p1, const pqChartCoordinate& p2);
  virtual ~pqLinePlot();

  /// \name pqAbstractPlot Methods
  //@{
  virtual const pqChartCoordinate getMinimum() const;
  virtual const pqChartCoordinate getMaximum() const;
  virtual void layoutPlot(const pqChartAxis& XAxis, const pqChartAxis& YAxis);
  virtual void drawPlot(QPainter& painter, const QRect& area, const pqChartAxis& XAxis, const pqChartAxis& YAxis);
  virtual const double getDistance(const QPoint& coords) const;
  virtual void showChartTip(QHelpEvent& event) const;
  //@}

private:
  /// Private implementation details
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
