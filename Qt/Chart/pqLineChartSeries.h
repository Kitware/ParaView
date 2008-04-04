/*=========================================================================

   Program: ParaView
   Module:    pqLineChartSeries.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

/// \file pqLineChartSeries.h
/// \date 9/7/2006

#ifndef _pqLineChartSeries_h
#define _pqLineChartSeries_h


#include "QtChartExport.h"
#include <QObject>

class pqChartCoordinate;
class pqChartValue;

/// \class pqLineChartSeries
/// \brief
///   The pqLineChartSeries class is the base class for all line chart
///   series.
///
/// The pqLineChartModel class uses the %pqLineChartSeries interface
/// to define the individual line plots in the model. This class
/// should be extended to create a line plot for the line chart model.
///
/// The line chart series is composed of one or more point sequences.
/// Each point sequence is painted in the order it is stored. Meaning,
/// the first sequence is painted first, and the last sequence is
/// painted last. Each sequence has a type associated with it so the
/// chart can determine how to draw it.
///
/// To create a scatter plot, the series would only need one sequence
/// of points. To create a line plot, the series would need two
/// sequences. One for the line and one for the points. The points
/// should be after the line in order to be drawn on top of the line.
/// An error plot would need three sequences: one for the error bounds,
/// one for the line, and one for the points.
class QTCHART_EXPORT pqLineChartSeries : public QObject
{
  Q_OBJECT

public:
  enum SequenceType
    {
    Invalid, ///< Indicates the point sequence type has not been set.
    Point,   ///< The point sequence should be used to draw points.
    Line,    ///< The point sequence should be used to draw a line.
    Error    ///< The point sequence contains error bounds information.
    };

  enum ChartAxes
    {
    BottomLeft = 0, ///< Series uses the bottom and left axes.
    BottomRight,    ///< Series uses the bottom and right axes.
    TopLeft,        ///< Series uses the top and left axes.
    TopRight        ///< Series uses the top and right axes.
    };

public:
  /// \brief
  ///   Creates a line chart series object.
  /// \param parent The parent object.
  pqLineChartSeries(QObject *parent=0);
  virtual ~pqLineChartSeries() {}

  /// \name Series Data Methods
  //@{
  /// \brief
  ///   Gets the number of sequences in the plot.
  /// \return
  ///   The number of sequences in the plot.
  virtual int getNumberOfSequences() const=0;

  /// \brief
  ///   Gets the total number of points in all the series sequences.
  /// \return
  ///   The total number of points in all the series sequences.
  virtual int getTotalNumberOfPoints() const=0;

  /// \brief
  ///   Get the drawing type for a sequence.
  /// \param sequence The index of the sequence.
  /// \return
  ///   The drawing type for a sequence.
  virtual SequenceType getSequenceType(int sequence) const=0;

  /// \brief
  ///   Get the number of points in a sequence.
  /// \param sequence The index of the sequence.
  /// \return
  ///   The number of points in a sequence.
  virtual int getNumberOfPoints(int sequence) const=0;

  /// \brief
  ///   Get the point coordinates for an index in a sequence.
  /// \param sequence The index of the sequence.
  /// \param index The index of the point in the sequence.
  /// \param coord Used to return the point's coordinates.
  /// \return
  ///   True if the point is disconnected from the previous point.
  virtual bool getPoint(int sequence, int index,
      pqChartCoordinate &coord) const=0;
  //@}

  /// \name Error Sequence Methods
  //@{
  /// \brief
  ///   Gets the error bounds associated with a sequence point.
  ///
  /// The error bounds returned should be relative to the axis not
  /// the point. For example, if the point has a value of 4, and the
  /// margin of error for the point is plus or minus 0.3, the upper
  /// bound should be 4.3 and the lower bound should be 3.7.
  ///
  /// \param sequence The index of the sequence.
  /// \param index The index of the point in the sequence.
  /// \param upper Used to return the upper bound.
  /// \param lower Used to return the lower bound.
  virtual void getErrorBounds(int sequence, int index, pqChartValue &upper,
      pqChartValue &lower) const=0;

  /// \brief
  ///   Get the error bar width for the sequence.
  ///
  /// The error bar width returned should be half the distance accross
  /// the horizontal bar that caps the error line. The error bar width
  /// is given in chart coordinates so it will scale with the chart.
  ///
  /// \param sequence The index of the sequence.
  /// \param width Used to return the error bar width.
  virtual void getErrorWidth(int sequence, pqChartValue &width) const=0;
  //@}

  /// \name Series Range Methods
  //@{
  /// \brief
  ///   Get the x-axis range for the series.
  ///
  /// The axis range is not considered valid if the series has no data.
  /// This allows the model to provide an accurate range for the list
  /// of series. Adding in an empty range of [0,0] may skew the overall
  /// range and produce a difficult to read chart.
  ///
  /// \param min Used to return the minimum x-axis value.
  /// \param max Used to return the maximum x-axis value.
  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const=0;

  /// \brief
  ///   Get the y-axis range for the series.
  /// \param min Used to return the minimum y-axis value.
  /// \param max Used to return the maximum y-axis value.
  /// \sa pqLineChartSeries::getRangeX(pqChartValue &, pqChartValue &)
  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const=0;
  //@}

  /// \name Series Layout Methods
  //@{
  /// \brief
  ///   Gets the chart axes used by the series.
  /// \return
  ///   The chart axes used by the series.
  ChartAxes getChartAxes() const {return this->Axes;}

  /// \brief
  ///   Sets the chart axes used by the series.
  /// \param axes The new chart axes.
  void setChartAxes(ChartAxes axes);
  //@}
 
signals:
  /// Emitted when the series data has changed drastically.
  void seriesReset();

  /// \brief
  ///   Emitted when new points will be inserted.
  /// \param sequence The index of the sequence to be modified.
  /// \param first The first index of the point insertion.
  /// \param last The last index of the point insertion.
  void aboutToInsertPoints(int sequence, int first, int last);

  /// \brief
  ///   Emitted when new points have been inserted.
  /// \param sequence The index of the modified sequence.
  void pointsInserted(int sequence);

  /// \brief
  ///   Emitted when points will be removed.
  /// \param sequence The index of the sequence to be modified.
  /// \param first The first index of the point removal.
  /// \param last The last index of the point removal.
  void aboutToRemovePoints(int sequence, int first, int last);

  /// \brief
  ///   Emitted when points have been removed.
  /// \param sequence The index of the modified sequence.
  void pointsRemoved(int sequence);

  /// \brief
  ///   Emitted when changes will be made to multiple sequences
  ///   simultaneously.
  /// \sa pqLineChartSeries::beginMultiSequenceChange()
  void aboutToChangeMultipleSequences();

  /// \brief
  ///   Emitted when simultaneous changes have been made to multiple
  ///   sequences.
  /// \sa pqLineChartSeries::beginMultiSequenceChange()
  void changedMultipleSequences();

  /// \brief
  ///   Emitted when the error bounds have changed.
  ///
  /// Every point in the error sequence has error bounds associated
  /// with it. Therefore, the error bounds are added and removed with
  /// the points. This signal notifies the model of a change. The
  /// change may be caused by the initial setting of the error bounds,
  /// where the error bounds change from an invalid to a valid state.
  ///
  /// \param sequence The index of the modified sequence.
  /// \param first The first index of the modified points.
  /// \param last The last index of the modified points.
  void errorBoundsChanged(int sequence, int first, int last);

  /// \brief
  ///   Emitted when the error bar width for a sequence has changed.
  /// \param sequence The index of the modified sequence.
  void errorWidthChanged(int sequence);

  /// Emitted when the series chart axes have changed.
  void chartAxesChanged();

protected:
  /// Emits the series reset signal.
  void resetSeries();

  /// \brief
  ///   Called to begin the point insertion process.
  ///
  /// This method must be called before inserting the points into the
  /// sequence. After the points have been inserted, the
  /// \c endInsertPoints method must be called. Each begin must have
  /// a matching end called.
  ///
  /// \param sequence The index of the sequence to be modified.
  /// \param first The first index of the point insertion.
  /// \param last The last index of the point insertion.
  /// \sa pqLineChartSeries::beginMultiSequenceChange()
  void beginInsertPoints(int sequence, int first, int last);

  /// \brief
  ///   Called to end the point insertion process.
  /// \param sequence The index of the modified sequence.
  /// \sa pqLineChartSeries::beginInsertPoints(int, int, int)
  void endInsertPoints(int sequence);

  /// \brief
  ///   Called to begin the point removal process.
  ///
  /// This method must be called before removing points from the
  /// sequence. After the points have been removed, the
  /// \c endRemovePoints method must be called. Each begin must have
  /// a matching end called.
  ///
  /// \param sequence The index of the sequence to be modified.
  /// \param first The first index of the point removal.
  /// \param last The last index of the point removal.
  /// \sa pqLineChartSeries::beginMultiSequenceChange()
  void beginRemovePoints(int sequence, int first, int last);

  /// \brief
  ///   Called to end the point removal process.
  /// \param sequence The index of the modified sequence.
  /// \sa pqLineChartSeries::beginRemovePoints(int, int, int)
  void endRemovePoints(int sequence);

  /// \brief
  ///   Called to begin a multi-sequence change.
  ///
  /// A series can use one array to store the data for multiple
  /// sequences. In this case, making a change to one sequence will
  /// change another sequence at the same time. To avoid accessing
  /// invalid data, the line chart allows for a multi-sequence change.
  /// The procedure is as follows:
  ///   \li Call \c beginMultiSequenceChange.
  ///   \li Call begin insert/remove for each affected sequence.
  ///   \li Make the change to the underlying data.
  ///   \li Call end insert/remove for each affected sequence.
  ///   \li Call \c endMultiSequenceChange.
  ///
  /// An examle of this is a series that draws a point on every line
  /// segment intersection. The series uses one list to store the
  /// points for a \c Line sequence and a \c Point sequence. In order
  /// to add or remove points, the multi-series change mechanism needs
  /// to be used.
  void beginMultiSequenceChange();

  /// Called to end a multi-sequence change.
  void endMultiSequenceChange();

private:
  ChartAxes Axes; ///< Stores the series chart axes.
};

#endif
