/*!
 * \file pqScatterPlot.h
 *
 * \brief
 *   The pqScatterPlot class is used to draw a piecewise linear
 *   function.
 *
 * \author Mark Richardson
 * \date   August 22, 2005
 */

#ifndef _pqScatterPlot_h
#define _pqScatterPlot_h

#include "pqChartExport.h"
#include "pqAbstractPlot.h"

class pqChartValue;
class pqMarkerPen;

/// Displays a scatter-plot of X/Y coordinates
class QTCHART_EXPORT pqScatterPlot :
  public pqAbstractPlot
{
public:
  /// Assumes ownership of the given pen
  pqScatterPlot(pqMarkerPen* pen, const pqChartCoordinateList& coords);
  virtual ~pqScatterPlot();

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
