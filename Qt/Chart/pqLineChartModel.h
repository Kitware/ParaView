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


class QTCHART_EXPORT pqLineChartModel : public QObject
{
  Q_OBJECT

public:
  pqLineChartModel(QObject *parent=0);
  virtual ~pqLineChartModel();

  int getNumberOfPlots() const;
  int getIndexOf(const pqLineChartPlot *plot) const;
  const pqLineChartPlot *getPlot(int index) const;
  void appendPlot(const pqLineChartPlot *plot);
  void insertPlot(const pqLineChartPlot *plot, int index);
  void removePlot(const pqLineChartPlot *plot);
  void removePlot(int index);
  void movePlot(const pqLineChartPlot *plot, int index);
  void movePlot(int current, int index);
  void clearPlots();

  void getRangeX(pqChartValue &min, pqChartValue &max) const;
  void getRangeY(pqChartValue &min, pqChartValue &max) const;

signals:
  void plotsReset();
  void aboutToInsertPlots(int first, int last);
  void plotsInserted(int first, int last);
  void aboutToRemovePlots(int first, int last);
  void plotsRemoved(int first, int last);
  void plotMoved(int current, int index);
  void plotReset(const pqLineChartPlot *plot);
  void aboutToInsertPoints(const pqLineChartPlot *plot, int series, int first,
      int last);
  void pointsInserted(const pqLineChartPlot *plot, int series);
  void aboutToRemovePoints(const pqLineChartPlot *plot, int series, int first,
      int last);
  void pointsRemoved(const pqLineChartPlot *plot, int series);
  void aboutToChangeMultipleSeries(const pqLineChartPlot *plot);
  void changedMultipleSeries(const pqLineChartPlot *plot);
  void errorBoundsChanged(const pqLineChartPlot *plot, int series, int first,
      int last);
  void errorWidthChanged(const pqLineChartPlot *plot, int series);

private slots:
  void handlePlotReset();
  void handlePlotBeginInsert(int series, int first, int last);
  void handlePlotEndInsert(int series);
  void handlePlotBeginRemove(int series, int first, int last);
  void handlePlotEndRemove(int series);
  void handlePlotBeginMultiSeriesChange();
  void handlePlotEndMultiSeriesChange();
  void handlePlotErrorBoundsChange(int series, int first, int last);
  void handlePlotErrorWidthChange(int series);

private:
  void updateChartRanges();
  void updateChartRanges(const pqLineChartPlot *plot);

private:
  pqLineChartModelInternal *Internal; ///< Stores the list of plots.
};

#endif
