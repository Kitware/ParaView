/*!
 * \file pqLineChart.h
 *
 * \brief
 *   The pqLineChart class is used to display a line plot.
 *
 * \author Mark Richardson
 * \date   August 1, 2005
 */

#ifndef _pqLineChart_h
#define _pqLineChart_h

#include "pqChartExport.h"
#include <QObject>
#include <QRect> // Needed for bounds member.

class pqChartAxis;
class pqChartCoordinate;
class pqAbstractPlot;
class QPainter;
class QHelpEvent;

/// \class pqLineChart
/// \brief
///   The pqLineChart class is used to display a line plot.
///
/// The line chart can display more than one line plot on the
/// same chart. The line chart can also be drawn over a
/// histogram as well. When combined with another chart, the
/// x-axis should be marked as a shared axis.
class QTCHART_EXPORT pqLineChart :
  public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a line chart instance.
  /// \param parent The parent object.
  pqLineChart(QObject *parent=0);
  virtual ~pqLineChart();

  /// \name Data Methods
  //@{
  /// \brief
  ///   Sets the axes for the chart.
  ///
  /// The axes must be set before the chart data can be set. If
  /// the x-axis is shared by another chart, the lie chart will
  /// not modify the x-axis.
  ///
  /// \param xAxis The x-axis object.
  /// \param yAxis The y-axis object.
  /// \param shared True if the x-axis is shared by another chart.
  void setAxes(pqChartAxis *xAxis, pqChartAxis *yAxis,
      bool shared=false);

  /// Removes any line plots current tied to the chart.  Use the
  /// \c addData method to add plots to the chart.
  void clearData();

  /// \brief
  ///   Adds another line plot to the chart.
  /// \param plot The line plot to add.
  void addData(pqAbstractPlot *plot);

  /// \brief
  ///   Removes the specified line plot from the chart.
  /// \param plot The line plot to remove.
  void removeData(pqAbstractPlot *plot);
  //@}

  /// \name Layout Methods
  //@{
  /// \brief
  ///   Used to layout the line chart.
  ///
  /// The chart axes must be layed out before this method is called.
  /// This method must be called before the chart can be drawn.
  ///
  /// \sa pqLineChart::drawChart(QPainter *, const QRect &)
  void layoutChart();

  /// \brief
  ///   Used to draw the line chart.
  ///
  /// The line chart needs to be layed out before it can be drawn.
  /// Separating the layout and drawing functions improves the
  /// repainting performance.
  ///
  /// \param painter The painter to use.
  /// \param area The area that needs to be painted.
  void drawChart(QPainter& painter, const QRect& area);
  //@}

  /// Displays a tooltip based on the position of the given event, relative to the chart data
  void showTooltip(QHelpEvent& event);

signals:
  /// Called when the line chart needs to be layed-out
  void layoutNeeded();
  /// Called when the line chart needs to be repainted.
  void repaintNeeded();

private:
  /// Internal implementation detail
  void updateAxes();

public:
  QRect Bounds;          ///< Stores the chart area.

private:
  /// Private implementation details
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
