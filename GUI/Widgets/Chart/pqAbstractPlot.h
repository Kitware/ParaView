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

/*!
 * \file pqAbstractPlot.h
 *
 * \brief
 *   The pqAbstractPlot class is the drawing interface to a function.
 *
 * \author Mark Richardson
 * \date   August 2, 2005
 */

#ifndef _pqAbstractPlot_h
#define _pqAbstractPlot_h

#include "pqChartCoordinate.h"
#include "QtChartExport.h"

class pqChartAxis;
class QHelpEvent;
class QPainter;
class QPoint;
class QRect;

/// \class pqAbstractPlot
/// \brief
///   The pqAbstractPlot class is the drawing interface to a function.
///
/// The pqLineChart uses this interface to draw functions. In order
/// to have a function show up on the line chart, a new class must
/// be created that inherits from %pqAbstractPlot.
class QTCHART_EXPORT pqAbstractPlot
{
public:
  virtual ~pqAbstractPlot() {}

  /// Returns the minimum coordinates for the plot
  virtual const pqChartCoordinate getMinimum() const = 0;
  /// Returns the maximum coordinates for the plot
  virtual const pqChartCoordinate getMaximum() const = 0;
  /// Gives the plot a chance to update its layout (e.g. create an internal cache of screen coordinates)
  virtual void layoutPlot(const pqChartAxis& XAxis, const pqChartAxis& YAxis) = 0;
  /// Gives the plot a chance to draw itself
  virtual void drawPlot(QPainter& painter, const QRect& area, const pqChartAxis& XAxis, const pqChartAxis& YAxis) = 0;
  /// Returns a distance metric from a point in screen coordinates to the plot (used to pick a plot to display chart tips)
  virtual const double getDistance(const QPoint& coords) const = 0;
  /// Displays a "chart tip" for the given screen coordinates (derivatives can use this to display any data they wish)
  virtual void showChartTip(QHelpEvent& event) const = 0;
};

#endif
