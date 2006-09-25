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
class QTCHART_EXPORT pqSimpleLineChartPlot : public pqLineChartPlot
{
public:
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
  void clearPlot();

  void addSeries(pqLineChartPlot::SeriesType type);
  void insertSeries(int index, pqLineChartPlot::SeriesType type);
  void setSeriesType(int series, pqLineChartPlot::SeriesType type);
  void removeSeries(int series);
  void copySeriesPoints(int source, int destination);

  void addPoint(int series, const pqChartCoordinate &coord);
  void insertPoint(int series, int index, const pqChartCoordinate &coord);
  void removePoint(int series, int index);
  void clearPoints(int series);

  void setErrorBounds(int series, int index, const pqChartValue &upper,
      const pqChartValue &lower);
  void setErrorWidth(int series, const pqChartValue &width);
  //@}

private:
  void updatePlotRanges();
  void updatePlotRanges(const pqChartCoordinate &coord);

private:
  pqSimpleLineChartPlotInternal *Internal; ///< Stores the plot data.
};

#endif
