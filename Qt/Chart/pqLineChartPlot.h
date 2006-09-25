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


class QTCHART_EXPORT pqLineChartPlot : public QObject
{
  Q_OBJECT

public:
  enum SeriesType
    {
    Invalid,
    Point,
    Line,
    Error
    };

public:
  pqLineChartPlot(QObject *parent=0);
  virtual ~pqLineChartPlot() {}

  virtual int getNumberOfSeries() const=0;
  virtual int getTotalNumberOfPoints() const=0;
  virtual SeriesType getSeriesType(int series) const=0;
  virtual int getNumberOfPoints(int series) const=0;
  virtual void getPoint(int series, int index,
      pqChartCoordinate &coord) const=0;
  virtual void getErrorBounds(int series, int index, pqChartValue &upper,
      pqChartValue &lower) const=0;
  virtual void getErrorWidth(int series, pqChartValue &width) const=0;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const=0;
  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const=0;

signals:
  void plotReset();
  void aboutToInsertPoints(int series, int first, int last);
  void pointsInserted(int series);
  void aboutToRemovePoints(int series, int first, int last);
  void pointsRemoved(int series);
  void aboutToChangeMultipleSeries();
  void changedMultipleSeries();
  void errorBoundsChanged(int series, int first, int last);
  void errorWidthChanged(int series);

protected:
  void resetPlot();
  void beginInsertPoints(int series, int first, int last);
  void endInsertPoints(int series);
  void beginRemovePoints(int series, int first, int last);
  void endRemovePoints(int series);
  void beginMultiSeriesChange();
  void endMultiSeriesChange();
};

#endif
