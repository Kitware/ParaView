/*=========================================================================

   Program:   ParaQ
   Module:    pqLinePlot.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef _pqLinePlot_h
#define _pqLinePlot_h

#include "QtChartExport.h"
#include "pqAbstractPlot.h"

class pqMarkerPen;

/// Displays a line plot
class QTCHART_EXPORT pqLinePlot :
  public pqAbstractPlot
{
public:
  /// pqLinePlot assumes ownership of the given pen
  pqLinePlot(pqMarkerPen* pen, const pqChartCoordinateList& coords);
  /// pqLinePlot assumes ownership of the given pen
  pqLinePlot(pqMarkerPen* pen, const pqChartCoordinate& p1, const pqChartCoordinate& p2);
  virtual ~pqLinePlot();

  /// \name pqAbstractPlot Methods
  //@{
  virtual const pqChartCoordinate getMinimum() const;
  virtual const pqChartCoordinate getMaximum() const;
  virtual void layoutPlot(const pqChartAxis& XAxis, const pqChartAxis& YAxis);
  virtual void drawPlot(QPainter& painter, const QRect& area, const pqChartAxis& XAxis, const pqChartAxis& YAxis);
  virtual const double getDistance(const QPoint& coords) const;
  virtual void showChartTip(QHelpEvent& event) const;
  //@}

private:
  /// Private implementation details
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
