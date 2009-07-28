/*=========================================================================

   Program: ParaView
   Module:    pqLineChartModel.h

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

/// \file pqLineChartModel.h
/// \date 9/8/2006

#ifndef _pqLineChartModel_h
#define _pqLineChartModel_h


#include "QtChartExport.h"
#include <QObject>

class pqChartAxis;
class pqChartValue;
class pqLineChartModelInternal;
class pqLineChartSeries;


/// \class pqLineChartModel
/// \brief
///   The pqLineChartModel class is the line chart's interface to the
///   chart data.
///
/// The model uses the pqLineChartSeries interface to draw the
/// individual line chart series. The line chart will draw the series
/// in the order the model stores them. The first series will be drawn
/// first, and so on. This can be used as a layering effect.
class QTCHART_EXPORT pqLineChartModel : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a line chart model object.
  /// \param parent The parent object.
  pqLineChartModel(QObject *parent=0);
  virtual ~pqLineChartModel();

  /// \name Model Data Methods
  //@{
  /// \brief
  ///   Gets the number of series in the model.
  /// \return
  ///   The number of series in the model.
  int getNumberOfSeries() const;

  /// \brief
  ///   Gets the index for the given series.
  /// \param series The line chart series to look up.
  /// \return
  ///   The index for the given series.
  int getIndexOf(pqLineChartSeries *series) const;

  /// \brief
  ///   Gets the series at the given index.
  /// \param index The series' model index.
  /// \return
  ///   A pointer to the series or null if the index is out of range.
  pqLineChartSeries *getSeries(int index) const;

  /// \brief
  ///   Appends a new series to the end of the model list.
  /// \param series The new series to add.
  void appendSeries(pqLineChartSeries *series);

  /// \brief
  ///   Inserts a new series in the model list.
  /// \param series The new series to add.
  /// \param index Where to insert the series.
  void insertSeries(pqLineChartSeries *series, int index);

  /// \brief
  ///   Removes the given series from the model.
  /// \param series The series to remove.
  void removeSeries(pqLineChartSeries *series);

  /// \brief
  ///   Removes the series at the given index from the model.
  /// \param index The index of the series to remove.
  void removeSeries(int index);

  /// \brief
  ///   Moves the given series to the new position.
  /// \param series The series to move.
  /// \param index Where to move the series to.
  void moveSeries(pqLineChartSeries *series, int index);

  /// \brief
  ///   Moves the series at the given index to the new position.
  /// \param current The index of the series to move.
  /// \param index Where to move the series to.
  void moveSeries(int current, int index);

  /// Removes all the series from the model.
  virtual void removeAll();
  //@}

  /// \name Chart Range Methods
  //@{
  /// \brief
  ///   Gets the range for the given axis.
  /// \param axis Which axis to get the range for.
  /// \param min Used to return the minimum value.
  /// \param max Used to return the maximum value.
  /// \return
  ///   False if the line chart does not use the given axis.
  bool getAxisRange(const pqChartAxis *axis, pqChartValue &min,
      pqChartValue &max) const;

  /// \brief
  ///   Get the x-axis range for the chart.
  /// \param min Used to return the minimum x-axis value.
  /// \param max Used to return the maximum x-axis value.
  void getRangeX(pqChartValue &min, pqChartValue &max) const;

  /// \brief
  ///   Get the y-axis range for the chart.
  /// \param min Used to return the minimum y-axis value.
  /// \param max Used to return the maximum y-axis value.
  void getRangeY(pqChartValue &min, pqChartValue &max) const;
  //@}

signals:
  /// Emitted when the model has been reset.
  void modelReset();

  /// \brief
  ///   Emitted when new series will be inserted.
  /// \param first The first index of the series insertion.
  /// \param last The last index of the series insertion.
  void aboutToInsertSeries(int first, int last);

  /// \brief
  ///   Emitted when new series have been inserted.
  /// \param first The first index of the series insertion.
  /// \param last The last index of the series insertion.
  void seriesInserted(int first, int last);

  /// \brief
  ///   Emitted when series will be removed from the model.
  /// \param first The first index of the series removal.
  /// \param last The last index of the series removal.
  void aboutToRemoveSeries(int first, int last);

  /// \brief
  ///   Emitted when series have been removed from the model.
  /// \param first The first index of the series removal.
  /// \param last The last index of the series removal.
  void seriesRemoved(int first, int last);

  /// \brief
  ///   Emitted when a series is moved in the list.
  /// \param current The known index of the series.
  /// \param index The new index of the series.
  void seriesMoved(int current, int index);

  /// \brief
  ///   Emitted when the chart axes for a series have changed.
  /// \param series The series that changed.
  void seriesChartAxesChanged(const pqLineChartSeries *series);

  /// \brief
  ///   Emitted when the data for a series has changed drastically.
  /// \param series The modified series.
  void seriesReset(const pqLineChartSeries *series);

  /// \brief
  ///   Emitted when new points will be inserted in a series.
  /// \param series The series to be modified.
  /// \param sequence The index of the sequence to be modified.
  /// \param first The first index of the point insertion.
  /// \param last The last index of the point insertion.
  void aboutToInsertPoints(const pqLineChartSeries *series, int sequence,
      int first, int last);

  /// \brief
  ///   Emitted when new points have been inserted in a series.
  /// \param series The modified series.
  /// \param sequence The index of the modified sequence.
  void pointsInserted(const pqLineChartSeries *series, int sequence);

  /// \brief
  ///   Emitted when points will be removed from a series.
  /// \param series The series to be modified.
  /// \param sequence The index of the sequence to be modified.
  /// \param first The first index of the point removal.
  /// \param last The last index of the point removal.
  void aboutToRemovePoints(const pqLineChartSeries *series, int sequence,
      int first, int last);

  /// \brief
  ///   Emitted when points have been removed from a series.
  /// \param series The modified series.
  /// \param sequence The index of the modified sequence.
  void pointsRemoved(const pqLineChartSeries *series, int sequence);

  /// \brief
  ///   Emitted when changes will be made to multiple sequences of a
  ///   series simultaneously.
  /// \param series The series to be modified.
  /// \sa pqLineChartSeries::beginMultiSeriesChange()
  void aboutToChangeMultipleSeries(const pqLineChartSeries *series);

  /// \brief
  ///   Emitted when simultaneous changes have been made to multiple
  ///   sequences of a series.
  /// \param series The modified series.
  /// \sa pqLineChartSeries::beginMultiSeriesChange()
  void changedMultipleSeries(const pqLineChartSeries *series);

  /// \brief
  ///   Emitted when the error bounds for a series have changed.
  /// \param series The modified series.
  /// \param sequence The index of the modified sequence.
  /// \param first The first index of the modified points.
  /// \param last The last index of the modified points.
  void errorBoundsChanged(const pqLineChartSeries *series, int sequence,
      int first, int last);

  /// \brief
  ///   Emitted when the error bar width for a sequence of a series
  ///   has changed.
  /// \param series The modified series.
  /// \param sequence The index of the modified sequence.
  void errorWidthChanged(const pqLineChartSeries *series, int sequence);

  /// Emitted when the line chart range in x and/or y changes.
  void chartRangeChanged();

private slots:
  /// \name Series Modification Handlers
  //@{
  /// Handles a series chart axes changed signal from the chart series.
  void handleSeriesAxesChanged();

  /// Handles a series reset signal from a line chart series.
  void handleSeriesReset();

  /// \brief
  ///   Handles a series begin insertion notification.
  /// \param sequence The index of the sequence to be modified.
  /// \param first The first index of the point insertion.
  /// \param last The last index of the point insertion.
  void handleSeriesBeginInsert(int sequence, int first, int last);

  /// \brief
  ///   Handles a series end insertion notification.
  /// \param sequence The index of the modified sequence.
  void handleSeriesEndInsert(int sequence);

  /// \brief
  ///   Handles a series begin removal notification.
  /// \param sequence The index of the sequence to be modified.
  /// \param first The first index of the point removal.
  /// \param last The last index of the point removal.
  void handleSeriesBeginRemove(int sequence, int first, int last);

  /// \brief
  ///   Handles a series end removal notification.
  /// \param sequence The index of the modified sequence.
  void handleSeriesEndRemove(int sequence);

  /// Handles a series begin multi-sequence change signal.
  void startSeriesMultiSequenceChange();

  /// Handles a series end multi-sequence change signal.
  void finishSeriesMultiSequenceChange();

  /// \brief
  ///   Handles a series error bounds changed signal.
  /// \param sequence The index of the modified sequence.
  /// \param first The first index of the modified points.
  /// \param last The last index of the modified points.
  void handleSeriesErrorBoundsChange(int sequence, int first, int last);

  /// \brief
  ///   Handles a series error bar width changed signal.
  /// \param sequence The index of the modified sequence.
  void handleSeriesErrorWidthChange(int sequence);
  //@}

private:
  /// Compiles the overall chart range from the series.
  void updateChartRanges();

  /// \brief
  ///   Updates the chart range after a series addition or point
  ///   insertion.
  ///
  /// This method can only be called after a series or point
  /// insertion. In both cases, the chart ranges can only grow not
  /// shrink.
  ///
  /// \param series The new or expanded series.
  void updateChartRanges(const pqLineChartSeries *series);

private:
  pqLineChartModelInternal *Internal; ///< Stores the line data.
};

#endif
