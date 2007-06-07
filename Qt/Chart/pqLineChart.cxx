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

/// \file pqLineChart.cxx
/// \date 2/28/2007

#include "pqLineChart.h"

#include "pqChartAxis.h"
#include "pqChartContentsSpace.h"
#include "pqChartCoordinate.h"
#include "pqChartSeriesOptionsGenerator.h"
#include "pqChartValue.h"
#include "pqLineChartModel.h"
#include "pqLineChartOptions.h"
#include "pqLineChartSeries.h"
#include "pqLineChartSeriesOptions.h"
#include "pqChartPixelScale.h"
#include "pqPointMarker.h"

#include <QColor>
#include <QList>
#include <QPainter>
#include <QPolygon>
#include <QRect>
#include <QtDebug>
#include <QVector>


class pqLineChartSeriesItemData
{
public:
  pqLineChartSeriesItemData() {}
  virtual ~pqLineChartSeriesItemData() {}

  virtual void clear() {}
};


class pqLineChartSeriesPointData : public pqLineChartSeriesItemData
{
public:
  pqLineChartSeriesPointData();
  virtual ~pqLineChartSeriesPointData() {}

  virtual void clear() {this->Sequence.clear();}

  QPolygon Sequence;
};


class pqLineChartSeriesLineData : public pqLineChartSeriesItemData
{
public:
  pqLineChartSeriesLineData();
  virtual ~pqLineChartSeriesLineData() {}

  virtual void clear() {this->Sequence.clear();}

  QList<QPolygon> Sequence;
};


class pqLineChartSeriesErrorDataItem
{
public:
  pqLineChartSeriesErrorDataItem();
  ~pqLineChartSeriesErrorDataItem() {}

  int X;
  int Upper;
  int Lower;
};


class pqLineChartSeriesErrorData : public pqLineChartSeriesItemData
{
public:
  pqLineChartSeriesErrorData();
  virtual ~pqLineChartSeriesErrorData() {}

  virtual void clear() {this->Sequence.clear();}

  QVector<pqLineChartSeriesErrorDataItem> Sequence;
  int Width;
};


class pqLineChartSeriesItem
{
public:
  pqLineChartSeriesItem(const pqLineChartSeries *series);
  ~pqLineChartSeriesItem() {}

  QList<pqLineChartSeriesItemData *> Sequences;
  const pqLineChartSeries *Series;
  bool NeedsLayout;
};


class pqLineChartInternal
{
public:
  pqLineChartInternal();
  ~pqLineChartInternal() {}

  /// Stores the series data list.
  QList<pqLineChartSeriesItem *> Series;

  /// Stores the series in multi-sequence changes.
  QList<const pqLineChartSeries *> MultiSequences;
  pqSquarePointMarker Marker; /// Stores the default marker.
  QRect Bounds;               ///< Stores the chart bounds.
  QRect Contents;             ///< Stores the contents area.
};


//----------------------------------------------------------------------------
pqLineChartSeriesPointData::pqLineChartSeriesPointData()
  : pqLineChartSeriesItemData(), Sequence()
{
};


//----------------------------------------------------------------------------
pqLineChartSeriesLineData::pqLineChartSeriesLineData()
  : pqLineChartSeriesItemData(), Sequence()
{
};


//----------------------------------------------------------------------------
pqLineChartSeriesErrorDataItem::pqLineChartSeriesErrorDataItem()
{
  this->X = 0;
  this->Upper = 0;
  this->Lower = 0;
}


//----------------------------------------------------------------------------
pqLineChartSeriesErrorData::pqLineChartSeriesErrorData()
  : pqLineChartSeriesItemData(), Sequence()
{
  this->Width = 0;
}


//----------------------------------------------------------------------------
pqLineChartSeriesItem::pqLineChartSeriesItem(const pqLineChartSeries *series)
  : Sequences()
{
  this->Series = series;
  this->NeedsLayout = true;
}


//----------------------------------------------------------------------------
pqLineChartInternal::pqLineChartInternal()
  : Series(), MultiSequences(), Marker(QSize(3, 3)), Bounds(), Contents()
{
}


//----------------------------------------------------------------------------
pqLineChart::pqLineChart(QObject *parentObject)
  : pqChartLayer(parentObject)
{
  this->Internal = new pqLineChartInternal();
  this->Options = new pqLineChartOptions(this);
  this->Model = 0;
  this->XAxis = 0;
  this->YAxis = 0;
  this->NeedsLayout = false;

  this->connect(this->Options, SIGNAL(optionsChanged()),
      this, SLOT(handleSeriesOptionsChanged()));
}

pqLineChart::~pqLineChart()
{
  this->clearSeriesList();
  delete this->Internal;
}

void pqLineChart::setAxes(pqChartAxis *xAxis, pqChartAxis *yAxis)
{
  if(xAxis == this->XAxis && yAxis == this->YAxis)
    {
    return;
    }

  if(xAxis->getLocation() == pqChartAxis::Left ||
      xAxis->getLocation() == pqChartAxis::Right)
    {
    qCritical() << "Error: The x-axis must be a horizontal axis.";
    return;
    }

  if(yAxis->getLocation() == pqChartAxis::Top ||
      yAxis->getLocation() == pqChartAxis::Bottom)
    {
    qCritical() << "Error: The y-axis must be a vertical axis.";
    return;
    }

  if(this->XAxis)
    {
    this->disconnect(this->XAxis, 0, this, 0);
    }

  if(this->YAxis)
    {
    this->disconnect(this->YAxis, 0, this, 0);
    }

  this->XAxis = xAxis;
  this->YAxis = yAxis;
  this->NeedsLayout = true;
  if(this->XAxis)
    {
    this->connect(this->XAxis, SIGNAL(pixelScaleChanged()),
        this, SLOT(handleRangeChange()));
    }

  if(this->YAxis)
    {
    this->connect(this->YAxis, SIGNAL(pixelScaleChanged()),
        this, SLOT(handleRangeChange()));
    }
}

void pqLineChart::setModel(pqLineChartModel *model)
{
  if(this->Model == model)
    {
    return;
    }

  // Clean up the previous chart data.
  this->clearSeriesList();

  if(this->Model)
    {
    // Disconnect from the previous model's signals.
    this->disconnect(this->Model, 0, this, 0);
    this->disconnect(this->Model, 0, this->Options, 0);
    }

  this->Model = model;
  if(this->Model)
    {
    // Connect to the new model's signals. Connect the options before
    // this so the options object will be ready.
    this->connect(this->Model, SIGNAL(seriesInserted(int, int)),
        this->Options, SLOT(insertSeriesOptions(int, int)));
    this->connect(this->Model, SIGNAL(aboutToRemoveSeries(int, int)),
        this->Options, SLOT(removeSeriesOptions(int, int)));
    this->connect(this->Model, SIGNAL(seriesMoved(int, int)),
        this->Options, SLOT(moveSeriesOptions(int, int)));

    this->connect(this->Model, SIGNAL(modelReset()),
        this, SLOT(handleModelReset()));
    this->connect(this->Model, SIGNAL(aboutToInsertSeries(int, int)),
        this, SLOT(startSeriesInsertion(int, int)));
    this->connect(this->Model, SIGNAL(seriesInserted(int, int)),
        this, SLOT(finishSeriesInsertion(int, int)));
    this->connect(this->Model, SIGNAL(aboutToRemoveSeries(int, int)),
        this, SLOT(startSeriesRemoval(int, int)));
    this->connect(this->Model, SIGNAL(seriesRemoved(int, int)),
        this, SLOT(finishSeriesRemoval(int, int)));
    this->connect(this->Model, SIGNAL(seriesMoved(int, int)),
        this, SLOT(handleSeriesMoved(int, int)));
    this->connect(this->Model, SIGNAL(seriesReset(const pqLineChartSeries *)),
        this, SLOT(handleSeriesReset(const pqLineChartSeries *)));
    this->connect(this->Model,
        SIGNAL(aboutToInsertPoints(const pqLineChartSeries *, int, int, int)),
        this,
        SLOT(startPointInsertion(const pqLineChartSeries *, int, int, int)));
    this->connect(this->Model,
        SIGNAL(pointsInserted(const pqLineChartSeries *, int)),
        this, SLOT(finishPointInsertion(const pqLineChartSeries *, int)));
    this->connect(this->Model,
        SIGNAL(aboutToRemovePoints(const pqLineChartSeries *, int, int, int)),
        this,
        SLOT(startPointRemoval(const pqLineChartSeries *, int, int, int)));
    this->connect(this->Model,
        SIGNAL(pointsRemoved(const pqLineChartSeries *, int)),
        this, SLOT(finishPointRemoval(const pqLineChartSeries *, int)));
    this->connect(this->Model,
        SIGNAL(aboutToChangeMultipleSeries(const pqLineChartSeries *)),
        this, SLOT(startMultiSeriesChange(const pqLineChartSeries *)));
    this->connect(this->Model,
        SIGNAL(changedMultipleSeries(const pqLineChartSeries *)),
        this, SLOT(finishMultiSeriesChange(const pqLineChartSeries *)));
    this->connect(this->Model,
        SIGNAL(errorBoundsChanged(const pqLineChartSeries *, int, int, int)),
        this,
        SLOT(handleSeriesErrorBoundsChanged(const pqLineChartSeries *, int, int, int)));
    this->connect(this->Model,
        SIGNAL(errorWidthChanged(const pqLineChartSeries *, int)),
        this,
        SLOT(handleSeriesErrorWidthChanged(const pqLineChartSeries *, int)));
    this->connect(this->Model, SIGNAL(chartRangeChanged()),
        this, SLOT(handleRangeChange()));
    this->connect(this->Model, SIGNAL(chartRangeChanged()),
        this, SIGNAL(rangeChanged()));
    }

  this->resetSeriesOptions();

  // Build a list of the current data.
  this->buildSeriesList();

  // Set up the axis ranges and update the chart layout.
  emit this->rangeChanged();
  emit this->layoutNeeded();
}

void pqLineChart::resetSeriesOptions()
{
  this->Options->clearSeriesOptions();
  if(this->Model && this->Model->getNumberOfSeries() > 0)
    {
    this->Options->insertSeriesOptions(0, this->Model->getNumberOfSeries() - 1);
    emit this->repaintNeeded();
    }
}

bool pqLineChart::getAxisRange(const pqChartAxis *axis,
    pqChartValue &min, pqChartValue &max, bool &, bool &) const
{
  if(this->Model && this->Model->getNumberOfSeries() > 0)
    {
    if(axis == this->XAxis)
      {
      this->Model->getRangeX(min, max);
      return true;
      }
    else if(axis == this->YAxis)
      {
      this->Model->getRangeY(min, max);
      return true;
      }
    }

  return false;
}

void pqLineChart::layoutChart(const QRect &area)
{
  if(!this->Model || !this->XAxis || !this->YAxis || !area.isValid())
    {
    return;
    }

  // Make sure the axes are valid.
  const pqChartPixelScale *xScale = this->XAxis->getPixelValueScale();
  const pqChartPixelScale *yScale = this->YAxis->getPixelValueScale();
  if(!xScale->isValid() || !yScale->isValid())
    {
    return;
    }

  // Save the chart bounds.
  this->Internal->Bounds = area;

  // Set up the contents size.
  this->Internal->Contents = this->Internal->Bounds;
  const pqChartContentsSpace *zoomPan = this->getContentsSpace();
  if(zoomPan)
    {
    this->Internal->Contents.setRight(this->Internal->Contents.right() +
        zoomPan->getMaximumXOffset());
    this->Internal->Contents.setBottom(this->Internal->Contents.bottom() +
        zoomPan->getMaximumYOffset());
    }

  // Layout each of the series. Only, layout the series if it has
  // changed or if either of the axis scales have changed.
  int i = 0;
  QList<pqLineChartSeriesItem *>::Iterator iter = this->Internal->Series.begin();
  for( ; iter != this->Internal->Series.end(); ++iter)
    {
    if(this->NeedsLayout || (*iter)->NeedsLayout)
      {
      // Make sure data objects have been allocated for each series
      // sequence. We may want to add support for dynamic sequences.
      (*iter)->NeedsLayout = false;
      if((*iter)->Sequences.size() == 0)
        {
        for(i = 0; i < (*iter)->Series->getNumberOfSequences(); i++)
          {
          pqLineChartSeries::SequenceType type =
              (*iter)->Series->getSequenceType(i);
          if(type == pqLineChartSeries::Point)
            {
            (*iter)->Sequences.append(new pqLineChartSeriesPointData());
            }
          else if(type == pqLineChartSeries::Line)
            {
            (*iter)->Sequences.append(new pqLineChartSeriesLineData());
            }
          else if(type == pqLineChartSeries::Error)
            {
            (*iter)->Sequences.append(new pqLineChartSeriesErrorData());
            }
          else
            {
            qWarning("Unknown series sequence type.");
            break;
            }
          }
        }

      if((*iter)->Sequences.size() != (*iter)->Series->getNumberOfSequences())
        {
        qWarning("Series layout data invalid.");
        continue;
        }

      QList<pqLineChartSeriesItemData *>::Iterator sequence =
          (*iter)->Sequences.begin();
      for(i = 0; sequence != (*iter)->Sequences.end(); ++sequence, ++i)
        {
        // Clear out the previous layout data.
        (*sequence)->clear();
        pqLineChartSeriesPointData *points =
            dynamic_cast<pqLineChartSeriesPointData *>(*sequence);
        pqLineChartSeriesLineData *line =
            dynamic_cast<pqLineChartSeriesLineData *>(*sequence);
        pqLineChartSeriesErrorData *error =
            dynamic_cast<pqLineChartSeriesErrorData *>(*sequence);

        QPolygon *polygon = 0;
        int total = (*iter)->Series->getNumberOfPoints(i);
        for(int j = 0; j < total; j++)
          {
          // Get the point coordinate from the series.
          pqChartCoordinate coord;
          (*iter)->Series->getPoint(i, j, coord);

          // Use the axis scale to translate the series' coordinates
          // into chart coordinates.
          QPoint point(xScale->getPixelFor(coord.X),
              yScale->getPixelFor(coord.Y));

          // Store the pixel coordinates for the point.
          if(line)
            {
            // Qt has a bug drawing polylines with large data sets, so
            // break them up into manageable chunks.
            if(!polygon || j % 100 == 0)
              {
              line->Sequence.append(QPolygon());
              polygon = &line->Sequence.last();
              polygon->reserve(101);

              // Copy the last point from the previous polygon to the
              // new one.
              if(line->Sequence.size() > 1)
                {
                polygon->append(
                    line->Sequence[line->Sequence.size() - 2].last());
                }
              }

            polygon->append(point);
            }
          else if(points)
            {
            points->Sequence.append(point);
            }
          else if(error)
            {
            // If this is the first point, calculate the error width
            // in pixels using the point and the error width.
            if(j == 0)
              {
              // Get the error width for the sequence.
              pqChartValue width;
              (*iter)->Series->getErrorWidth(i, width);
              error->Width = xScale->getPixelFor(coord.X + width);
              error->Width -= point.x();
              }

            pqLineChartSeriesErrorDataItem errorData;
            errorData.X = point.x();

            // Get the error bounds for the point.
            pqChartValue upper, lower;
            (*iter)->Series->getErrorBounds(i, j, upper, lower);
            errorData.Upper = yScale->getPixelFor(upper);
            errorData.Lower = yScale->getPixelFor(lower);

            error->Sequence.append(errorData);
            }
          }
        }
      }
    }

  this->NeedsLayout = false;
}

void pqLineChart::drawChart(QPainter &painter, const QRect &area)
{
  if(!painter.isActive() || !area.isValid() ||
      !this->Internal->Bounds.isValid())
    {
    return;
    }

  // Set up the clip area for the line and error sequences.
  QRect clipArea = area.intersect(this->Internal->Bounds);

  // Translate the painter and the area to paint to contents space.
  painter.save();
  QRect toPaint = area;
  const pqChartContentsSpace *zoomPan = this->getContentsSpace();
  if(zoomPan)
    {
    painter.translate(-zoomPan->getXOffset(), -zoomPan->getYOffset());
    toPaint.translate(zoomPan->getXOffset(), zoomPan->getYOffset());
    clipArea.translate(zoomPan->getXOffset(), zoomPan->getYOffset());
    }

  // Set up the painter.
  painter.setRenderHint(QPainter::Antialiasing, true);

  // Draw each of the series.
  QList<pqLineChartSeriesItem *>::Iterator iter =
      this->Internal->Series.begin();
  for(int i = 0; iter != this->Internal->Series.end(); ++iter, ++i)
    {
    // Get the options for the series.
    pqLineChartSeriesOptions *options = this->Options->getSeriesOptions(i);
    QList<pqLineChartSeriesItemData *>::Iterator sequence =
        (*iter)->Sequences.begin();
    for(int j = 0; sequence != (*iter)->Sequences.end(); ++sequence, ++j)
      {
      // Set up the painter options for the sequence.
      if(options)
        {
        options->setupPainter(painter, j);
        }
      else
        {
        QPen defaultPen;
        this->Options->getGenerator()->getSeriesPen(i, defaultPen);
        painter.setPen(defaultPen);
        }

      pqLineChartSeriesPointData *points =
          dynamic_cast<pqLineChartSeriesPointData *>(*sequence);
      pqLineChartSeriesLineData *line =
          dynamic_cast<pqLineChartSeriesLineData *>(*sequence);
      pqLineChartSeriesErrorData *error =
          dynamic_cast<pqLineChartSeriesErrorData *>(*sequence);
      if(points)
        {
        // Use the marker to draw the points.
        pqPointMarker *marker = options ? options->getMarker(j) : 0;
        if(!marker)
          {
          // Use a default marker.
          marker = &this->Internal->Marker;
          }

        // Translate the painter for each point.
        QPolygon::Iterator jter = points->Sequence.begin();
        for( ; jter != points->Sequence.end(); ++jter)
          {
          // Make sure the point is in the clip area.
          if(clipArea.contains(*jter))
            {
            painter.save();
            painter.translate(*jter);
            marker->drawMarker(painter);
            painter.restore();
            }
          }
        }
      else
        {
        // Clip the drawing area for the line and error sequences.
        painter.save();
        painter.setClipping(true);
        painter.setClipRect(clipArea);
        if(line)
          {
          // Draw each polyline segment of the line. The line is broken
          // up to avoid a Qt bug when the polyline is large.
          QList<QPolygon>::Iterator jter = line->Sequence.begin();
          for( ; jter != line->Sequence.end(); ++jter)
            {
            painter.drawPolyline(*jter);
            }
          }
        else if(error)
          {
          // Get the error width from the options.
          QVector<pqLineChartSeriesErrorDataItem>::Iterator jter =
              error->Sequence.begin();
          for( ; jter != error->Sequence.end(); ++jter)
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

        painter.restore();
        }
      }
    }

  painter.restore();
}

void pqLineChart::handleModelReset()
{
  // Clean up the previous layout data.
  this->clearSeriesList();

  // Build a list of the current data.
  this->buildSeriesList();

  // Set up the axis ranges and update the chart layout.
  emit this->rangeChanged();
  emit this->layoutNeeded();
}

void pqLineChart::startSeriesInsertion(int, int)
{
  // TODO: Add selection support.
}

void pqLineChart::finishSeriesInsertion(int first, int last)
{
  // Add new chart items for the new series.
  for(int i = first; i <= last; i++)
    {
    const pqLineChartSeries *series = this->Model->getSeries(i);
    this->Internal->Series.insert(i, new pqLineChartSeriesItem(series));
    }

  emit this->layoutNeeded();
}

void pqLineChart::startSeriesRemoval(int first, int last)
{
  // Remove the internal items from the list.
  for(int i = last; i >= first; i--)
    {
    delete this->Internal->Series.takeAt(i);
    }

  // TODO: Add selection support.
}

void pqLineChart::finishSeriesRemoval(int, int)
{
  // The series removal may modify the ranges.
  if(this->NeedsLayout)
    {
    emit this->layoutNeeded();
    }
  else
    {
    emit this->repaintNeeded();
    }
}

void pqLineChart::handleSeriesMoved(int current, int index)
{
  // Moving the series only affects the painting layer. Move the
  // corresponding chart item and signal a repaint.
  pqLineChartSeriesItem *item = this->Internal->Series.takeAt(current);
  this->Internal->Series.insert(index, item);
  emit this->repaintNeeded();
}

void pqLineChart::handleSeriesReset(const pqLineChartSeries *series)
{
  // Find the given series' layout item.
  pqLineChartSeriesItem *item = this->getItem(series);
  if(item)
    {
    QList<pqLineChartSeriesItemData *>::Iterator sequence =
        item->Sequences.begin();
    for( ; sequence != item->Sequences.end(); ++sequence)
      {
      delete *sequence;
      }

    item->Sequences.clear();
    item->NeedsLayout = true;
    emit this->layoutNeeded();
    }
}

void pqLineChart::startPointInsertion(const pqLineChartSeries *, int, int, int)
{
  // TODO: Add selection support.
}

void pqLineChart::finishPointInsertion(const pqLineChartSeries *series, int)
{
  // If this is a multi-sequence change, wait for the completion.
  if(!this->Internal->MultiSequences.contains(series))
    {
    // Mark the series for re-layout.
    pqLineChartSeriesItem *item = this->getItem(series);
    item->NeedsLayout = true;
    emit this->layoutNeeded();
    }
}

void pqLineChart::startPointRemoval(const pqLineChartSeries *, int, int, int)
{
  // TODO: Add selection support.
}

void pqLineChart::finishPointRemoval(const pqLineChartSeries *series, int)
{
  // If this is a multi-sequence change, wait for the completion.
  if(!this->Internal->MultiSequences.contains(series))
    {
    // Mark the series for re-layout.
    pqLineChartSeriesItem *item = this->getItem(series);
    item->NeedsLayout = true;
    emit this->layoutNeeded();
    }
}

void pqLineChart::startMultiSeriesChange(const pqLineChartSeries *series)
{
  // Keep track of the series so all the changes can be made before
  // updating the layout.
  this->Internal->MultiSequences.append(series);
}

void pqLineChart::finishMultiSeriesChange(const pqLineChartSeries *series)
{
  // Update the axis ranges and the layout for the changes.
  this->Internal->MultiSequences.removeAll(series);
  pqLineChartSeriesItem *item = this->getItem(series);
  item->NeedsLayout = true;
  emit this->layoutNeeded();
}

void pqLineChart::handleSeriesErrorBoundsChanged(
    const pqLineChartSeries *series, int, int, int)
{
  pqLineChartSeriesItem *item = this->getItem(series);
  item->NeedsLayout = true;
  emit this->layoutNeeded();
}

void pqLineChart::handleSeriesErrorWidthChanged(
    const pqLineChartSeries *series, int)
{
  pqLineChartSeriesItem *item = this->getItem(series);
  item->NeedsLayout = true;
  emit this->layoutNeeded();
}

void pqLineChart::handleRangeChange()
{
  this->NeedsLayout = true;
}

void pqLineChart::handleSeriesOptionsChanged()
{
  emit this->repaintNeeded();
}

void pqLineChart::clearSeriesList()
{
  // Clean up the layout data.
  QList<pqLineChartSeriesItemData *>::Iterator sequence;
  QList<pqLineChartSeriesItem *>::Iterator series =
      this->Internal->Series.begin();
  for( ; series != this->Internal->Series.end(); ++series)
    {
    sequence = (*series)->Sequences.begin();
    for( ; sequence != (*series)->Sequences.end(); ++sequence)
      {
      delete *sequence;
      }

    delete *series;
    }

  this->Internal->Series.clear();
}

void pqLineChart::buildSeriesList()
{
  if(this->Model)
    {
    // Make a line chart item for each series in the model.
    for(int i = 0; i < this->Model->getNumberOfSeries(); i++)
      {
      this->Internal->Series.append(new pqLineChartSeriesItem(
          this->Model->getSeries(i)));
      }
    }
}

pqLineChartSeriesItem *pqLineChart::getItem(
    const pqLineChartSeries *series) const
{
  pqLineChartSeriesItem *item = 0;
  QList<pqLineChartSeriesItem *>::Iterator iter =
      this->Internal->Series.begin();
  for( ; iter != this->Internal->Series.end(); ++iter)
    {
    if((*iter)->Series == series)
      {
      item = *iter;
      break;
      }
    }

  return item;
}

/*void pqLineChart::showTooltip(QHelpEvent *e)
{
  int series_index = -1;
  double series_distance = VTK_DOUBLE_MAX;
  
  for(unsigned int i = 0; i != this->Implementation->size(); ++i)
    {
    const double distance = this->Implementation->at(i)->getDistance(e->pos());
    if(distance < series_distance)
      {
      series_index = i;
      series_distance = distance;
      }
    }

  if(series_index == -1 || series_distance > 10)
    return;
    
  this->Implementation->at(series_index)->showChartTip(e);
}*/


