/*=========================================================================

   Program: ParaView
   Module:    pqSimpleLineChartPlot.cxx

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

/// \file pqSimpleLineChartPlot.cxx
/// \date 8/22/2005

#include "pqSimpleLineChartPlot.h"

#include "pqChartCoordinate.h"
#include "pqChartValue.h"

#include <QHelpEvent>
#include <QList>
#include <QToolTip>
#include <QVector>

class pqSimpleLineChartPlotErrorBounds
{
public:
  pqSimpleLineChartPlotErrorBounds();
  ~pqSimpleLineChartPlotErrorBounds() {}

  pqChartValue Upper;
  pqChartValue Lower;
};


class pqSimpleLineChartPlotErrorData
{
public:
  pqSimpleLineChartPlotErrorData();
  ~pqSimpleLineChartPlotErrorData() {}

  QVector<pqSimpleLineChartPlotErrorBounds> Bounds;
  pqChartValue Width;
};


class pqSimpleLineChartPlotSeries
{
public:
  pqSimpleLineChartPlotSeries(pqLineChartPlot::SeriesType type);
  ~pqSimpleLineChartPlotSeries();

  QVector<pqChartCoordinate> Points;
  pqLineChartPlot::SeriesType Type;
  pqSimpleLineChartPlotErrorData *Error;
};


class pqSimpleLineChartPlotInternal
{
public:
  pqSimpleLineChartPlotInternal();
  ~pqSimpleLineChartPlotInternal() {}

  QList<pqSimpleLineChartPlotSeries *> Series;
  pqChartCoordinate Minimum;
  pqChartCoordinate Maximum;
};


//----------------------------------------------------------------------------
pqSimpleLineChartPlotErrorBounds::pqSimpleLineChartPlotErrorBounds()
  : Upper(), Lower()
{
}


//----------------------------------------------------------------------------
pqSimpleLineChartPlotErrorData::pqSimpleLineChartPlotErrorData()
  : Bounds(), Width()
{
}


//----------------------------------------------------------------------------
pqSimpleLineChartPlotSeries::pqSimpleLineChartPlotSeries(
    pqLineChartPlot::SeriesType type)
  : Points()
{
  this->Type = type;
  this->Error = 0;
  if(this->Type == pqLineChartPlot::Error)
    {
    this->Error = new pqSimpleLineChartPlotErrorData();
    }
}

pqSimpleLineChartPlotSeries::~pqSimpleLineChartPlotSeries()
{
  if(this->Error)
    {
    delete this->Error;
    }
}


//----------------------------------------------------------------------------
pqSimpleLineChartPlotInternal::pqSimpleLineChartPlotInternal()
  : Series(), Minimum(), Maximum()
{
}


//----------------------------------------------------------------------------
pqSimpleLineChartPlot::pqSimpleLineChartPlot(QObject *parentObject)
  : pqLineChartPlot(parentObject)
{
  this->Internal = new pqSimpleLineChartPlotInternal();
}

pqSimpleLineChartPlot::~pqSimpleLineChartPlot()
{
  QList<pqSimpleLineChartPlotSeries *>::Iterator iter =
      this->Internal->Series.begin();
  for( ; iter != this->Internal->Series.end(); ++iter)
    {
    delete *iter;
    }

  delete this->Internal;
}

int pqSimpleLineChartPlot::getNumberOfSeries() const
{
  return this->Internal->Series.size();
}

int pqSimpleLineChartPlot::getTotalNumberOfPoints() const
{
  int total = 0;
  QList<pqSimpleLineChartPlotSeries *>::Iterator series =
      this->Internal->Series.begin();
  for( ; series != this->Internal->Series.end(); ++series)
    {
    total += (*series)->Points.size();
    }

  return total;
}

pqLineChartPlot::SeriesType pqSimpleLineChartPlot::getSeriesType(
    int series) const
{
  if(series >= 0 && series < this->getNumberOfSeries())
    {
    return this->Internal->Series[series]->Type;
    }

  return pqLineChartPlot::Invalid;
}

int pqSimpleLineChartPlot::getNumberOfPoints(int series) const
{
  if(series >= 0 && series < this->getNumberOfSeries())
    {
    return this->Internal->Series[series]->Points.size();
    }

  return 0;
}

void pqSimpleLineChartPlot::getPoint(int series, int index,
    pqChartCoordinate &coord) const
{
  if(index >= 0 && index < this->getNumberOfPoints(series))
    {
    coord = this->Internal->Series[series]->Points[index];
    }
}

void pqSimpleLineChartPlot::getErrorBounds(int series, int index,
    pqChartValue &upper, pqChartValue &lower) const
{
  if(this->getSeriesType(series) == pqLineChartPlot::Error)
    {
    pqSimpleLineChartPlotSeries *plotSeries = this->Internal->Series[series];
    if(plotSeries->Error && index >= 0 &&
        index < plotSeries->Error->Bounds.size())
      {
      upper = plotSeries->Error->Bounds[index].Upper;
      lower = plotSeries->Error->Bounds[index].Lower;
      }
    }
}

void pqSimpleLineChartPlot::getErrorWidth(int series,
    pqChartValue &width) const
{
  if(this->getSeriesType(series) == pqLineChartPlot::Error)
    {
    pqSimpleLineChartPlotSeries *plotSeries = this->Internal->Series[series];
    if(plotSeries->Error)
      {
      width = plotSeries->Error->Width;
      }
    }
}

void pqSimpleLineChartPlot::getRangeX(pqChartValue &min,
    pqChartValue &max) const
{
  min = this->Internal->Minimum.X;
  max = this->Internal->Maximum.X;
}

void pqSimpleLineChartPlot::getRangeY(pqChartValue &min,
    pqChartValue &max) const
{
  min = this->Internal->Minimum.Y;
  max = this->Internal->Maximum.Y;
}

void pqSimpleLineChartPlot::clearPlot()
{
  if(this->Internal->Series.size() > 0)
    {
    QList<pqSimpleLineChartPlotSeries *>::Iterator iter =
        this->Internal->Series.begin();
    for( ; iter != this->Internal->Series.end(); ++iter)
      {
      delete *iter;
      }

    this->Internal->Series.clear();
    this->updatePlotRanges();
    this->resetPlot();
    }
}

void pqSimpleLineChartPlot::addSeries(pqLineChartPlot::SeriesType type)
{
  this->Internal->Series.append(new pqSimpleLineChartPlotSeries(type));
  this->resetPlot();
}

void pqSimpleLineChartPlot::insertSeries(int index,
    pqLineChartPlot::SeriesType type)
{
  if(index >= 0 && index < this->getNumberOfSeries())
    {
    this->Internal->Series.insert(index,
        new pqSimpleLineChartPlotSeries(type));
    this->resetPlot();
    }
}

void pqSimpleLineChartPlot::setSeriesType(int series,
    pqLineChartPlot::SeriesType type)
{
  if(series >= 0 && series < this->getNumberOfSeries())
    {
    pqSimpleLineChartPlotSeries *plotSeries = this->Internal->Series[series];
    if(plotSeries->Type != type)
      {
      if(plotSeries->Error)
        {
        delete plotSeries->Error;
        plotSeries->Error = 0;
        }

      plotSeries->Type = type;
      if(plotSeries->Type == pqLineChartPlot::Error)
        {
        plotSeries->Error = new pqSimpleLineChartPlotErrorData();
        plotSeries->Error->Bounds.resize(plotSeries->Points.size());
        }

      this->resetPlot();
      }
    }
}

void pqSimpleLineChartPlot::removeSeries(int series)
{
  if(series >= 0 && series < this->getNumberOfSeries())
    {
    delete this->Internal->Series.takeAt(series);
    this->updatePlotRanges();
    this->resetPlot();
    }
}

void pqSimpleLineChartPlot::copySeriesPoints(int source, int destination)
{
  if(source >= 0 && source < this->getNumberOfSeries() &&
      destination >= 0 && destination < this->getNumberOfSeries())
    {
    // If the destination already has points, notify the view that
    // they will be removed.
    this->clearPoints(destination);

    // Copy the source points to the destination. Notify the view
    // that points are being added to the destination.
    if(this->getNumberOfPoints(source) > 0)
      {
      int last = this->getNumberOfPoints(source) - 1;
      this->beginInsertPoints(destination, 0, last);
      pqSimpleLineChartPlotSeries *destPlot =
          this->Internal->Series[destination];
      destPlot->Points = this->Internal->Series[source]->Points;
      if(destPlot->Error)
        {
        destPlot->Error->Bounds.resize(destPlot->Points.size());
        }

      this->endInsertPoints(destination);
      }
    }
}

void pqSimpleLineChartPlot::addPoint(int series,
    const pqChartCoordinate &coord)
{
  if(series >= 0 && series < this->getNumberOfSeries())
    {
    int total = this->getNumberOfPoints(series);
    this->beginInsertPoints(series, total, total);
    pqSimpleLineChartPlotSeries *plotSeries = this->Internal->Series[series];
    plotSeries->Points.append(coord);
    if(plotSeries->Error)
      {
      plotSeries->Error->Bounds.resize(plotSeries->Points.size());
      }

    this->updatePlotRanges(coord);
    this->endInsertPoints(series);
    }
}

void pqSimpleLineChartPlot::insertPoint(int series, int index,
    const pqChartCoordinate &coord)
{
  if(index >= 0 && index < this->getNumberOfPoints(series))
    {
    this->beginInsertPoints(series, index, index);
    pqSimpleLineChartPlotSeries *plotSeries = this->Internal->Series[series];
    plotSeries->Points.insert(index, coord);
    if(plotSeries->Error && index < plotSeries->Error->Bounds.size())
      {
      plotSeries->Error->Bounds.insert(index,
          pqSimpleLineChartPlotErrorBounds());
      }

    this->updatePlotRanges(coord);
    this->endInsertPoints(series);
    }
}

void pqSimpleLineChartPlot::removePoint(int series, int index)
{
  if(index >= 0 && index < this->getNumberOfPoints(series))
    {
    this->beginRemovePoints(series, index, index);
    pqSimpleLineChartPlotSeries *plotSeries = this->Internal->Series[series];
    plotSeries->Points.remove(index);
    if(plotSeries->Error && index < plotSeries->Error->Bounds.size())
      {
      plotSeries->Error->Bounds.remove(index);
      }

    this->updatePlotRanges();
    this->endRemovePoints(series);
    }
}

void pqSimpleLineChartPlot::clearPoints(int series)
{
  if(series >= 0 && series < this->getNumberOfSeries() &&
      this->Internal->Series[series]->Points.size() > 0)
    {
    // Notify the view that the points are being removed. Then, clear
    // the point series.
    pqSimpleLineChartPlotSeries *plotSeries = this->Internal->Series[series];
    int last = plotSeries->Points.size() - 1;
    this->beginRemovePoints(series, 0, last);
    plotSeries->Points.clear();
    if(plotSeries->Error)
      {
      plotSeries->Error->Bounds.clear();
      }

    this->updatePlotRanges();
    this->endRemovePoints(series);
    }
}

void pqSimpleLineChartPlot::setErrorBounds(int series, int index,
    const pqChartValue &upper, const pqChartValue &lower)
{
  if(this->getSeriesType(series) == pqLineChartPlot::Error)
    {
    pqSimpleLineChartPlotSeries *plotSeries = this->Internal->Series[series];
    if(plotSeries->Error && index >= 0 &&
        index < plotSeries->Error->Bounds.size())
      {
      plotSeries->Error->Bounds[index].Upper = upper;
      plotSeries->Error->Bounds[index].Lower = lower;

      // Adjust the y-axis range for the new error bounds.
      if(lower < this->Internal->Minimum.Y)
        {
        this->Internal->Minimum.Y = lower;
        }
      if(upper > this->Internal->Maximum.Y)
        {
        this->Internal->Maximum.Y = upper;
        }

      emit this->errorBoundsChanged(series, index, index);
      }
    }
}

void pqSimpleLineChartPlot::setErrorWidth(int series,
    const pqChartValue &width)
{
  if(this->getSeriesType(series) == pqLineChartPlot::Error)
    {
    pqSimpleLineChartPlotSeries *plotSeries = this->Internal->Series[series];
    if(plotSeries->Error)
      {
      plotSeries->Error->Width = width;
      emit this->errorWidthChanged(series);
      }
    }
}

void pqSimpleLineChartPlot::updatePlotRanges()
{
  this->Internal->Minimum.X = (int)0;
  this->Internal->Minimum.Y = (int)0;
  this->Internal->Maximum.X = (int)0;
  this->Internal->Maximum.Y = (int)0;
  bool firstSet = false;
  QList<pqSimpleLineChartPlotSeries *>::Iterator series =
      this->Internal->Series.begin();
  for( ; series != this->Internal->Series.end(); ++series)
    {
    QVector<pqChartCoordinate>::Iterator point = (*series)->Points.begin();
    for( ; point != (*series)->Points.end(); ++point)
      {
      if(firstSet)
        {
        if(point->X < this->Internal->Minimum.X)
          {
          this->Internal->Minimum.X = point->X;
          }
        else if(point->X > this->Internal->Maximum.X)
          {
          this->Internal->Maximum.X = point->X;
          }

        if(point->Y < this->Internal->Minimum.Y)
          {
          this->Internal->Minimum.Y = point->Y;
          }
        else if(point->Y > this->Internal->Maximum.Y)
          {
          this->Internal->Maximum.Y = point->Y;
          }
        }
      else
        {
        firstSet = true;
        this->Internal->Minimum.X = point->X;
        this->Internal->Minimum.Y = point->Y;
        this->Internal->Maximum.X = point->X;
        this->Internal->Maximum.Y = point->Y;
        }
      }

    // Account for the error bounds in the y-axis range.
    if((*series)->Error)
      {
      QVector<pqSimpleLineChartPlotErrorBounds>::Iterator bounds =
          (*series)->Error->Bounds.begin();
      for( ; bounds != (*series)->Error->Bounds.end(); ++bounds)
        {
        // If the error bounds are equal, they are ignored.
        if(bounds->Upper != bounds->Lower)
          {
          if(bounds->Lower < this->Internal->Minimum.Y)
            {
            this->Internal->Minimum.Y = bounds->Lower;
            }
          if(bounds->Upper > this->Internal->Maximum.Y)
            {
            this->Internal->Maximum.Y = bounds->Upper;
            }
          }
        }
      }
    }
}

void pqSimpleLineChartPlot::updatePlotRanges(const pqChartCoordinate &coord)
{
  if(this->getTotalNumberOfPoints() == 1)
    {
    this->Internal->Minimum.X = coord.X;
    this->Internal->Minimum.Y = coord.Y;
    this->Internal->Maximum.X = coord.X;
    this->Internal->Maximum.Y = coord.Y;
    }
  else
    {
    if(coord.X < this->Internal->Minimum.X)
      {
      this->Internal->Minimum.X = coord.X;
      }
    else if(coord.X > this->Internal->Maximum.X)
      {
      this->Internal->Maximum.X = coord.X;
      }

    if(coord.Y < this->Internal->Minimum.Y)
      {
      this->Internal->Minimum.Y = coord.Y;
      }
    else if(coord.Y > this->Internal->Maximum.Y)
      {
      this->Internal->Maximum.Y = coord.Y;
      }
    }
}


/*const double pqLinePlot::getDistance(const QPoint& coords) const
{
  double distance = VTK_DOUBLE_MAX;
  for(int i = 0; i != this->Implementation->ScreenCoords.size(); ++i)
    distance = vtkstd::min(distance, static_cast<double>((this->Implementation->ScreenCoords[i] - coords).manhattanLength()));
  return distance;
}

void pqLinePlot::showChartTip(QHelpEvent& event) const
{
  double tip_distance = VTK_DOUBLE_MAX;
  pqChartCoordinate tip_coordinate;
  for(int i = 0; i != this->Implementation->ScreenCoords.size(); ++i)
    {
    const double distance = (this->Implementation->ScreenCoords[i] - event.pos()).manhattanLength();
    if(distance < tip_distance)
      {
      tip_distance = distance;
      tip_coordinate = this->Implementation->WorldCoords[i];
      }
    }

  if(tip_distance < VTK_DOUBLE_MAX)    
    QToolTip::showText(event.globalPos(), QString("%1, %2").arg(tip_coordinate.X.getDoubleValue()).arg(tip_coordinate.Y.getDoubleValue()));
}*/
