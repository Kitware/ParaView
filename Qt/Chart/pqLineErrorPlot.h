#ifndef _pqLineErrorPlot_h
#define _pqLineErrorPlot_h

#include "pqChartExport.h"
#include "pqAbstractPlot.h"

#include <QVector>

class pqMarkerPen;
class QPen;

/// Displays a line plot that includes "whiskers" to visualize error bounds
class QTCHART_EXPORT pqLineErrorPlot :
  public pqAbstractPlot
{
public:
  /// Defines a 2-dimensional coordinate whose Y value is bounded by the given error values
  struct Coordinate
  {
    Coordinate()
    {
    }
    
    Coordinate(double x, double y, double upper_bound, double lower_bound) :
      X(x),
      Y(y),
      UpperBound(upper_bound),
      LowerBound(lower_bound)
    {
    }
    
    /// Stores the X value of the coordinate
    double X;
    /// Stores the Y value of the coordinate
    double Y;
    /// Stores the upper bound of the Y value, as a positive delta
    double UpperBound;
    /// Stores the lower bound of the Y value, as a *positive* delta
    double LowerBound;
  };
  /// Defines a collection of coordinates with error bounds
  typedef QVector<Coordinate> CoordinatesT;

  /// pqLineErrorPlot assumes ownership of the given pen
  pqLineErrorPlot(pqMarkerPen* pen, const QPen& whisker_pen, double whisker_size, const CoordinatesT& coords);
  virtual ~pqLineErrorPlot();

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
