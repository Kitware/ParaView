/*=========================================================================

   Program: ParaView
   Module:    pqHistogramChart.h

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
 * \file pqHistogramChart.h
 *
 * \brief
 *   The pqHistogramChart class is used to display a histogram chart.
 *
 * \author Mark Richardson
 * \date   May 12, 2005
 */

#ifndef _pqHistogramChart_h
#define _pqHistogramChart_h


#include "QtChartExport.h"
#include <QObject>

#include "pqHistogramSelectionModel.h" // Needed for list type.
#include <QColor> // Needed for select member.
#include <QRect>  // Needed for bounds member.

class pqChartAxis;
class pqChartValue;
class pqChartValueList;
class pqHistogramChartData;
class pqHistogramColor;
class pqHistogramModel;
class QPainter;


/// \class pqHistogramChart
/// \brief
///   The pqHistogramChart class is used to draw a histogram chart.
///
/// The histogram chart uses two axes to lay out the histogram bars.
/// The histogram can have a highlighted selection of bars or values.
/// The outline style determines what color the bar outline is. The
/// outline can be black or a dark color based on the bar color. The
/// highlight style determines how the bars will be highlighted when
/// they are selected. Either the outline or the filled portion of
/// the bar can be highlighted. The default settings are as follows:
///   - highlight style: \c Fill
///   - outline style: \c Darker
///   - selection background: \c LightBlue
class QTCHART_EXPORT pqHistogramChart : public QObject
{
  Q_OBJECT

public:
  enum OutlineStyle {
    Darker,
    Black
  };

  enum HighlightStyle {
    Outline,
    Fill
  };

public:
  /// \brief
  ///   Creates a histogram chart instance.
  /// \param parent The parent object.
  pqHistogramChart(QObject *parent=0);
  virtual ~pqHistogramChart();

  /// \name Data Methods
  //@{
  /// \brief
  ///   Sets the axes for the chart.
  /// \param xAxis The x-axis object.
  /// \param yAxis The y-axis object.
  void setAxes(pqChartAxis *xAxis, pqChartAxis *yAxis);

  /// \brief
  ///   Sets the histogram model to be displayed.
  /// \param model A pointer to the new histogram model.
  void setModel(pqHistogramModel *model);

  /// \brief
  ///   Gets the current histogram model.
  /// \return
  ///   A pointer to the currect histogram model.
  pqHistogramModel *getModel() const {return this->Model;}

  /// \brief
  ///   Gets the pixel width of a bin on the chart.
  /// \return
  ///   The pixel width of a bin on the chart.
  int getBinWidth() const;
  //@}

  /// \name Selection Methods
  //@{
  /// \brief
  ///   Gets the selection model for the histogram.
  /// \return
  ///   A pointer to the histogram selection model.
  pqHistogramSelectionModel *getSelectionModel() const {return this->Selection;}

  /// \brief
  ///   Gets the bin index for the given location.
  ///
  /// If there is no bin in the given location, -1 is returned.
  ///  
  /// \param x The x pixel coordinate.
  /// \param y The y pixel coordinate.
  /// \param entireBin Test the location against the entire bin, not just the "filled" portion.
  /// \return
  ///   The bin index for the given location.
  /// \sa pqHistogramChart::getBinsIn(const QRect &,
  ///         pqHistogramSelectionList &)
  int getBinAt(int x, int y, bool entireBin = true) const;

  /// \brief
  ///   Gets the value for the given location.
  /// \param x The x pixel coordinate.
  /// \param y The y pixel coordinate.
  /// \param value Used to return the value.
  /// \return
  ///   True if the x,y coordinates are in the chart area.
  /// \sa pqHistogramChart::getValuesIn(const QRect &,
  ///         pqHistogramSelectionList &)
  bool getValueAt(int x, int y, pqChartValue &value) const;

  /// \brief
  ///   Gets the value selection range for the given location.
  ///
  /// \param x The x pixel coordinate.
  /// \param y The y pixel coordinate.
  /// \param range Used to return the value selection.
  /// \return
  ///   True if the x,y coordinates are in a value selection range.
  bool getValueRangeAt(int x, int y, pqHistogramSelection &range) const;

  /// \brief
  ///   Gets a list of bin indexes within the given area.
  /// \param area The area of the chart to query.
  /// \param list Used to return the list of index ranges.
  /// \param entireBins Test the area for intersection with entire bins, not just the "filled" portions.
  /// \sa pqHistogramChart::getBinAt(int, int)
  void getBinsIn(const QRect &area, pqHistogramSelectionList &list, bool entireBins = true) const;

  /// \brief
  ///   Gets a list of values within the given area.
  /// \param area The area of the chart to query.
  /// \param list Used to return the list of value ranges.
  /// \sa pqHistogramChart::getValueAt(int, int)
  void getValuesIn(const QRect &area, pqHistogramSelectionList &list) const;
  //@}

  /// \name Drawing Parameters
  //@{
  /// \brief
  ///   Sets the histogram bar color scheme.
  /// \param scheme The color scheme interface to use.
  void setBinColorScheme(pqHistogramColor *scheme);

  /// \brief
  ///   Sets the highlight style for the histogram bars.
  ///
  /// The default style is \c Fill.
  ///
  /// \param style The highlight style to use.
  void setBinHighlightStyle(HighlightStyle style);

  /// \brief
  ///   Gets the highlight style for the histogram bars.
  /// \return
  ///   The current bin highlight style.
  HighlightStyle getBinHighlightStyle() const {return this->Style;}

  /// \brief
  ///   Sets the outline style for the histogram bars.
  ///
  /// The default style is \c Darker.
  ///
  /// \param style The outline style to use.
  void setBinOutlineStyle(OutlineStyle style);

  /// \brief
  ///   Gets the outline style for the histogram bars.
  /// \return
  ///   The current bin outline style.
  OutlineStyle getBinOutlineStyle() const {return this->OutlineType;}
  //@}

  /// \name Display Methods
  //@{
  /// \brief
  ///   Used to layout the histogram.
  ///
  /// The chart axes must be layed out before this method is called.
  /// This method must be called before the chart can be drawn.
  ///
  /// \sa pqHistogramChart::setAxes(pqChartAxis *, pqChartAxis *)
  ///     pqHistogramChart::drawChart(QPainter *, const QRect &,
  ///         QHistogramColorParams *)
  void layoutChart();

  /// \brief
  ///   Draws the highlight background for the chart.
  ///
  /// The highlight background is drawn separately so it can be
  /// drawn before the background grid.
  ///
  /// \param p The painter to use.
  /// \param area The area that needs to be painted.
  /// \sa pqHistogramChart::drawChart(QPainter *, const QRect &,
  ///         QHistogramColorParams *)
  void drawBackground(QPainter *p, const QRect &area);

  /// \brief
  ///   Used to draw the histogram chart.
  ///
  /// The histogram chart needs to be layed out before it can be
  /// drawn. Separating the layout and drawing functions improves
  /// the repainting performance.
  ///
  /// \param p The painter to use.
  /// \param area The area that needs to be painted.
  /// \sa pqHistogramChart::layoutChart()
  void drawChart(QPainter *p, const QRect &area);
  //@}

signals:
  /// \brief
  ///   Called when the chart needs to be layed out again.
  void layoutNeeded();

  /// \brief
  ///   Called when the chart needs to be repainted.
  void repaintNeeded();

private slots:
  /// Updates the layout of the histogram when the model is reset.
  void handleModelReset();

  /// \brief
  ///   Starts the histogram bin insertion process.
  /// \param first The first index of the new bins.
  /// \param last The last index of the new bins.
  void startBinInsertion(int first, int last);

  /// Finishes the histogram bin insertion process.
  void finishBinInsertion();

  /// \brief
  ///   Starts the histogram bin removal process.
  /// \param first The first index of the old bins.
  /// \param last The last index of the old bins.
  void startBinRemoval(int first, int last);

  /// Finishes the histogram bin removal process.
  void finishBinRemoval();

  /// \brief
  ///   Updates the histogram highlights when the selection changes.
  /// \param list The list of selected ranges.
  void updateHighlights(const pqHistogramSelectionList &list);

private:
  /// Used to layout the selection list.
  void layoutSelection();

  /// Used to update the axis ranges when the model changes.
  void updateAxisRanges();

  /// Used to update the x-axis range when the model changes.
  void updateXAxisRange();

  /// Used to update the y-axis range when the model changes.
  void updateYAxisRange();

  /// Cleans up the bar and highlight lists.
  void clearData();

public:
  QRect Bounds;              ///< Stores the chart area.

  static QColor LightBlue;   ///< Defines the default selection background.

private:
  HighlightStyle Style;       ///< Stores the highlight style.
  OutlineStyle OutlineType;   ///< Stores the outline style.
  QColor Select;              ///< Stores the highlight background color.
  pqHistogramColor *Colors;   ///< A pointer to the bar color scheme.
  pqChartAxis *XAxis;         ///< Stores the x-axis object.
  pqChartAxis *YAxis;         ///< Stores the y-axis object.
  pqHistogramModel *Model;    ///< A pointer to the histogram model.
  pqHistogramChartData *Data; ///< Stores the bar and highlight lists.

  /// Stores the selection model.
  pqHistogramSelectionModel *Selection;

  /// True if selection change is due to a model change.
  bool InModelChange;
};

#endif
