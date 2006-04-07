/*=========================================================================

   Program:   ParaQ
   Module:    pqLineErrorPlot.cxx

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
 * \file pqLineErrorPlot.cxx
 *
 * \brief
 *   The pqLineErrorPlot class is used to draw a piecewise linear
 *   function.
 *
 * \author Mark Richardson
 * \date   August 22, 2005
 */

#include "pqChartAxis.h"
#include "pqChartValue.h"
#include "pqMarkerPen.h"
#include "pqLineErrorPlot.h"

#include <QHelpEvent>
#include <QPainter>
#include <QPolygon>
#include <QToolTip>

#include <vtkType.h>

/////////////////////////////////////////////////////////////////////////
// pqLineErrorPlot::pqImplementation

class pqLineErrorPlot::pqImplementation
{
public:
  pqImplementation(pqMarkerPen* pen, const QPen& whisker_pen, double whisker_size, const CoordinatesT coords) :
    WorldCoords(coords),
    Pen(pen),
    WhiskerPen(whisker_pen),
    WhiskerSize(whisker_size * 0.5)
  {
    double minx = VTK_DOUBLE_MAX;
    double miny = VTK_DOUBLE_MAX;
    double maxx = -VTK_DOUBLE_MAX;
    double maxy = -VTK_DOUBLE_MAX;
    
    this->WorldMin = pqChartCoordinate(VTK_DOUBLE_MAX, VTK_DOUBLE_MAX);
    this->WorldMax = pqChartCoordinate(-VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX);
    
    for(CoordinatesT::const_iterator c = this->WorldCoords.begin(); c != this->WorldCoords.end(); ++c)
      {
      minx = vtkstd::min(minx, c->X);
      maxx = vtkstd::max(maxx, c->X);
      
      miny = vtkstd::min(miny, c->Y - c->LowerBound);
      maxy = vtkstd::max(maxy, c->Y + c->UpperBound);
      }
      
    this->WorldMin = pqChartCoordinate(minx, miny);
    this->WorldMax = pqChartCoordinate(maxx, maxy);
  }
  
  ~pqImplementation()
  {
    delete Pen;
  }

  /// Stores plot data in world coordinates
  const CoordinatesT WorldCoords;
  /// Caches plot data in screen coordinates
  QPolygon PlotScreenCoords;
  QVector<QPoint> WhiskerScreenCoords;
  /// Stores the minimum world coordinate values
  pqChartCoordinate WorldMin;
  /// Stores the maximum world coordinate values
  pqChartCoordinate WorldMax;
  /// Stores the pen used for drawing the plot
  pqMarkerPen* const Pen;
  /// Stores the pen used for drawing error "whiskers"
  QPen WhiskerPen;
  /// Stores the size of the error "whiskers"
  double WhiskerSize;
};

////////////////////////////////////////////////////////////////////////////
// pqLineErrorPlot

pqLineErrorPlot::pqLineErrorPlot(pqMarkerPen* pen, const QPen& whisker_pen, double whisker_size, const CoordinatesT& coords) :
  pqAbstractPlot(),
  Implementation(new pqImplementation(pen, whisker_pen, whisker_size, coords))
{
}

pqLineErrorPlot::~pqLineErrorPlot()
{
  delete this->Implementation;
}

const pqChartCoordinate pqLineErrorPlot::getMinimum() const
{
  return this->Implementation->WorldMin;
}

const pqChartCoordinate pqLineErrorPlot::getMaximum() const
{
  return this->Implementation->WorldMax;
}

void pqLineErrorPlot::layoutPlot(const pqChartAxis& XAxis, const pqChartAxis& YAxis)
{
  this->Implementation->PlotScreenCoords.clear();
  this->Implementation->WhiskerScreenCoords.clear();

  this->Implementation->PlotScreenCoords.reserve(this->Implementation->WorldCoords.size());
  this->Implementation->WhiskerScreenCoords.reserve(this->Implementation->WorldCoords.size() * 6);
  
  for(int i = 0; i != this->Implementation->WorldCoords.size(); ++i)
    {
    const int x = XAxis.getPixelFor(this->Implementation->WorldCoords[i].X);
    const int x1 = XAxis.getPixelFor(this->Implementation->WorldCoords[i].X - this->Implementation->WhiskerSize);
    const int x2 = XAxis.getPixelFor(this->Implementation->WorldCoords[i].X + this->Implementation->WhiskerSize);
    const int y = YAxis.getPixelFor(this->Implementation->WorldCoords[i].Y);
    const int y1 = YAxis.getPixelFor(this->Implementation->WorldCoords[i].Y + this->Implementation->WorldCoords[i].UpperBound);
    const int y2 = YAxis.getPixelFor(this->Implementation->WorldCoords[i].Y - this->Implementation->WorldCoords[i].LowerBound);


    this->Implementation->PlotScreenCoords.push_back(QPoint(x, y));
    
    if(y1 != y || y2 != y)
      {
      this->Implementation->WhiskerScreenCoords.push_back(QPoint(x1, y1));
      this->Implementation->WhiskerScreenCoords.push_back(QPoint(x2, y1));

      this->Implementation->WhiskerScreenCoords.push_back(QPoint(x1, y2));
      this->Implementation->WhiskerScreenCoords.push_back(QPoint(x2, y2));

      this->Implementation->WhiskerScreenCoords.push_back(QPoint(x, y1));
      this->Implementation->WhiskerScreenCoords.push_back(QPoint(x, y2));
      }
    }
}

void pqLineErrorPlot::drawPlot(QPainter& painter, const QRect& /*area*/, const pqChartAxis& /*XAxis*/, const pqChartAxis& /*YAxis*/)
{
  // Draw the error "whiskers" ...
  painter.setPen(this->Implementation->WhiskerPen);
  painter.drawLines(this->Implementation->WhiskerScreenCoords);

  // Draw the plot ...
  this->Implementation->Pen->drawPolyline(painter, this->Implementation->PlotScreenCoords);
  if(this->Implementation->PlotScreenCoords.size())
    this->Implementation->Pen->drawPoint(painter, this->Implementation->PlotScreenCoords.back());
}

const double pqLineErrorPlot::getDistance(const QPoint& coords) const
{
  double distance = VTK_DOUBLE_MAX;
  for(int i = 0; i != this->Implementation->PlotScreenCoords.size(); ++i)
    distance = vtkstd::min(distance, static_cast<double>((this->Implementation->PlotScreenCoords[i] - coords).manhattanLength()));
  return distance;
}

void pqLineErrorPlot::showChartTip(QHelpEvent& event) const
{
  double tip_distance = VTK_DOUBLE_MAX;
  pqChartCoordinate tip_coordinate;
  for(int i = 0; i != this->Implementation->PlotScreenCoords.size(); ++i)
    {
    const double distance = (this->Implementation->PlotScreenCoords[i] - event.pos()).manhattanLength();
    if(distance < tip_distance)
      {
      tip_distance = distance;
      tip_coordinate = pqChartCoordinate(this->Implementation->WorldCoords[i].X, this->Implementation->WorldCoords[i].Y);
      }
    }

  if(tip_distance < VTK_DOUBLE_MAX)    
    QToolTip::showText(event.globalPos(), QString("%1, %2").arg(tip_coordinate.X.getDoubleValue()).arg(tip_coordinate.Y.getDoubleValue()));
}
