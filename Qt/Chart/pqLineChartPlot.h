/*=========================================================================

   Program: ParaView
   Module:    pqLineChartPlot.h

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

/// \file pqLineChartPlot.h
/// \date 9/7/2006

#ifndef _pqLineChartPlot_h
#define _pqLineChartPlot_h


#include "QtChartExport.h"
#include <QObject>

class pqChartCoordinate;
class pqChartValue;
class QColor;


/// \class pqLineChartPlot
/// \brief
///   The pqLineChartPlot class is the base class for all line chart
///   plots.
///
/// The pqLineChartModel class uses the %pqLineChartPlot interface to
/// define the individual line plots in the model. This class should
/// be extended to create a line plot for the line chart model.
///
/// The line chart plot is composed of one or more point series. Each
/// point series is painted in the order it is stored. Meaning, the
/// first series is painted first, and the last series is painted
/// last. Each series has a type associated with it so the chart can
/// determine how to draw it.
///
/// To create a scatter plot, the plot would only need one series of
/// points. To create a line plot, the plot would need two series.
/// One for the line and one for the points. The points should be
/// after the line in order to be drawn on top of the line. An error
/// plot would need three series: one for the error bounds, one for
/// the line, and one for the points.
class QTCHART_EXPORT pqLineChartPlot : public QObject
{
  Q_OBJECT

public:
  enum SeriesType
    {
    Invalid, ///< Indicates the point series type has not been set.
    Point,   ///< The point series should be used to draw points.
    Line,    ///< The point series should be used to draw a line.
    Error    ///< The point series contains error bounds information.
    };

public:
  /// \brief
  ///   Creates a line chart plot object.
  /// \param parent The parent object.
  pqLineChartPlot(QObject *parent=0);
  virtual ~pqLineChartPlot() {}

  /// \name Plot Data Methods
  //@{
  /// \brief
  ///   Gets the number of series in the plot.
  /// \return
  ///   The number of series in the plot.
  virtual int getNumberOfSeries() const=0;

  /// \brief
  ///   Gets the total number of points in all the plot series.
  /// \return
  ///   The total number of points in all the plot series.
  virtual int getTotalNumberOfPoints() const=0;

  /// \brief
  ///   Get the drawing type for a series.
  /// \param series The index of the series.
  /// \return
  ///   The drawing type for a series.
  virtual SeriesType getSeriesType(int series) const=0;

  /// \brief
  ///   Get the drawing color for a series.
  /// \param series The index of the series.
  /// \return
  ///   The drawing color for a series.
  virtual QColor getColor(int series) const=0;

  /// \brief
  ///   Get the number of points in a series.
  /// \param series The index of the series.
  /// \return
  ///   The number of points in a series.
  virtual int getNumberOfPoints(int series) const=0;

  /// \brief
  ///   Get the point coordinates for an index in a series.
  /// \param series The index of the series.
  /// \param index The index of the point in the series.
  /// \param coord Used to return the point's coordinates.
  virtual void getPoint(int series, int index,
      pqChartCoordinate &coord) const=0;
  //@}

  /// \name Error Series Methods
  //@{
  /// \brief
  ///   Gets the error bounds associated with a series point.
  ///
  /// The error bounds returned should be relative to the axis not
  /// the point. For example, if the point has a value of 4, and the
  /// margin of error for the point is plus or minus 0.3, the upper
  /// bound should be 4.3 and the lower bound should be 3.7.
  ///
  /// \param series The index of the series.
  /// \param index The index of the point in the series.
  /// \param upper Used to return the upper bound.
  /// \param lower Used to return the lower bound.
  virtual void getErrorBounds(int series, int index, pqChartValue &upper,
      pqChartValue &lower) const=0;

  /// \brief
  ///   Get the error bar width for the series.
  ///
  /// The error bar width returned should be half the distance accross
  /// the horizontal bar that caps the error line. The error bar width
  /// is given in chart coordinates so it will scale with the chart.
  ///
  /// \param series The index of the series.
  /// \param width Used to return the error bar width.
  virtual void getErrorWidth(int series, pqChartValue &width) const=0;
  //@}

  /// \name Plot Range Methods
  //@{
  /// \brief
  ///   Get the x-axis range for the plot.
  ///
  /// The axis range is not considered valid if the plot has no data.
  /// This allows the model to provide an accurate range for the list
  /// of plots. Adding in an empty range of [0,0] may skew the overall
  /// range and produce a difficult to read chart.
  ///
  /// \param min Used to return the minimum x-axis value.
  /// \param max Used to return the maximum x-axis value.
  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const=0;

  /// \brief
  ///   Get the y-axis range for the plot.
  /// \param min Used to return the minimum y-axis value.
  /// \param max Used to return the maximum y-axis value.
  /// \sa pqLineChartPlot::getRangeX(pqChartValue &, pqChartValue &)
  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const=0;
  //@}
 
signals:
  /// Emitted when the plot data has changed drastically.
  void plotReset();

  /// \brief
  ///   Emitted when new points will be inserted.
  /// \param series The index of the series to be modified.
  /// \param first The first index of the point insertion.
  /// \param last The last index of the point insertion.
  void aboutToInsertPoints(int series, int first, int last);

  /// \brief
  ///   Emitted when new points have been inserted.
  /// \param series The index of the modified series.
  void pointsInserted(int series);

  /// \brief
  ///   Emitted when points will be removed.
  /// \param series The index of the series to be modified.
  /// \param first The first index of the point removal.
  /// \param last The last index of the point removal.
  void aboutToRemovePoints(int series, int first, int last);

  /// \brief
  ///   Emitted when points have been removed.
  /// \param series The index of the modified series.
  void pointsRemoved(int series);

  /// \brief
  ///   Emitted when changes will be made to multiple series
  ///   simultaneously.
  /// \sa pqLineChartPlot::beginMultiSeriesChange()
  void aboutToChangeMultipleSeries();

  /// \brief
  ///   Emitted when simultaneous changes have been made to multiple
  ///   series.
  /// \sa pqLineChartPlot::beginMultiSeriesChange()
  void changedMultipleSeries();

  /// \brief
  ///   Emitted when the error bounds have changed.
  ///
  /// Every point in the error series has error bounds associated with
  /// it. Therefore, the error bounds are added and removed with the
  /// points. This signal notifies the model of a change. The change
  /// may be caused by the initial setting of the error bounds, where
  /// the error bounds change from an invalid to a valid state.
  ///
  /// \param series The index of the modified series.
  /// \param first The first index of the modified points.
  /// \param last The last index of the modified points.
  void errorBoundsChanged(int series, int first, int last);

  /// \brief
  ///   Emitted when the error bar width for a series has changed.
  /// \param series The index of the modified series.
  void errorWidthChanged(int series);

protected:
  /// Emits the plot reset signal.
  void resetPlot();

  /// \brief
  ///   Called to begin the point insertion process.
  ///
  /// This method must be called before inserting the points into the
  /// series. After the points have been inserted, the
  /// \c endInsertPoints method must be called. Each begin must have
  /// a matching end called.
  ///
  /// \param series The index of the series to be modified.
  /// \param first The first index of the point insertion.
  /// \param last The last index of the point insertion.
  /// \sa pqLineChartPlot::beginMultiSeriesChange()
  void beginInsertPoints(int series, int first, int last);

  /// \brief
  ///   Called to end the point insertion process.
  /// \param series The index of the modified series.
  /// \sa pqLineChartPlot::beginInsertPoints(int, int, int)
  void endInsertPoints(int series);

  /// \brief
  ///   Called to begin the point removal process.
  ///
  /// This method must be called before removing points from the
  /// series. After the points have been removed, the
  /// \c endRemovePoints method must be called. Each begin must have
  /// a matching end called.
  ///
  /// \param series The index of the series to be modified.
  /// \param first The first index of the point removal.
  /// \param last The last index of the point removal.
  /// \sa pqLineChartPlot::beginMultiSeriesChange()
  void beginRemovePoints(int series, int first, int last);

  /// \brief
  ///   Called to end the point removal process.
  /// \param series The index of the modified series.
  /// \sa pqLineChartPlot::beginRemovePoints(int, int, int)
  void endRemovePoints(int series);

  /// \brief
  ///   Called to begin a multi-series change.
  ///
  /// A plot can use one array to store the data for multiple series.
  /// In this case, making a change to one series will change another
  /// series at the same time. To avoid accessing invalid data, the
  /// line chart allows for a multi-series change. The procedure is as
  /// follows:
  ///   \li Call \c beginMultiSeriesChange.
  ///   \li Call begin insert/remove for each affected series.
  ///   \li Make the change to the underlying data.
  ///   \li Call end insert/remove for each affected series.
  ///   \li Call \c endMultiSeriesChange.
  ///
  /// An examle of this is a line plot that draws a point on every
  /// line segment intersection. The line plot uses one list to store
  /// the points for a \c Line series and a \c Point series. In order
  /// to add or remove points, the multi-series change mechanism would
  /// have to be used.
  void beginMultiSeriesChange();

  /// Called to end a multi-series change.
  void endMultiSeriesChange();
};

#endif
