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
#include "pqLinePlot.h"
#include "pqMarkerPen.h"

#include <QHelpEvent>
#include <QPainter>
#include <QPolygon>

#include <vtkMath.h>
#include <vtkstd/vector>
#include <vtkType.h>

/// \class pqLineChartItem
/// \brief
///   The pqLineChartItem class is used by the pqLineChart class
///   to store a function and its pixel equivalent.
class pqLineChartItem
{
public:
  pqLineChartItem();
  ~pqLineChartItem() {}

public:
  QPolygon Array;   ///< Stores the pixel coordinates.
  pqLinePlot *Plot; ///< Stores the line plot interface.
};


/// \class pqLineChartData
/// \brief
///   The pqLineChartData class hides the private data of the
///   pqLineChart class.
class pqLineChartData : public vtkstd::vector<pqLineChartItem *> {};


pqLineChartItem::pqLineChartItem()
  : Array()
{
  this->Plot = 0;
}


pqLineChart::pqLineChart(QObject *p)
  : QObject(p), Bounds()
{
  this->XAxis = 0;
  this->YAxis = 0;
  this->Data = new pqLineChartData();
  this->XShared = false;
}

pqLineChart::~pqLineChart()
{
  clearData();
  if(this->Data)
    delete this->Data;
}

void pqLineChart::setAxes(pqChartAxis *xAxis, pqChartAxis *yAxis,
    bool shared)
{
  this->XAxis = xAxis;
  this->YAxis = yAxis;
  this->XShared = shared;
}

void pqLineChart::addData(pqLinePlot *plot)
{
  if(!this->Data || !this->XAxis || !this->YAxis)
    return;

  // Don't allow the pointer to be added more than once.
  pqLineChartItem *item = 0;
  pqLineChartData::iterator iter = this->Data->begin();
  for( ; iter != this->Data->end(); iter++)
    {
    item = *iter;
    if(item && item->Plot == plot)
      return;
    }

  // Create a new pqLineChartItem for the plot.
  item = new pqLineChartItem();
  if(item)
    {
    item->Plot = plot;
    this->Data->push_back(item);

    // Listen to the plot changed signal.
    connect(item->Plot, SIGNAL(plotModified(const pqLinePlot *)), this,
        SLOT(handlePlotChanges(const pqLinePlot *)));

    pqChartCoordinate min;
    pqChartCoordinate max;
    item->Plot->getMinX(min.X);
    item->Plot->getMinY(min.Y);
    item->Plot->getMaxX(max.X);
    item->Plot->getMaxY(max.Y);

    // If neither of the axes needs to be changed, the new plot
    // can be layed out using the current pixel maping. Setting
    // the axis min and/or max should cause the entire chart to
    // be layed out again.
    if(!this->updateAxes(min, max, true))
      {
      // Make sure the chart gets repainted after laying out the item.
      this->layoutItem(item);
      emit this->repaintNeeded();
      }
    }
}

void pqLineChart::removeData(pqLinePlot *plot)
{
  if(!this->Data || !this->XAxis || !this->YAxis)
    return;

  // Find the chart min/max while searching for the item.
  bool firstItem = true;
  bool found = false;
  pqChartValue value;
  pqChartCoordinate min;
  pqChartCoordinate max;
  pqLineChartItem *item = 0;
  pqLineChartData::iterator iter = this->Data->begin();
  while(iter != this->Data->end())
    {
    item = *iter;
    if(item)
      {
      if(item->Plot == plot)
        {
        // Disconnect from the plot's signals.
        disconnect(item->Plot, 0, this, 0);

        // Remove the item from the list.
        iter = this->Data->erase(iter);
        delete item;
        found = true;
        }
      else
        {
        if(firstItem)
          {
          item->Plot->getMinX(min.X);
          item->Plot->getMinY(min.Y);
          item->Plot->getMaxX(max.X);
          item->Plot->getMaxY(max.Y);
          firstItem = false;
          }
        else
          {
          item->Plot->getMinX(value);
          if(value < min.X)
            min.X = value;
          item->Plot->getMinY(value);
          if(value < min.Y)
            min.Y = value;
          item->Plot->getMaxX(value);
          if(value > max.X)
            max.X = value;
          item->Plot->getMaxY(value);
          if(value > max.Y)
            max.Y = value;
          }

        ++iter;
        }
      }
    else
      {
      // Remove the empty item.
      iter = this->Data->erase(iter);
      }
    }

  // Don't resize the min/max if there are no plots to display.
  if(found && this->Data->size() > 0)
    this->updateAxes(min, max, false);
}

void pqLineChart::layoutChart()
{
  if(!this->Data || !this->XAxis || !this->YAxis)
    return;

  // Make sure the axes are valid.
  if(!this->XAxis->isValid() || !this->YAxis->isValid())
    return;

  // Set up the chart area based on the remaining space.
  this->Bounds.setTop(this->YAxis->getMaxPixel());
  this->Bounds.setLeft(this->XAxis->getMinPixel());
  this->Bounds.setRight(this->XAxis->getMaxPixel());
  this->Bounds.setBottom(this->YAxis->getMinPixel());

  // Loop through all the plot items to create the point arrays.
  pqLineChartData::iterator iter = this->Data->begin();
  for( ; iter != this->Data->end(); iter++)
    this->layoutItem(*iter);
}

void pqLineChart::drawChart(QPainter *p, const QRect &area)
{
  if(!this->Data || !p || !p->isActive() || !area.isValid())
    return;
  if(!this->Bounds.isValid() || this->Data->empty())
    return;

  QRect clip = area.intersect(this->Bounds);
  if(!clip.isValid())
    return;

  p->save();
  p->setRenderHint(QPainter::Antialiasing, true);

  // Set the clipping area to make sure the lines are drawn
  // inside the chart area.
  QRegion lastClip;
  bool wasClipping = p->hasClipping();
  if(wasClipping)
    lastClip = p->clipRegion();
  p->setClipping(true);
  p->setClipRect(clip);

  // Draw in all the plot items.
  pqLineChartData::iterator iter = this->Data->begin();
  for( ; iter != this->Data->end(); iter++)
    {
    pqLineChartItem *item = *iter;
    if(item && item->Plot && !item->Array.isEmpty())
      {
      // Set the drawing pen up for the plot
      pqMarkerPen& pen = item->Plot->getPen();

      // Draw the line segments.
      if(item->Plot->isPolyLine())
        {
        pen.drawPolyline(*p, item->Array);
        }
      else
        {
        for(int i = 1; i < item->Array.size(); i += 2)
          {
          pen.drawLine(*p, item->Array[i - 1], item->Array[i]);
          }
        }

      // TODO: Draw in the user editable points.
      }
    }

  // Restore the clipping to its previous state.
  p->setClipping(wasClipping);
  if(wasClipping)
    p->setClipRegion(lastClip);
    
  p->restore();
}

const double length(const QPoint p)
{
  return sqrt(static_cast<double>(p.x() * p.x() + p.y() * p.y()));
}

void pqLineChart::showTooltip(QHelpEvent& event)
{
  // Find the closest data ...
  int line_index = -1;
  int coordinate_index = -1;
  double coordinate_distance = VTK_DOUBLE_MAX;
  
  for(int i = 0; i != this->Data->size(); ++i)
    {
    for(int j = 0; j != this->Data->at(i)->Array.size(); ++j)
      {
      double distance = length(this->Data->at(i)->Array[j] - event.pos());
      if(distance < coordinate_distance)
        {
        line_index = i;
        coordinate_index = j;        
        coordinate_distance = distance;
        }
      }
    }
    
  if(line_index == -1 || coordinate_index == -1 || coordinate_distance > 10)
    return;
  
  this->Data->at(line_index)->Plot->showTooltip(coordinate_index, event);  
}

void pqLineChart::handlePlotChanges(const pqLinePlot *plot)
{
  if(!plot || !this->Data || !this->XAxis || !this->YAxis)
    return;

  // Make sure the axes are valid.
  if(!this->XAxis->isValid() || !this->YAxis->isValid())
    return;

  // Check to see if the plot item belongs to this chart.
  // Find the chart bounding rectangle while searching.
  bool firstItem = true;
  pqChartValue value;
  pqChartCoordinate min;
  pqChartCoordinate max;
  pqLineChartItem *item = 0;
  pqLineChartItem *match = 0;
  pqLineChartData::iterator iter = this->Data->begin();
  for( ; iter != this->Data->end(); iter++)
    {
    item = *iter;
    if(item)
      {
      if(item->Plot == plot)
        {
        match = item;
        if(!item->Plot->isModified())
          break;
        }

      if(plot->isModified())
        {
        if(firstItem)
          {
          item->Plot->getMinX(min.X);
          item->Plot->getMinY(min.Y);
          item->Plot->getMaxX(max.X);
          item->Plot->getMaxY(max.Y);
          firstItem = false;
          }
        else
          {
          item->Plot->getMinX(value);
          if(value < min.X)
            min.X = value;
          item->Plot->getMinY(value);
          if(value < min.Y)
            min.Y = value;
          item->Plot->getMaxX(value);
          if(value > max.X)
            max.X = value;
          item->Plot->getMaxY(value);
          if(value > max.Y)
            max.Y = value;
          }
        }
      }
    }

  if(!match)
    return;

  if(plot->isModified())
    {
    // If neither of the axes needs to be changed, the plot can
    // be layed out again using the current pixel maping. Setting
    // the axis min and/or max should cause the entire chart to
    // be layed out again.
    if(!this->updateAxes(min, max, false))
      {
      // Make sure the chart gets repainted after laying out the item.
      this->layoutItem(match);
      emit this->repaintNeeded();
      }
    }
  else
    {
    // The color or line width has changed so the plot needs to
    // be repainted.
    emit this->repaintNeeded();
    }
}

void pqLineChart::clearData()
{
  if(this->Data)
    {
    pqLineChartData::iterator iter = this->Data->begin();
    for( ; iter != this->Data->end(); iter++)
      delete *iter;
    this->Data->clear();
    }
}

void pqLineChart::layoutItem(pqLineChartItem *item)
{
  if(!item || !item->Plot)
    return;

  // Flag the item as not modified.
  item->Plot->setModified(false);

  // Make sure the array can hold the data.
  int total = item->Plot->getCoordinateCount();
  item->Array.resize(total);

  // Transform the plot data into pixel coordinates.
  int px = 0;
  int py = 0;
  pqChartCoordinate coord;
  for(int i = 0; i < total; i++)
    {
    if(item->Plot->getCoordinate(i, coord))
      {
      px = this->XAxis->getPixelFor(coord.X);
      py = this->YAxis->getPixelFor(coord.Y);
      }

    item->Array.setPoint(i, px, py);
    }
}

bool pqLineChart::updateAxes(pqChartCoordinate &min, pqChartCoordinate &max,
    bool fromAdd)
{
  bool xChanged = false;
  bool yChanged = false;

  // Make sure the x and y axes are large enough to contain the plot.
  // Only try to update a best interval axis. Changing the max on a
  // fixed interval axis can mess up the interval.
  if(!this->XShared &&
      this->XAxis->getLayoutType() != pqChartAxis::FixedInterval)
    {
    if(fromAdd)
      {
      if(this->Data->size() > 1)
        {
        xChanged = min.X < this->XAxis->getMinValue() ||
            max.X > this->XAxis->getMaxValue();
        if(xChanged)
          {
          if(min.X > this->XAxis->getMinValue())
            {
            min.X = this->XAxis->getMinValue();
            }
          if(max.X < this->XAxis->getMaxValue())
            {
            max.X = this->XAxis->getMaxValue();
            }
          }
        }
      else
        {
        xChanged = true;
        }
      }
    else
      {
      xChanged = min.X != this->XAxis->getMinValue() ||
          max.X != this->XAxis->getMaxValue();
      }
    }

  if(this->YAxis->getLayoutType() != pqChartAxis::FixedInterval)
    {
    if(fromAdd)
      {
      if(this->Data->size() > 1)
        {
        yChanged = min.Y < this->YAxis->getMinValue() ||
            max.Y > this->YAxis->getMaxValue();
        if(yChanged)
          {
          if(min.Y > this->YAxis->getMinValue())
            {
            min.Y = this->YAxis->getMinValue();
            }
          if(max.Y < this->YAxis->getMaxValue())
            {
            max.Y = this->YAxis->getMaxValue();
            }
          }
        }
      else
        {
        yChanged = true;
        }
      }
    else
      {
      yChanged = min.Y != this->YAxis->getMinValue() ||
          max.Y != this->YAxis->getMaxValue();
      }
    }

  if(yChanged)
    {
    // If the x-axis has changed too, block the y-axis update
    // signal to avoid laying out the chart twice.
    if(xChanged)
      this->YAxis->blockSignals(true);
    this->YAxis->setValueRange(min.Y, max.Y);
    if(xChanged)
      this->YAxis->blockSignals(false);
    }

  if(xChanged)
    {
    this->XAxis->setValueRange(min.X, max.X);
    }

  return xChanged || yChanged;
}


