/*=========================================================================

   Program: ParaView
   Module:    pqHistogramChart.h

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

/// \file pqHistogramChart.h
/// \date 2/14/2007

#ifndef _pqHistogramChart_h
#define _pqHistogramChart_h


#include "QtChartExport.h"
#include "pqChartLayer.h"

#include "pqHistogramSelectionModel.h" // Needed for typedef

class pqChartAxis;
class pqChartValue;
class pqHistogramChartOptions;
class pqHistogramChartInternal;
class pqHistogramModel;
class QRect;


/// \class pqHistogramChart
/// \brief
///   The pqHistogramChart class is used to display a histogram.
class QTCHART_EXPORT pqHistogramChart : public pqChartLayer
{
  Q_OBJECT

public:
  enum BinPickMode
    {
    BinRectangle, ///< Pick must be within the bin rectangle.
    BinRange      ///< Pick only has to be within the bin range.
    };

  enum ChartAxes
    {
    BottomLeft = 0, ///< Histogram uses the bottom and left axes.
    BottomRight,    ///< Histogram uses the bottom and right axes.
    TopLeft,        ///< Histogram uses the top and left axes.
    TopRight        ///< Histogram uses the top and right axes.
    };

public:
  /// \brief
  ///   Creates a histogram chart instance.
  /// \param parent The parent object.
  pqHistogramChart(QObject *parent=0);
  virtual ~pqHistogramChart();

  /// \name Setup Methods
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

  /// \brief
  ///   Gets the x-axis for the chart.
  /// \return
  ///   A pointer to the x-axis for the chart.
  pqChartAxis *getXAxis() const;

  /// \brief
  ///   Gets the y-axis for the chart.
  /// \return
  ///   A pointer to the y-axis for the chart.
  pqChartAxis *getYAxis() const;

  /// \brief
  ///   Sets the histogram model to be displayed.
  /// \param model A pointer to the new histogram model.
  void setModel(pqHistogramModel *model);

  /// \brief
  ///   Gets the current histogram model.
  /// \return
  ///   A pointer to the currect histogram model.
  pqHistogramModel *getModel() const {return this->Model;}
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
  /// \param mode The bin picking mode to use (rectangle or range).
  /// \return
  ///   The bin index for the given location.
  /// \sa pqHistogramChart::getBinsIn(const QRect &,
  ///         pqHistogramSelectionList &, bool)
  int getBinAt(int x, int y, BinPickMode mode=BinRange) const;

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
  /// \param mode The bin picking mode to use (rectangle or range).
  /// \sa pqHistogramChart::getBinAt(int, int)
  void getBinsIn(const QRect &area, pqHistogramSelectionList &list,
      BinPickMode mode=BinRange) const;

  /// \brief
  ///   Gets a list of values within the given area.
  /// \param area The area of the chart to query.
  /// \param list Used to return the list of value ranges.
  /// \sa pqHistogramChart::getValueAt(int, int)
  void getValuesIn(const QRect &area, pqHistogramSelectionList &list) const;

  /// \brief
  ///   Gets the area that contains the given selection.
  /// \param list The selection to find.
  /// \param area Used to return the selection area.
  void getSelectionArea(const pqHistogramSelectionList &list,
      QRect &area) const;
  //@}

  /// \name Drawing Parameters
  //@{
  /// \brief
  ///   Gets the histogram chart drawing options.
  /// \return
  ///   A pointer to the histogram chart drawing options.
  pqHistogramChartOptions *getOptions() const {return this->Options;}

  /// \brief
  ///   Sets the histogram chart drawing options.
  ///
  /// This method sets all the options at once, which can prevent
  /// unnecessary view updates.
  ///
  /// \param options The new histogram drawing options.
  void setOptions(const pqHistogramChartOptions &options);
  //@}

  /// \name pqChartLayer Methods
  //@{
  virtual bool getAxisRange(const pqChartAxis *axis, pqChartValue &min,
      pqChartValue &max, bool &padMin, bool &padMax) const;

  virtual bool isAxisControlPreferred(const pqChartAxis *axis) const;

  virtual void generateAxisLabels(pqChartAxis *axis);

  virtual void layoutChart(const QRect &area);

  /// \brief
  ///   Draws the highlight background for the chart.
  ///
  /// The highlight background is drawn separately so it can be
  /// drawn before the background grid.
  ///
  /// \param painter The painter to use.
  /// \param area The area that needs to be painted.
  /// \sa pqHistogramChart::drawChart(QPainter &, const QRect &)
  virtual void drawBackground(QPainter &painter, const QRect &area);

  virtual void drawChart(QPainter &painter, const QRect &area);
  //@}

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

  /// Updates the histogram highlights when the selection changes.
  void updateHighlights();

private:
  /// Used to layout the selection list.
  void layoutSelection();

private:
  /// Stores the view items.
  pqHistogramChartInternal *Internal;
  pqHistogramChartOptions *Options; ///< Stores the histogram options.
  ChartAxes Axes;                   ///< Stores the chart axes.
  pqHistogramModel *Model;          ///< Stores the histogram model.

  /// Stores the selection model.
  pqHistogramSelectionModel *Selection;

  /// True if selection change is due to a model change.
  bool InModelChange;
};

#endif
