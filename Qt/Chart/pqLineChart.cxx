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

#include "pqLineChart.h"

#include "pqAbstractPlot.h"
#include "pqChartAxis.h"
#include "pqChartCoordinate.h"
#include "pqLineChartModel.h"
#include "pqLineChartPlot.h"
#include "pqLineChartPlotOptions.h"
#include "pqMarkerPen.h"
#include "pqPointMarker.h"

#include <QHelpEvent>
#include <QList>
#include <QPainter>
#include <QPolygon>
#include <QtDebug>
#include <QVector>


class pqLineChartItemData
{
public:
  pqLineChartItemData() {}
  virtual ~pqLineChartItemData() {}

  virtual void clear() {}
};


class pqLineChartItemPoints : public pqLineChartItemData
{
public:
  pqLineChartItemPoints();
  virtual ~pqLineChartItemPoints() {}

  virtual void clear() {this->Series.clear();}

  QPolygon Series;
};


class pqLineChartItemLine : public pqLineChartItemData
{
public:
  pqLineChartItemLine();
  virtual ~pqLineChartItemLine() {}

  virtual void clear() {this->Series.clear();}

  QList<QPolygon> Series;
};


class pqLineChartItemErrorData
{
public:
  pqLineChartItemErrorData();
  ~pqLineChartItemErrorData() {}

  int X;
  int Upper;
  int Lower;
};


class pqLineChartItemError : public pqLineChartItemData
{
public:
  pqLineChartItemError();
  virtual ~pqLineChartItemError() {}

  virtual void clear() {this->Series.clear();}

  QVector<pqLineChartItemErrorData> Series;
  int Width;
};


class pqLineChartItem
{
public:
  pqLineChartItem(const pqLineChartPlot *plot);
  ~pqLineChartItem() {}

  QList<pqLineChartItemData *> Series;
  const pqLineChartPlot *Plot;
  bool NeedsLayout;
};


class pqLineChartInternal
{
public:
  pqLineChartInternal();
  ~pqLineChartInternal() {}

  QList<pqLineChartItem *> Plots;
  QVector<pqLineChartPlotOptions *> PlotOptions;
  QList<const pqLineChartPlot *> MultiSeries;
};


//----------------------------------------------------------------------------
pqLineChartItemPoints::pqLineChartItemPoints()
  : pqLineChartItemData(), Series()
{
};


//----------------------------------------------------------------------------
pqLineChartItemLine::pqLineChartItemLine()
  : pqLineChartItemData(), Series()
{
};


//----------------------------------------------------------------------------
pqLineChartItemErrorData::pqLineChartItemErrorData()
{
  this->X = 0;
  this->Upper = 0;
  this->Lower = 0;
}


//----------------------------------------------------------------------------
pqLineChartItemError::pqLineChartItemError()
  : pqLineChartItemData(), Series()
{
  this->Width = 0;
}


//----------------------------------------------------------------------------
pqLineChartItem::pqLineChartItem(const pqLineChartPlot *plot)
  : Series()
{
  this->Plot = plot;
  this->NeedsLayout = true;
}


//----------------------------------------------------------------------------
pqLineChartInternal::pqLineChartInternal()
  : Plots(), PlotOptions(), MultiSeries()
{
}


//----------------------------------------------------------------------------
pqLineChart::pqLineChart(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqLineChartInternal();
  this->Model = 0;
  this->XAxis = 0;
  this->YAxis = 0;
  this->XShared = false;
  this->NeedsLayout = true;
}

pqLineChart::~pqLineChart()
{
  this->clearData();
  delete this->Internal;
}

void pqLineChart::setAxes(pqChartAxis *xAxis, pqChartAxis *yAxis,
    bool shared)
{
  this->XAxis = xAxis;
  this->YAxis = yAxis;
  this->XShared = shared;

  // If the model is set, update the axis ranges from the model.
  if(this->Model)
    {
    this->updateAxisRanges();
    emit this->layoutNeeded();
    }
}

void pqLineChart::setModel(pqLineChartModel *model)
{
  if(this->Model == model)
    {
    return;
    }

  // Clean up the previous chart data.
  this->clearData();

  if(this->Model)
    {
    // Disconnect from the previous model's signals.
    this->disconnect(this->Model, 0, this, 0);
    }

  this->Model = model;
  if(this->Model)
    {
    // Connect to the new model's signals.
    this->connect(this->Model, SIGNAL(plotsReset()),
        this, SLOT(handleModelReset()));
    this->connect(this->Model, SIGNAL(plotMoved(int, int)),
        this, SLOT(handlePlotMoved(int, int)));
    this->connect(this->Model, SIGNAL(plotReset(const pqLineChartPlot *)),
        this, SLOT(handlePlotReset(const pqLineChartPlot *)));
    }

  // Set up the axis ranges and update the chart layout.
  this->buildPlotList();
  this->updateAxisRanges();
  emit this->layoutNeeded();
}

void pqLineChart::setPlotOptions(pqLineChartPlotOptions *options, int plot)
{
  if(plot < 0)
    {
    return;
    }

  // Expand the vector to fit the plot index if needed.
  if(plot >= this->Internal->PlotOptions.size())
    {
    int i = this->Internal->PlotOptions.size();
    this->Internal->PlotOptions.resize(plot + 1);
    for( ; i < this->Internal->PlotOptions.size(); i++)
      {
      this->Internal->PlotOptions[i] = 0;
      }
    }

  this->Internal->PlotOptions[plot] = options;
}

void pqLineChart::clearPlotOptions()
{
  this->Internal->PlotOptions.clear();
}

pqLineChartPlotOptions *pqLineChart::getPlotOptions(int plot) const
{
  if(plot >= 0 && plot < this->Internal->PlotOptions.size())
    {
    return this->Internal->PlotOptions[plot];
    }

  return 0;
}

void pqLineChart::layoutChart()
{
  if(!this->XAxis || !this->YAxis)
    {
    return;
    }

  // Make sure the axes are valid.
  if(!this->XAxis->isValid() || !this->YAxis->isValid())
    {
    return;
    }

  // Set up the chart area based on the remaining space.
  this->Bounds.setTop(this->YAxis->getMaxPixel());
  this->Bounds.setLeft(this->XAxis->getMinPixel());
  this->Bounds.setRight(this->XAxis->getMaxPixel());
  this->Bounds.setBottom(this->YAxis->getMinPixel());

  // Layout each of the plots. Only, layout the plot if it has changed
  // or if either of the axis scales have changed.
  int i = 0;
  QList<pqLineChartItem *>::Iterator iter = this->Internal->Plots.begin();
  for( ; iter != this->Internal->Plots.end(); ++iter)
    {
    if(this->NeedsLayout || (*iter)->NeedsLayout)
      {
      // Make sure data objects have been allocated for each plot
      // series. We may want to add support for dynamic series.
      (*iter)->NeedsLayout = false;
      if((*iter)->Series.size() == 0)
        {
        for(i = 0; i < (*iter)->Plot->getNumberOfSeries(); i++)
          {
          pqLineChartPlot::SeriesType type = (*iter)->Plot->getSeriesType(i);
          if(type == pqLineChartPlot::Point)
            {
            (*iter)->Series.append(new pqLineChartItemPoints());
            }
          else if(type == pqLineChartPlot::Line)
            {
            (*iter)->Series.append(new pqLineChartItemLine());
            }
          else if(type == pqLineChartPlot::Error)
            {
            (*iter)->Series.append(new pqLineChartItemError());
            }
          else
            {
            qWarning("Unknown plot series type.");
            break;
            }
          }
        }

      if((*iter)->Series.size() != (*iter)->Plot->getNumberOfSeries())
        {
        qWarning("Plot layout data invalid.");
        continue;
        }

      QList<pqLineChartItemData *>::Iterator series = (*iter)->Series.begin();
      for(i = 0; series != (*iter)->Series.end(); ++series, ++i)
        {
        // Clear out the previous layout data.
        (*series)->clear();
        pqLineChartItemPoints *points = dynamic_cast<pqLineChartItemPoints *>(
            *series);
        pqLineChartItemLine *line = dynamic_cast<pqLineChartItemLine *>(
            *series);
        pqLineChartItemError *error = dynamic_cast<pqLineChartItemError *>(
            *series);

        QPolygon *polygon = 0;
        int total = (*iter)->Plot->getNumberOfPoints(i);
        for(int j = 0; j < total; j++)
          {
          // Get the point coordinate from the plot.
          pqChartCoordinate coord;
          (*iter)->Plot->getPoint(i, j, coord);

          // Use the axis scale to translate the plot's coordinates
          // into chart coordinates.
          QPoint point(this->XAxis->getPixelFor(coord.X),
              this->YAxis->getPixelFor(coord.Y));

          // Store the pixel coordinates for the point.
          if(line)
            {
            // Qt has a bug drawing polylines with large data sets, so
            // break them up into manageable chunks.
            if(!polygon || j % 100 == 0)
              {
              line->Series.append(QPolygon());
              polygon = &line->Series.last();
              polygon->reserve(101);

              // Copy the last point from the previous polygon to the
              // new one.
              if(line->Series.size() > 1)
                {
                polygon->append(line->Series[line->Series.size() - 2].last());
                }
              }

            polygon->append(point);
            }
          else if(points)
            {
            points->Series.append(point);
            }
          else if(error)
            {
            // If this is the first point, calculate the error width
            // in pixels using the point and the error width.
            if(j == 0)
              {
              // Get the error width for the series.
              pqChartValue width;
              (*iter)->Plot->getErrorWidth(i, width);
              error->Width = this->XAxis->getPixelFor(coord.X + width);
              error->Width -= point.x();
              }

            pqLineChartItemErrorData errorData;
            errorData.X = point.x();

            // Get the error bounds for the point.
            pqChartValue upper, lower;
            (*iter)->Plot->getErrorBounds(i, j, upper, lower);
            errorData.Upper = this->YAxis->getPixelFor(upper);
            errorData.Lower = this->YAxis->getPixelFor(lower);

            error->Series.append(errorData);
            }
          }
        }
      }
    }

  // Reset the layout flag to true so all changes will be noted. The
  // flag can be set to false when a new plot is added or a plot gets
  // new points.
  this->NeedsLayout = true;
}

void pqLineChart::drawChart(QPainter& painter, const QRect& area)
{
  if(!painter.isActive() || !area.isValid())
    {
    return;
    }
    
  if(!this->Bounds.isValid())
    {
    return;
    }

  QRect clip = area.intersect(this->Bounds);
  if(!clip.isValid())
    {
    return;
    }

  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setClipping(true);
  painter.setClipRect(clip);

  // Draw each of the plots.
  QList<pqLineChartItem *>::Iterator iter = this->Internal->Plots.begin();
  for(int i = 0; iter != this->Internal->Plots.end(); ++iter, ++i)
    {
    pqLineChartPlotOptions *options = this->getPlotOptions(i);
    QList<pqLineChartItemData *>::Iterator series = (*iter)->Series.begin();
    for(int j = 0; series != (*iter)->Series.end(); ++series, ++j)
      {
      // Set up the painter options for the series.
      if(options)
        {
        options->setupPainter(painter, j);
        }

      pqLineChartItemPoints *points = dynamic_cast<pqLineChartItemPoints *>(
          *series);
      pqLineChartItemLine *line = dynamic_cast<pqLineChartItemLine *>(
          *series);
      pqLineChartItemError *error = dynamic_cast<pqLineChartItemError *>(
          *series);
      if(points)
        {
        // Use the marker to draw the points.
        pqPointMarker *marker = options ? options->getMarker(j) : 0;
        if(!marker)
          {
          // TODO: Use a default marker.
          continue;
          }

        // Translate the painter for each point.
        QPolygon::Iterator jter = points->Series.begin();
        for( ; jter != points->Series.end(); ++jter)
          {
          painter.save();
          painter.translate(*jter);
          marker->drawMarker(painter);
          painter.restore();
          }
        }
      else if(line)
        {
        // Draw each polyline segment of the line. The line is broken
        // up to avoid a Qt bug when the polyline is large.
        QList<QPolygon>::Iterator jter = line->Series.begin();
        for( ; jter != line->Series.end(); ++jter)
          {
          painter.drawPolyline(*jter);
          }
        }
      else if(error)
        {
        // Get the error width from the options.
        QVector<pqLineChartItemErrorData>::Iterator jter;
        for(jter = error->Series.begin(); jter != error->Series.end(); ++jter)
          {
          // Draw a line from the upper bounds to the lower bounds
          // through the point.
          painter.drawLine(jter->X, jter->Upper, jter->X, jter->Lower);

          // Draw lines to cap the error bounds.
          if(error->Width > 0)
            {
            painter.drawLine(jter->X - error->Width, jter->Upper,
                jter->X + error->Width, jter->Upper);
            painter.drawLine(jter->X - error->Width, jter->Lower,
                jter->X + error->Width, jter->Lower);
            }
          }
        }
      }
    }

  painter.restore();
}

/*void pqLineChart::showTooltip(QHelpEvent *e)
{
  int plot_index = -1;
  double plot_distance = VTK_DOUBLE_MAX;
  
  for(unsigned int i = 0; i != this->Implementation->size(); ++i)
    {
    const double distance = this->Implementation->at(i)->getDistance(e->pos());
    if(distance < plot_distance)
      {
      plot_index = i;
      plot_distance = distance;
      }
    }

  if(plot_index == -1 || plot_distance > 10)
    return;
    
  this->Implementation->at(plot_index)->showChartTip(e);
}*/

void pqLineChart::handleModelReset()
{
  // Clean up the previous layout data.
  this->clearData();

  // Set up the axis ranges and update the chart layout.
  this->buildPlotList();
  this->updateAxisRanges();
  emit this->layoutNeeded();
}

void pqLineChart::startPlotInsertion(int, int)
{
  // TODO: Add selection support.
}

void pqLineChart::finishPlotInsertion(int first, int last)
{
  // Add new chart items for the new plots.
  for(int i = first; i <= last; i++)
    {
    const pqLineChartPlot *plot = this->Model->getPlot(i);
    this->Internal->Plots.insert(i, new pqLineChartItem(plot));
    }

  // Update the axis ranges for the new plots. If the plots can fit
  // in the current layout, only the new plots need to be layed out.
  this->NeedsLayout = false;
  this->updateAxisRanges(false);
  emit this->layoutNeeded();
}

void pqLineChart::startPlotRemoval(int first, int last)
{
  // Remove the internal items from the list.
  for(int i = last; i >= first; i--)
    {
    delete this->Internal->Plots.takeAt(i);
    }

  // TODO: Add selection support.
}

void pqLineChart::finishPlotRemoval(int, int)
{
  // Update the axis ranges. The removed plots may modify the ranges.
  this->NeedsLayout = false;
  this->updateAxisRanges(false);
  if(this->NeedsLayout)
    {
    emit this->layoutNeeded();
    }
  else
    {
    emit this->repaintNeeded();
    }
}

void pqLineChart::handlePlotMoved(int current, int index)
{
  // Moving the plot only affects the painting layer. Move the
  // corresponding chart item and signal a repaint.
  pqLineChartItem *item = this->Internal->Plots.takeAt(current);
  this->Internal->Plots.insert(index, item);
  emit this->repaintNeeded();
}

void pqLineChart::handlePlotReset(const pqLineChartPlot *plot)
{
  // Find the given plot's layout item.
  bool found = false;
  QList<pqLineChartItem *>::Iterator iter = this->Internal->Plots.begin();
  for( ; iter != this->Internal->Plots.end(); ++iter)
    {
    if((*iter)->Plot == plot)
      {
      QList<pqLineChartItemData *>::Iterator series = (*iter)->Series.begin();
      for( ; series != (*iter)->Series.end(); ++series)
        {
        delete *series;
        }

      (*iter)->Series.clear();
      (*iter)->NeedsLayout = true;
      found = true;
      }
    }

  if(found)
    {
    this->NeedsLayout = false;
    this->updateAxisRanges(false);
    emit this->layoutNeeded();
    }
}

void pqLineChart::startPointInsertion(const pqLineChartPlot *, int, int, int)
{
  // TODO: Add selection support.
}

void pqLineChart::finishPointInsertion(const pqLineChartPlot *plot, int)
{
  // If this is a multi-series change, wait for the completion.
  if(!this->Internal->MultiSeries.contains(plot))
    {
    // Mark the plot for re-layout.
    pqLineChartItem *item = this->getItem(plot);
    item->NeedsLayout = true;

    // Check the axis ranges for the point insertion.
    this->NeedsLayout = false;
    this->updateAxisRanges(false);
    emit this->layoutNeeded();
    }
}

void pqLineChart::startPointRemoval(const pqLineChartPlot *, int, int, int)
{
  // TODO: Add selection support.
}

void pqLineChart::finishPointRemoval(const pqLineChartPlot *plot, int)
{
  // If this is a multi-series change, wait for the completion.
  if(!this->Internal->MultiSeries.contains(plot))
    {
    // Mark the plot for re-layout.
    pqLineChartItem *item = this->getItem(plot);
    item->NeedsLayout = true;

    // Update the axis ranges for the plot modification.
    this->NeedsLayout = false;
    this->updateAxisRanges(false);
    emit this->layoutNeeded();
    }
}

void pqLineChart::startMultiSeriesChange(const pqLineChartPlot *plot)
{
  // Keep track of the plot so all the changes can be made before
  // updating the layout.
  this->Internal->MultiSeries.append(plot);
}

void pqLineChart::finishMultiSeriesChange(const pqLineChartPlot *plot)
{
  // Update the axis ranges and the layout for the changes.
  this->Internal->MultiSeries.removeAll(plot);
  pqLineChartItem *item = this->getItem(plot);
  item->NeedsLayout = true;
  this->NeedsLayout = false;
  this->updateAxisRanges(false);
  emit this->layoutNeeded();
}

void pqLineChart::handlePlotErrorBoundsChanged(const pqLineChartPlot *plot,
    int, int, int)
{
  pqLineChartItem *item = this->getItem(plot);
  item->NeedsLayout = true;
  this->NeedsLayout = false;
  this->updateAxisRanges(false);
  emit this->layoutNeeded();
}

void pqLineChart::handlePlotErrorWidthChanged(const pqLineChartPlot *plot, int)
{
  pqLineChartItem *item = this->getItem(plot);
  item->NeedsLayout = true;
  emit this->layoutNeeded();
}

void pqLineChart::updateAxisRanges(bool force)
{
  if(!this->XAxis || !this->YAxis || !this->Model)
    {
    return;
    }

  // Get the current axis ranges from the model.
  pqChartCoordinate min, max;
  this->Model->getRangeX(min.X, max.X);
  this->Model->getRangeY(min.Y, max.Y);

  // Set the axis ranges. Only check the ranges if force is false.
  // Block the axis signals to prevent premature chart layout.
  // TODO: How to better handle a combined histogram and line chart
  // axis. Right now, the histogram controls the range.
  if(!this->XShared &&
      this->XAxis->getLayoutType() != pqChartAxis::FixedInterval &&
      (force || min.X != this->XAxis->getTrueMinValue() ||
      max.X != this->XAxis->getTrueMaxValue()))
    {
    this->NeedsLayout = true;
    this->XAxis->blockSignals(true);
    this->XAxis->setValueRange(min.X, max.X);
    this->XAxis->blockSignals(false);
    }

  if(this->YAxis->getLayoutType() != pqChartAxis::FixedInterval &&
      (force || min.Y != this->YAxis->getTrueMinValue() ||
      max.Y != this->YAxis->getTrueMaxValue()))
    {
    this->NeedsLayout = true;
    this->YAxis->blockSignals(true);
    this->YAxis->setValueRange(min.Y, max.Y);
    this->YAxis->blockSignals(false);
    }
}

void pqLineChart::clearData()
{
  // Clean up the layout data.
  QList<pqLineChartItemData *>::Iterator series;
  QList<pqLineChartItem *>::Iterator plot = this->Internal->Plots.begin();
  for( ; plot != this->Internal->Plots.end(); ++plot)
    {
    series = (*plot)->Series.begin();
    for( ; series != (*plot)->Series.end(); ++series)
      {
      delete *series;
      }

    delete *plot;
    }

  this->Internal->Plots.clear();
}

void pqLineChart::buildPlotList()
{
  if(!this->Model)
    {
    return;
    }

  // Make a line chart item for each plot in the model.
  for(int i = 0; i < this->Model->getNumberOfPlots(); i++)
    {
    this->Internal->Plots.append(new pqLineChartItem(this->Model->getPlot(i)));
    }
}

pqLineChartItem *pqLineChart::getItem(const pqLineChartPlot *plot) const
{
  pqLineChartItem *item = 0;
  QList<pqLineChartItem *>::Iterator iter = this->Internal->Plots.begin();
  for( ; iter != this->Internal->Plots.end(); ++iter)
    {
    if((*iter)->Plot == plot)
      {
      item = *iter;
      break;
      }
    }

  return item;
}


