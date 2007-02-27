/*=========================================================================

   Program: ParaView
   Module:    pqLineChartModel.h

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

/// \file pqLineChartModel.h
/// \date 9/8/2006

#ifndef _pqLineChartModel_h
#define _pqLineChartModel_h


#include "QtChartExport.h"
#include <QObject>

class pqChartValue;
class pqLineChartModelInternal;
class pqLineChartPlot;
class pqLineChartPlotOptions;


/// \class pqLineChartModel
/// \brief
///   The pqLineChartModel class is the line chart's interface to the
///   chart data.
///
/// The model uses the pqLineChartPlot interface to draw the
/// individual line plots. The line chart will draw the plots in the
/// order the model stores them. The first plot will be drawn first,
/// and so on. This can be used as a layering effect.
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
  ///   Gets the number of plots in the model.
  /// \return
  ///   The number of plots in the model.
  int getNumberOfPlots() const;

  /// \brief
  ///   Gets the index for the given plot.
  /// \param plot The line chart plot to look up.
  /// \return
  ///   The index for the given plot.
  int getIndexOf(pqLineChartPlot *plot) const;

  /// \brief
  ///   Gets the plot at the given index.
  /// \param index The plot's model index.
  /// \return
  ///   A pointer to the plot or null if the index is out of range.
  pqLineChartPlot *getPlot(int index) const;

  /// \brief
  ///   Appends a new plot to the end of the model list.
  /// \param plot The new plot to add.
  void appendPlot(pqLineChartPlot *plot);

  /// \brief
  ///   Inserts a new plot in the model list.
  /// \param plot The new plot to add.
  /// \param index Where to insert the plot.
  void insertPlot(pqLineChartPlot *plot, int index);

  /// \brief
  ///   Removes the given plot from the model.
  /// \param plot The plot to remove.
  void removePlot(pqLineChartPlot *plot);

  /// \brief
  ///   Removes the plot at the given index from the model.
  /// \param index The index of the plot to remove.
  void removePlot(int index);

  /// \brief
  ///   Moves the given plot to the new position.
  /// \param plot The plot to move.
  /// \param index Where to move the plot to.
  void movePlot(pqLineChartPlot *plot, int index);

  /// \brief
  ///   Moves the plot at the given index to the new position.
  /// \param current The index of the plot to move.
  /// \param index Where to move the plot to.
  void movePlot(int current, int index);

  /// \brief
  ///   Moves the plot at the given index to the new position.
  ///
  /// The corresponding display options will also get moved with the
  /// plot.
  ///
  /// \param current The index of the plot to move.
  /// \param index Where to move the plot to.
  void movePlotAndOptions(int current, int index);

  /// Removes all the plots from the model.
  virtual void clearPlots();
  //@}

  /// \name Chart Range Methods
  //@{
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

  /// \name Chart Display Options
  //@{
  /// \brief
  ///   Gets the line chart plot's display options.
  /// \return
  ///   A pointer to the line chart plot's display options.
  virtual pqLineChartPlotOptions *getOptions(int index) const;

  /// \brief
  ///   Sets the line chart plot's display options.
  /// \param options The new display options.
  void setOptions(int index, pqLineChartPlotOptions *options);

  /// Removes all the display options from the model.
  void clearOptions();
  //@}

signals:
  /// Emitted when the model has been reset.
  void plotsReset();

  /// \brief
  ///   Emitted when new plots will be inserted.
  /// \param first The first index of the plot insertion.
  /// \param last The last index of the plot insertion.
  void aboutToInsertPlots(int first, int last);

  /// \brief
  ///   Emitted when new plots have been inserted.
  /// \param first The first index of the plot insertion.
  /// \param last The last index of the plot insertion.
  void plotsInserted(int first, int last);

  /// \brief
  ///   Emitted when plots will be removed from the model.
  /// \param first The first index of the plot removal.
  /// \param last The last index of the plot removal.
  void aboutToRemovePlots(int first, int last);

  /// \brief
  ///   Emitted when plots have been removed from the model.
  /// \param first The first index of the plot removal.
  /// \param last The last index of the plot removal.
  void plotsRemoved(int first, int last);

  /// \brief
  ///   Emitted when a plot is moved in the list.
  /// \param current The known index of the plot.
  /// \param index The new index of the plot.
  void plotMoved(int current, int index);

  /// \brief
  ///   Emitted when the data for a plot has changed drastically.
  /// \param plot The modified plot.
  void plotReset(const pqLineChartPlot *plot);

  /// \brief
  ///   Emitted when new points will be inserted in a plot.
  /// \param plot The plot to be modified.
  /// \param series The index of the series to be modified.
  /// \param first The first index of the point insertion.
  /// \param last The last index of the point insertion.
  void aboutToInsertPoints(const pqLineChartPlot *plot, int series, int first,
      int last);

  /// \brief
  ///   Emitted when new points have been inserted in a plot.
  /// \param plot The modified plot.
  /// \param series The index of the modified series.
  void pointsInserted(const pqLineChartPlot *plot, int series);

  /// \brief
  ///   Emitted when points will be removed from a plot.
  /// \param plot The plot to be modified.
  /// \param series The index of the series to be modified.
  /// \param first The first index of the point removal.
  /// \param last The last index of the point removal.
  void aboutToRemovePoints(const pqLineChartPlot *plot, int series, int first,
      int last);

  /// \brief
  ///   Emitted when points have been removed from a plot.
  /// \param plot The modified plot.
  /// \param series The index of the modified series.
  void pointsRemoved(const pqLineChartPlot *plot, int series);

  /// \brief
  ///   Emitted when changes will be made to multiple series of a plot
  ///   simultaneously.
  /// \param plot The plot to be modified.
  /// \sa pqLineChartPlot::beginMultiSeriesChange()
  void aboutToChangeMultipleSeries(const pqLineChartPlot *plot);

  /// \brief
  ///   Emitted when simultaneous changes have been made to multiple
  ///   series of a plot.
  /// \param plot The modified plot.
  /// \sa pqLineChartPlot::beginMultiSeriesChange()
  void changedMultipleSeries(const pqLineChartPlot *plot);

  /// \brief
  ///   Emitted when the error bounds for a plot have changed.
  /// \param plot The modified plot.
  /// \param series The index of the modified series.
  /// \param first The first index of the modified points.
  /// \param last The last index of the modified points.
  void errorBoundsChanged(const pqLineChartPlot *plot, int series, int first,
      int last);

  /// \brief
  ///   Emitted when the error bar width for a series of a plot has
  ///   changed.
  /// \param plot The modified plot.
  /// \param series The index of the modified series.
  void errorWidthChanged(const pqLineChartPlot *plot, int series);

  /// Emitted when the drawing options for a plot change.
  void optionsChanged();

private slots:
  /// \name Plot Modification Handlers
  //@{
  /// Handles a plot reset signal from a line chart plot.
  void handlePlotReset();

  /// \brief
  ///   Handles a plot begin insertion notification.
  /// \param series The index of the series to be modified.
  /// \param first The first index of the point insertion.
  /// \param last The last index of the point insertion.
  void handlePlotBeginInsert(int series, int first, int last);

  /// \brief
  ///   Handles a plot end insertion notification.
  /// \param series The index of the modified series.
  void handlePlotEndInsert(int series);

  /// \brief
  ///   Handles a plot begin removal notification.
  /// \param series The index of the series to be modified.
  /// \param first The first index of the point removal.
  /// \param last The last index of the point removal.
  void handlePlotBeginRemove(int series, int first, int last);

  /// \brief
  ///   Handles a plot end removal notification.
  /// \param series The index of the modified series.
  void handlePlotEndRemove(int series);

  /// Handles a plot begin multi-series change signal.
  void handlePlotBeginMultiSeriesChange();

  /// Handles a plot end multi-series change signal.
  void handlePlotEndMultiSeriesChange();

  /// \brief
  ///   Handles a plot error bounds changed signal.
  /// \param series The index of the modified series.
  /// \param first The first index of the modified points.
  /// \param last The last index of the modified points.
  void handlePlotErrorBoundsChange(int series, int first, int last);

  /// \brief
  ///   Handles a plot error bar width changed signal.
  /// \param series The index of the modified series.
  void handlePlotErrorWidthChange(int series);
  //@}

private:
  /// Compiles the overall chart range from the plots.
  void updateChartRanges();

  /// \brief
  ///   Updates the chart range after a plot addition or point
  ///   insertion.
  ///
  /// This method can only be called after a plot or point insertion.
  /// In both cases, the chart ranges can only grow not shrink.
  ///
  /// \param plot The new or expanded plot.
  void updateChartRanges(const pqLineChartPlot *plot);

private:
  pqLineChartModelInternal *Internal; ///< Stores the list of plots.
};

#endif
