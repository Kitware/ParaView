/*=========================================================================

   Program: ParaView
   Module:    pqLineChart.cxx

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

/*!
 * \file pqLineChart.cxx
 *
 * \brief
 *   The pqLineChart class is used to display a line plot.
 *
 * \author Mark Richardson
 * \date   August 1, 2005
 */

#include "pqChartAxis.h"
#include "pqChartCoordinate.h"
#include "pqLineChart.h"
#include "pqAbstractPlot.h"
#include "pqMarkerPen.h"

#include <QHelpEvent>
#include <QPainter>
#include <QPolygon>

#include <vtkMath.h>
#include <vtkstd/algorithm>
#include <vtkstd/vector>
#include <vtkType.h>

//////////////////////////////////////////////////////////////////////
// pqLineChart::pqImplementation

class pqLineChart::pqImplementation :
  public vtkstd::vector<pqAbstractPlot*>
{
public:
  pqImplementation() :
    XAxis(0),
    YAxis(0),
    XShared(false)
  {
  }
  
  ~pqImplementation()
  {
    this->clear();
  }
  
  void clear()
  {
    for(unsigned int i = 0; i != this->size(); ++i)
      delete this->at(i);
      
    vtkstd::vector<pqAbstractPlot*>::clear();
  }

  pqChartAxis* XAxis;    ///< Stores the x-axis object.
  pqChartAxis* YAxis;    ///< Stores the y-axis object.
  bool XShared;          ///< True if the x-axis is shared.
};

///////////////////////////////////////////////////////////////////////
// pqLineChart

pqLineChart::pqLineChart(QObject *p) :
  QObject(p),
  Implementation(new pqImplementation())
{
}

pqLineChart::~pqLineChart()
{
  delete this->Implementation;
}

void pqLineChart::setAxes(pqChartAxis *xAxis, pqChartAxis *yAxis,
    bool shared)
{
  this->Implementation->XAxis = xAxis;
  this->Implementation->YAxis = yAxis;
  this->Implementation->XShared = shared;
}

void pqLineChart::addData(pqAbstractPlot* plot)
{
  if(!plot)
    return;
    
  if(vtkstd::find(this->Implementation->begin(), this->Implementation->end(), plot) != this->Implementation->end())
    return;

  this->Implementation->push_back(plot);

  this->updateAxes();
  plot->layoutPlot(*this->Implementation->XAxis, *this->Implementation->YAxis);
  emit this->repaintNeeded();
}

void pqLineChart::removeData(pqAbstractPlot *plot)
{
  this->Implementation->erase(
    vtkstd::remove(this->Implementation->begin(), this->Implementation->end(), plot),
    this->Implementation->end());

  this->updateAxes();
  emit this->repaintNeeded();    
}

void pqLineChart::layoutChart()
{
  if(!this->Implementation->XAxis || !this->Implementation->YAxis)
    return;

  // Make sure the axes are valid.
  if(!this->Implementation->XAxis->isValid() || !this->Implementation->YAxis->isValid())
    return;

  // Give all of the plots a chance to update their internal state
  for(pqImplementation::iterator plot = this->Implementation->begin(); plot != this->Implementation->end(); ++plot)
    {
    (*plot)->layoutPlot(*this->Implementation->XAxis, *this->Implementation->YAxis);
    }

  // Set up the chart area based on the remaining space.
  this->Bounds.setTop(this->Implementation->YAxis->getMaxPixel());
  this->Bounds.setLeft(this->Implementation->XAxis->getMinPixel());
  this->Bounds.setRight(this->Implementation->XAxis->getMaxPixel());
  this->Bounds.setBottom(this->Implementation->YAxis->getMinPixel());
}

void pqLineChart::drawChart(QPainter& painter, const QRect& area)
{
  if(!painter.isActive() || !area.isValid())
    return;
    
  if(!this->Bounds.isValid() || this->Implementation->empty())
    return;

  QRect clip = area.intersect(this->Bounds);
  if(!clip.isValid())
    return;

  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setClipping(true);
  painter.setClipRect(clip);

  // Give all of the plots a chance to draw themselves
  for(pqImplementation::iterator plot = this->Implementation->begin(); plot != this->Implementation->end(); ++plot)
    {
    (*plot)->drawPlot(painter, area, *this->Implementation->XAxis, *this->Implementation->YAxis);
    }

  painter.restore();
}

void pqLineChart::showTooltip(QHelpEvent& e)
{
  int plot_index = -1;
  double plot_distance = VTK_DOUBLE_MAX;
  
  for(unsigned int i = 0; i != this->Implementation->size(); ++i)
    {
    const double distance = this->Implementation->at(i)->getDistance(e.pos());
    if(distance < plot_distance)
      {
      plot_index = i;
      plot_distance = distance;
      }
    }

  if(plot_index == -1 || plot_distance > 10)
    return;
    
  this->Implementation->at(plot_index)->showChartTip(e);
}

void pqLineChart::clearData()
{
  this->Implementation->clear();
}

void pqLineChart::updateAxes()
{
  if(this->Implementation->empty())
    return;

  pqChartCoordinate minimum(VTK_DOUBLE_MAX, VTK_DOUBLE_MAX);
  pqChartCoordinate maximum(-VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX);

  for(pqImplementation::iterator plot = this->Implementation->begin(); plot != this->Implementation->end(); ++plot)
    {
    const pqChartCoordinate plot_minimum = (*plot)->getMinimum();
    const pqChartCoordinate plot_maximum = (*plot)->getMaximum();
    
    minimum.X = vtkstd::min(minimum.X, plot_minimum.X);
    minimum.Y = vtkstd::min(minimum.Y, plot_minimum.Y);
    maximum.X = vtkstd::max(maximum.X, plot_maximum.X);
    maximum.Y = vtkstd::max(maximum.Y, plot_maximum.Y);
    }

  if(!this->Implementation->XShared &&
    this->Implementation->XAxis->getLayoutType() != pqChartAxis::FixedInterval)
    {
    this->Implementation->XAxis->setValueRange(minimum.X, maximum.X);
    }
    
  if(this->Implementation->YAxis->getLayoutType() != pqChartAxis::FixedInterval)
    {
    this->Implementation->YAxis->setValueRange(minimum.Y, maximum.Y);
    }
}
