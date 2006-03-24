/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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


#include "pqChartExport.h"
#include <QObject>

#include <QColor> // Needed for select member.
#include <QRect>  // Needed for bounds member.

class pqChartAxis;
class pqChartValue;
class pqChartValueList;
class pqHistogramChartData;
class pqHistogramColor;
class pqHistogramSelection;
class pqHistogramSelectionList;
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
  ///
  /// The axes must be set before the chart data can be set.
  ///
  /// \param xAxis The x-axis object.
  /// \param yAxis The y-axis object.
  void setAxes(pqChartAxis *xAxis, pqChartAxis *yAxis);

  /// \brief
  ///   Used to set up the histogram data.
  ///
  /// The number of items in the value list determines the number
  /// of bars on the histogram chart. The horizontal axis is set up
  /// based on the \c min and \c max parameters. The interval
  /// value on the horizontal axis is calculated using the minumum,
  /// the maximum, and the number of values in the list. The
  /// limits of the vertical axis are determined from the minimum
  /// and maximum values in the list.
  ///
  /// \param values A list of values. One for each histogram bar.
  /// \param min The minimum value for the histogram axis.
  /// \param max The maximum value for the histogram axis.
  /// \sa pqHistogramChart::setAxes(pqChartAxis *, pqChartAxis *)
  void setData(const pqChartValueList &values, const pqChartValue &min,
      const pqChartValue &max);

  /// Clears data
  void clearData();

  /// \brief
  ///   Gets the number of bins on the histogram.
  /// \return
  ///   The number of bins on the histogram.
  int getBinCount() const;

  /// \brief
  ///   Gets the index of the last bin on the histogram.
  /// \return
  ///   The index of the last bin on the histogram.
  int getLastBin() const;

  /// \brief
  ///   Gets the pixel width of a bin on the chart.
  /// \return
  ///   The pixel width of a bin on the chart.
  int getBinWidth() const;
  //@}

  /// \name Selection Methods
  //@{
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

  /// \brief
  ///   Gets whether or not the chart has selected range(s).
  /// \return
  ///   True if there is a selection.
  bool hasSelection() const;

  /// \brief
  ///   Gets the current selection.
  /// \param list Used to return the selection.
  /// \sa pqHistogramChart::hasSelection()
  void getSelection(pqHistogramSelectionList &list) const;

  /// Selects all the bins on the chart.
  void selectAllBins();

  /// Selects all the values on the chart.
  void selectAllValues();

  /// Clears the selection.
  void selectNone();

  /// Inverts the selection.
  void selectInverse();

  /// \brief
  ///   Sets the selection to the specified range(s).
  /// \param list The list of selection range(s).
  void setSelection(const pqHistogramSelectionList &list);

  /// \brief
  ///   Adds the specified range(s) to the selection.
  ///
  /// Each range in the list must be the same type of selection as
  /// the current selection list. The range(s) in the supplied list
  /// are merged with the current selection, which may cause items
  /// to be removed.
  ///
  /// \param list The list of selection range(s).
  /// \sa pqHistogramSelection::unite(pqHistogramSelection *,
  ///         pqHistogramSelectionList &)
  void addSelection(const pqHistogramSelectionList &list);

  /// \brief
  ///   Performs an exclusive or between the specified range(s) and
  ///   the selection.
  ///
  /// Each range in the list must be the same type of selection as
  /// the current selection list. The range(s) in the supplied list
  /// are xored with the current selection, which may cause items
  /// to be removed.
  ///
  /// \param list The list of selection range(s).
  /// \sa pqHistogramSelection::Xor(pqHistogramSelection *,
  ///         pqHistogramSelectionList &)
  void xorSelection(const pqHistogramSelectionList &list);

  /// \brief
  ///   Subtracts the specified range(s) from the selection.
  ///
  /// Each range in the list must be the same type of selection as
  /// the current selection list. The range(s) in the supplied list
  /// are removed from the current selection, which may cause items
  /// to be removed.
  ///
  /// \param list The list of selection range(s).
  /// \sa pqHistogramSelection::unite(pqHistogramSelection *,
  ///         pqHistogramSelectionList &)
  void subtractSelection(const pqHistogramSelectionList &list);

  /// \brief
  ///   Sets the selection to the specified range.
  /// \param range The selection range.
  void setSelection(const pqHistogramSelection *range);

  /// \brief
  ///   Adds the specified range to the selection.
  /// \param range The selection range.
  /// \sa pqHistogramChart::addSelection(const pqHistogramSelectionList &)
  void addSelection(const pqHistogramSelection *range);

  /// \brief
  ///   Performs an exclusive or between the specified range and
  ///   the selection.
  /// \param range The selection range.
  /// \sa pqHistogramChart::xorSelection(const pqHistogramSelectionList &)
  void xorSelection(const pqHistogramSelection *range);

  /// \brief
  ///   Moves the selection range if it exists.
  ///
  /// After finding the selection range, the offset will be added to
  /// both ends of the range. The new range will be adjusted to fit
  /// within the bounds of the chart. The new, adjusted range will be
  /// united with the remaining selections in the list. In other words,
  /// it is possible for the range to become shorter and/or combined
  /// with overlapping selection ranges.
  ///
  /// \param range The selection range to move.
  /// \param offset The amount to move the selection range.
  void moveSelection(const pqHistogramSelection &range,
      const pqChartValue &offset);
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
  ///   Called when the chart needs to be repainted.
  void repaintNeeded();

  /// \brief
  ///   Called when the selection has changed.
  /// \param list The new selection.
  void selectionChanged(const pqHistogramSelectionList &list);

private:
  /// Used to layout the selection list.
  void layoutSelection();

  /// Cleans up the bin and selection lists.
  void resetData();

public:
  QRect Bounds;              ///< Stores the chart area.

  static QColor LightBlue;   ///< Defines the default selection background.

private:
  HighlightStyle Style;       ///< Stores the highlight style.
  OutlineStyle OutlineType;   ///< Stores the outline style.
  QColor Select;              ///< Stores the selection background color.
  pqHistogramColor *Colors;   ///< A pointer to the bar color scheme.
  pqChartAxis *XAxis;         ///< Stores the x-axis object.
  pqChartAxis *YAxis;         ///< Stores the y-axis object.
  pqHistogramChartData *Data; ///< Stores the bar and selection lists.
};

#endif
