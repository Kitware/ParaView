/*=========================================================================

   Program: ParaView
   Module:    pqSimpleLineChartPlot.h

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

#ifndef _pqSimpleLineChartPlot_h
#define _pqSimpleLineChartPlot_h

#include "QtChartExport.h"
#include "pqLineChartPlot.h"

class pqChartCoordinate;
class pqChartValue;
class pqSimpleLineChartPlotInternal;

/// \class pqSimpleLineChartPlot
/// \brief
///   The pqSimpleLineChartPlot class is a generic line chart plot.
///
/// The simple line chart plot can be used to make a scatter plot, a
/// line plot, or an error line plot. It stores each series in a
/// separate list and doesn't take advantage of the multi-series
/// change feature.
class QTCHART_EXPORT pqSimpleLineChartPlot : public pqLineChartPlot
{
public:
  /// \brief
  ///   Creates a line chart plot object.
  /// \param parent The parent object.
  pqSimpleLineChartPlot(QObject *parent=0);
  virtual ~pqSimpleLineChartPlot();

  /// \name pqLineChartPlot Methods
  //@{
  virtual int getNumberOfSeries() const;
  virtual int getTotalNumberOfPoints() const;
  virtual pqLineChartPlot::SeriesType getSeriesType(int series) const;
  virtual int getNumberOfPoints(int series) const;
  virtual void getPoint(int series, int index, pqChartCoordinate &coord) const;
  virtual void getErrorBounds(int series, int index, pqChartValue &upper,
      pqChartValue &lower) const;
  virtual void getErrorWidth(int series, pqChartValue &width) const;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const;
  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const;
  //@}

  /// \name Data Setup Methods
  //@{
  /// Clears all the series and points from the plot.
  void clearPlot();

  /// \brief
  ///   Adds a new series to the plot.
  /// \param type The type of series to create.
  void addSeries(pqLineChartPlot::SeriesType type);

  /// \brief
  ///   Inserts a new series in the plot list.
  /// \param index Where to insert the series.
  /// \param type The type of series to create.
  void insertSeries(int index, pqLineChartPlot::SeriesType type);

  /// \brief
  ///   Sets the type for the given series.
  /// \param series The index of the series to change.
  /// \param type The type of series.
  void setSeriesType(int series, pqLineChartPlot::SeriesType type);

  /// \brief
  ///   Removes a series from the plot.
  /// \param series The index of the series to remove.
  void removeSeries(int series);

  /// \brief
  ///   Copies the point data from one series to another.
  /// \param source The index of the source series.
  /// \param destination The index of the destination series.
  void copySeriesPoints(int source, int destination);

  /// \brief
  ///   Adds a point to the given series.
  /// \param series The index of the series.
  /// \param coord The coordinates of the new point.
  void addPoint(int series, const pqChartCoordinate &coord);

  /// \brief
  ///   Inserts a point in the given series.
  /// \param series The index of the series.
  /// \param index Where to insert the point.
  /// \param coord The coordinates of the new point.
  void insertPoint(int series, int index, const pqChartCoordinate &coord);

  /// \brief
  ///   Removes the indicated point from the given series.
  /// \param series The index of the series.
  /// \param index The index of the point to remove.
  void removePoint(int series, int index);

  /// \brief
  ///   Removes all the points for a given series.
  /// \param series The index of the series to clear.
  void clearPoints(int series);

  /// \brief
  ///   Sets the error bounds for an error series point.
  /// \param series The index of the series.
  /// \param index The index of the point in the series.
  /// \param upper The upper error bound.
  /// \param lower The lower error bound.
  /// \sa pqLineChartPlot::getErrorBounds(int, int, pqChartValue &,
  ///         pqChartValue &)
  void setErrorBounds(int series, int index, const pqChartValue &upper,
      const pqChartValue &lower);

  /// \brief
  ///   Sets the error bar width for a series.
  /// \param series The index of the series.
  /// \param width The width of the error bar from the vertical line
  ///   to the end.
  /// \sa pqLineChartPlot::getErrorWidth(int, pqChartValue &)
  void setErrorWidth(int series, const pqChartValue &width);
  //@}

  /// \brief
  ///   Get the drawing color for a series.
  /// \param series The index of the series.
  /// \return
  ///   The drawing color for a series.
  virtual QColor getColor(int series) const;

  /// \brief
  ///   Set the drawing color for a series.
  /// \param series The index of the series.
  /// \param color The color for the series.
  void setColor(int series, const QColor& color);
private:
  /// Compiles the overall plot range from the points.
  void updatePlotRanges();

  /// \brief
  ///   Updates the plot range after a point addition or insertion.
  ///
  /// This method can only be called after a adding a point to the
  /// plot. In this case, the plot ranges can only grow not shrink.
  ///
  /// \param coord The newly added point.
  void updatePlotRanges(const pqChartCoordinate &coord);

private:
  pqSimpleLineChartPlotInternal *Internal; ///< Stores the plot data.
};

#endif
