/*!
 * \file pqAbstractPlot.h
 *
 * \brief
 *   The pqAbstractPlot class is the drawing interface to a function.
 *
 * \author Mark Richardson
 * \date   August 2, 2005
 */

#ifndef _pqAbstractPlot_h
#define _pqAbstractPlot_h

#include "pqChartCoordinate.h"
#include "pqChartExport.h"

class pqChartAxis;
class QHelpEvent;
class QPainter;
class QPoint;
class QRect;

/// \class pqAbstractPlot
/// \brief
///   The pqAbstractPlot class is the drawing interface to a function.
///
/// The pqLineChart uses this interface to draw functions. In order
/// to have a function show up on the line chart, a new class must
/// be created that inherits from %pqAbstractPlot.
class QTCHART_EXPORT pqAbstractPlot
{
public:
  virtual ~pqAbstractPlot() {}

  /// Returns the minimum coordinates for the plot
  virtual const pqChartCoordinate getMinimum() const = 0;
  /// Returns the maximum coordinates for the plot
  virtual const pqChartCoordinate getMaximum() const = 0;
  /// Gives the plot a chance to update its layout (e.g. create an internal cache of screen coordinates)
  virtual void layoutPlot(const pqChartAxis& XAxis, const pqChartAxis& YAxis) = 0;
  /// Gives the plot a chance to draw itself
  virtual void drawPlot(QPainter& painter, const QRect& area, const pqChartAxis& XAxis, const pqChartAxis& YAxis) = 0;
  /// Returns a distance metric from a point in screen coordinates to the plot (used to pick a plot to display chart tips)
  virtual const double getDistance(const QPoint& coords) const = 0;
  /// Displays a "chart tip" for the given screen coordinates (derivatives can use this to display any data they wish)
  virtual void showChartTip(QHelpEvent& event) const = 0;
};

#endif
