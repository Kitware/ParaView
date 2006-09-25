/*=========================================================================

   Program: ParaView
   Module:    pqLineChart.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

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

#include "QtChartExport.h"
#include <QObject>
#include <QRect> // Needed for bounds member.

class pqChartAxis;
class pqChartCoordinate;
class pqLineChartInternal;
class pqLineChartItem;
class pqLineChartModel;
class pqLineChartPlot;
class pqLineChartPlotOptions;
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
class QTCHART_EXPORT pqLineChart : public QObject
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
  /// If the x-axis is shared by another chart, the line chart will
  /// not modify the x-axis.
  ///
  /// \param xAxis The x-axis object.
  /// \param yAxis The y-axis object.
  /// \param shared True if the x-axis is shared by another chart.
  void setAxes(pqChartAxis *xAxis, pqChartAxis *yAxis,
      bool shared=false);

  /// \brief
  ///   Sets the line chart model to be displayed.
  /// \param model A pointer to the new line chart model.
  void setModel(pqLineChartModel *model);

  /// \brief
  ///   Gets the current line chart model.
  /// \return
  ///   A pointer to the currect line chart model.
  pqLineChartModel *getModel() const {return this->Model;}
  //@}

  /// \name Drawing Parameters
  //@{
  /// \brief
  ///   Sets the drawing parameters for a plot.
  /// \param options The plot options object.
  /// \param plot The plot index to set.
  void setPlotOptions(pqLineChartPlotOptions *options, int plot);

  /// \brief
  ///   Gets the drawing parameters for a plot.
  /// \param plot The plot index to look up.
  /// \return
  ///   A pointer to the drawing parameters for a plot.
  pqLineChartPlotOptions *getPlotOptions(int plot) const;

  /// Clears the list of plot options.
  void clearPlotOptions();
  //@}

  /// \name Display Methods
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
  void drawChart(QPainter &painter, const QRect &area);
  //@}

  /// Displays a tooltip based on the position of the given event, relative to the chart data
  //void showTooltip(QHelpEvent *event);

signals:
  /// Called when the line chart needs to be layed-out
  void layoutNeeded();

  /// Called when the line chart needs to be repainted.
  void repaintNeeded();

private slots:
  /// Updates the layout of the line chart when the model is reset.
  void handleModelReset();

  void startPlotInsertion(int first, int last);
  void finishPlotInsertion(int first, int last);
  void startPlotRemoval(int first, int last);
  void finishPlotRemoval(int first, int last);

  void handlePlotMoved(int current, int index);
  void handlePlotReset(const pqLineChartPlot *plot);

  void startPointInsertion(const pqLineChartPlot *plot, int series, int first,
      int last);
  void finishPointInsertion(const pqLineChartPlot *plot, int series);
  void startPointRemoval(const pqLineChartPlot *plot, int series, int first,
      int last);
  void finishPointRemoval(const pqLineChartPlot *plot, int series);
  void startMultiSeriesChange(const pqLineChartPlot *plot);
  void finishMultiSeriesChange(const pqLineChartPlot *plot);
  void handlePlotErrorBoundsChanged(const pqLineChartPlot *plot, int series,
      int first, int last);
  void handlePlotErrorWidthChanged(const pqLineChartPlot *plot, int series);

private:
  /// \brief
  ///   Updates the axis ranges when the model changes.
  /// \param force True if the ranges should be set unchecked.
  void updateAxisRanges(bool force=true);

  /// Removes all the line plots from the chart.
  void clearData();

  /// Builds the plot list when the model is reset.
  void buildPlotList();

  /// \brief
  ///   Gets the internal line chart item for the given plot.
  /// \param plot The plot to look up.
  /// \return
  ///   A pointer to the internal chart item for the given plot.
  pqLineChartItem *getItem(const pqLineChartPlot *plot) const;

public:
  QRect Bounds;                  ///< Stores the chart area.

private:
  pqLineChartInternal *Internal; ///< Stores the plot layout.
  pqLineChartModel *Model;       ///< A pointer to the model.
  pqChartAxis *XAxis;            ///< A pointer to the x-axis.
  pqChartAxis *YAxis;            ///< A pointer to the y-axis.
  bool XShared;                  ///< True if the x-axis is shared.
  bool NeedsLayout;              ///< True if a chart layout is needed.
};

#endif
