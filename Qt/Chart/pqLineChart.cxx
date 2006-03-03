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
    for(int i = 0; i != this->size(); ++i)
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

void pqLineChart::showTooltip(QHelpEvent& event)
{
  int plot_index = -1;
  double plot_distance = VTK_DOUBLE_MAX;
  
  for(int i = 0; i != this->Implementation->size(); ++i)
    {
    const double distance = this->Implementation->at(i)->getDistance(event.pos());
    if(distance < plot_distance)
      {
      plot_index = i;
      plot_distance = distance;
      }
    }

  if(plot_index == -1 || plot_distance > 10)
    return;
    
  this->Implementation->at(plot_index)->showChartTip(event);
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
