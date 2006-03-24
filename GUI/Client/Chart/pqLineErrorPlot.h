/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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

#ifndef _pqLineErrorPlot_h
#define _pqLineErrorPlot_h

#include "pqChartExport.h"
#include "pqAbstractPlot.h"

#include <QVector>

class pqMarkerPen;
class QPen;

/// Displays a line plot that includes "whiskers" to visualize error bounds
class QTCHART_EXPORT pqLineErrorPlot :
  public pqAbstractPlot
{
public:
  /// Defines a 2-dimensional coordinate whose Y value is bounded by the given error values
  struct Coordinate
  {
    Coordinate()
    {
    }
    
    Coordinate(double x, double y, double upper_bound, double lower_bound) :
      X(x),
      Y(y),
      UpperBound(upper_bound),
      LowerBound(lower_bound)
    {
    }
    
    /// Stores the X value of the coordinate
    double X;
    /// Stores the Y value of the coordinate
    double Y;
    /// Stores the upper bound of the Y value, as a positive delta
    double UpperBound;
    /// Stores the lower bound of the Y value, as a *positive* delta
    double LowerBound;
  };
  /// Defines a collection of coordinates with error bounds
  typedef QVector<Coordinate> CoordinatesT;

  /// pqLineErrorPlot assumes ownership of the given pen
  pqLineErrorPlot(pqMarkerPen* pen, const QPen& whisker_pen, double whisker_size, const CoordinatesT& coords);
  virtual ~pqLineErrorPlot();

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
