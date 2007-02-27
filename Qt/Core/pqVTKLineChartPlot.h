/*=========================================================================

   Program: ParaView
   Module:    pqVTKLineChartPlot.h

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
#ifndef __pqVTKLineChartPlot_h
#define __pqVTKLineChartPlot_h

#include "pqLineChartPlot.h"
#include "pqCoreExport.h"

class pqVTKLineChartPlotInternal;
class vtkDataArray;
class pqLineChartDisplay;
class pqLineChartPlotOptions;

class PQCORE_EXPORT pqVTKLineChartPlot : public pqLineChartPlot
{
  Q_OBJECT
public:
  pqVTKLineChartPlot(pqLineChartDisplay* display, QObject* parent);
  virtual ~pqVTKLineChartPlot();

  // pqLineChartPlot API.
  virtual int getNumberOfSeries() const;
  virtual int getTotalNumberOfPoints() const;
  virtual SeriesType getSeriesType(int series) const;
  virtual int getNumberOfPoints(int series) const;
  virtual void getPoint(int series, int index,
    pqChartCoordinate &coord) const;
  virtual void getErrorBounds(int series, int index, pqChartValue &upper,
    pqChartValue &lower) const;
  virtual void getErrorWidth(int series, pqChartValue &width) const;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const;
  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const;


  /// This will check the modified times and the last update time
  /// and call forceUpdate() if required.
  void update();

  /// Update the plot with the current data values.
  void forceUpdate();

  /// Return the options for this plot.
  pqLineChartPlotOptions* getOptions() const;
protected slots:
  /// updates the MTime.
  void markModified();

private:
  pqVTKLineChartPlot(const pqVTKLineChartPlot&); // Not implemented.
  void operator=(const pqVTKLineChartPlot&); // Not implemented.

  pqVTKLineChartPlotInternal* Internal;

  /// Converts a series number to a Y array index in the pqLineChartDisplay.
  int getIndexFromSeries(int series) const;

};

#endif

