/*=========================================================================

   Program: ParaView
   Module:    pqLineChart.h

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

/// \file pqLineChart.h
/// \date 2/28/2007

#ifndef _pqLineChart_h
#define _pqLineChart_h


#include "QtChartExport.h"
#include "pqChartLayer.h"

class pqChartAxis;
class pqChartValue;
class pqLineChartModel;
class pqLineChartOptions;
class pqLineChartSeries;
class pqLineChartSeriesItem;
class pqLineChartInternal;
class QPainter;
class QRect;


/// \class pqLineChart
/// \brief
///   The pqLineChart class is used to display a line chart.
class QTCHART_EXPORT pqLineChart : public pqChartLayer
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a line chart instance.
  /// \param parent The parent object.
  pqLineChart(QObject *parent=0);
  virtual ~pqLineChart();

  /// \name Setup Methods
  //@{
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
  ///   Gets the line chart drawing options.
  /// \return
  ///   A pointer to the line chart drawing options.
  pqLineChartOptions *getOptions() const {return this->Options;}

  /// \brief
  ///   Rebuilds the line chart series options.
  ///
  /// The current line chart series options are removed. New series
  /// options are created to replace the current ones.
  void resetSeriesOptions();
  //@}

  /// \name pqChartLayer Methods
  //@{
  virtual bool getAxisRange(const pqChartAxis *axis, pqChartValue &min,
      pqChartValue &max, bool &padMin, bool &padMax) const;

  virtual void layoutChart(const QRect &area);

  virtual void drawChart(QPainter &painter, const QRect &area);

  virtual void setChartArea(pqChartArea *area);
  //@}

private slots:
  /// Updates the layout of the line chart when the model is reset.
  void handleModelReset();

  /// \brief
  ///   Prepares the chart for a series insertion.
  /// \param first The first index of the new series.
  /// \param last The last index of the new series.
  void startSeriesInsertion(int first, int last);

  /// \brief
  ///   Updates the chart layout after a series is inserted.
  /// \param first The first index of the new series.
  /// \param last The last index of the new series.
  void finishSeriesInsertion(int first, int last);

  /// \brief
  ///   Prepares the chart for a series removal.
  /// \param first The first index of the series being removed.
  /// \param last The last index of the series being removed.
  void startSeriesRemoval(int first, int last);

  /// \brief
  ///   Updates the chart layout after a series is removed.
  ///
  /// If the chart ranges do not change after removing the series,
  /// the chart only has to be repainted. Otherwise, the layout has
  /// to be re-calculated.
  ///
  /// \param first The first index of the series being removed.
  /// \param last The last index of the series being removed.
  void finishSeriesRemoval(int first, int last);

  /// \brief
  ///   Changes the series painting order.
  /// \param current The current index of the moving series.
  /// \param index The index to move the series to.
  void handleSeriesMoved(int current, int index);

  /// \brief
  ///   Updates the series layout after an axis change.
  /// \param series The series that changed.
  void handleSeriesAxesChanged(const pqLineChartSeries *series);

  /// \brief
  ///   Updates the series layout after a major change.
  /// \param series The series that changed.
  void handleSeriesReset(const pqLineChartSeries *series);

  /// \brief
  ///   Prepares the chart for a series point insertion.
  /// \param series The affected series.
  /// \param sequence The index of the point sequence.
  /// \param first The first index of the new points.
  /// \param last The last index of the new points.
  void startPointInsertion(const pqLineChartSeries *series, int sequence,
      int first, int last);

  /// \brief
  ///   Updates the series layout after a point is inserted.
  ///
  /// If the new points modify the overall chart ranges, the layout
  /// for all the series is re-calculated.
  ///
  /// \param series The affected series.
  /// \param sequence The index of the point sequence.
  void finishPointInsertion(const pqLineChartSeries *series, int sequence);

  /// \brief
  ///   Prepares the chart for a series point removal.
  /// \param series The affected series.
  /// \param sequence The index of the point sequence.
  /// \param first The first index of the points being removed.
  /// \param last The last index of the points being removed.
  void startPointRemoval(const pqLineChartSeries *series, int sequence,
      int first, int last);

  /// \brief
  ///   Updates the series layout after a point is removed.
  ///
  /// If the new series ranges modify the overall chart ranges, the
  /// layout for all the series is re-calculated.
  ///
  /// \param series The affected series.
  /// \param sequence The index of the point sequence.
  void finishPointRemoval(const pqLineChartSeries *series, int sequence);

  /// \brief
  ///   Prepares the chart for a multi-sequence change.
  /// \param series The series to be modified.
  void startMultiSeriesChange(const pqLineChartSeries *series);

  /// \brief
  ///   Updates the chart layout after a multi-sequence change.
  /// \param series The series that was modified.
  void finishMultiSeriesChange(const pqLineChartSeries *series);

  /// \brief
  ///   Updates the series layout when its error boundaries change.
  ///
  /// The entire chart layout may need to be re-calculated if the new
  /// series ranges affect the chart ranges.
  ///
  /// \param series The series that was modified.
  /// \param sequence The index of the point sequence.
  /// \param first The first index of the modified points.
  /// \param last The last index of the modified points.
  void handleSeriesErrorBoundsChanged(const pqLineChartSeries *series,
      int sequence, int first, int last);

  /// \brief
  ///   Updates the series layout when the error bar width is changed.
  /// \param series The series that was modified.
  /// \param sequence The index of the modified point sequence.
  void handleSeriesErrorWidthChanged(const pqLineChartSeries *series,
      int sequence);

  /// Initiates a full chart layout when the range changes.
  void handleRangeChange();

  /// Initiates a chart repaint for the changed drawing options.
  void handleSeriesOptionsChanged();

private:
  /// Removes all the line series from the chart.
  void clearSeriesList();

  /// Builds the series list when the model is reset.
  void buildSeriesList();

  /// \brief
  ///   Gets the internal line chart item for the given series.
  /// \param series The series to look up.
  /// \return
  ///   A pointer to the internal chart item for the given series.
  pqLineChartSeriesItem *getItem(const pqLineChartSeries *series) const;

private:
  pqLineChartInternal *Internal; /// Stores the line chart layout.
  pqLineChartOptions *Options;   ///< Stores the drawing options.
  pqLineChartModel *Model;       ///< A pointer to the model.
  bool NeedsLayout;              ///< True if a chart layout is needed.
};

#endif
