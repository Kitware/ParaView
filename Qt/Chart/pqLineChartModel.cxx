/*=========================================================================

   Program: ParaView
   Module:    pqLineChartModel.cxx

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

/// \file pqLineChartModel.cxx
/// \date 9/8/2006

#include "pqLineChartModel.h"

#include "pqChartCoordinate.h"
#include "pqChartValue.h"
#include "pqLineChartPlot.h"
#include "pqLineChartPlotOptions.h"

#include <QList>
#include <QVector>


class pqLineChartModelInternal
{
public:
  pqLineChartModelInternal();
  ~pqLineChartModelInternal() {}

  QList<pqLineChartPlot *> List;
  QList<pqLineChartPlot *> MultiSeries;
  QVector<pqLineChartPlotOptions *> Options;
  pqChartValue MinimumX;
  pqChartValue MaximumX;
  pqChartValue MinimumY;
  pqChartValue MaximumY;
};


//----------------------------------------------------------------------------
pqLineChartModelInternal::pqLineChartModelInternal()
  : List(), MultiSeries(), Options(), MinimumX(), MaximumX(), MinimumY(),
    MaximumY()
{
}


//----------------------------------------------------------------------------
pqLineChartModel::pqLineChartModel(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqLineChartModelInternal();
}

pqLineChartModel::~pqLineChartModel()
{
  delete this->Internal;
}

int pqLineChartModel::getNumberOfPlots() const
{
  return this->Internal->List.size();
}

int pqLineChartModel::getIndexOf(pqLineChartPlot *plot) const
{
  return this->Internal->List.indexOf(plot);
}

pqLineChartPlot *pqLineChartModel::getPlot(int index) const
{
  if(index >= 0 && index < this->Internal->List.size())
    {
    return this->Internal->List.at(index);
    }

  return 0;
}

void pqLineChartModel::appendPlot(pqLineChartPlot *plot)
{
  this->insertPlot(plot, this->Internal->List.size());
}

void pqLineChartModel::insertPlot(pqLineChartPlot *plot, int index)
{
  // Make sure the plot is not in the list.
  if(!plot || this->Internal->List.indexOf(plot) != -1)
    {
    return;
    }

  if(index < 0 || index > this->Internal->List.size())
    {
    index = this->Internal->List.size();
    }

  // Insert the new plot. Listen to the plot signals for rebroadcast.
  emit this->aboutToInsertPlots(index, index);
  this->Internal->List.insert(index, plot);
  this->connect(plot, SIGNAL(plotReset()), this, SLOT(handlePlotReset()));
  this->connect(plot, SIGNAL(aboutToInsertPoints(int, int, int)),
      this, SLOT(handlePlotBeginInsert(int, int, int)));
  this->connect(plot, SIGNAL(pointsInserted(int)),
      this, SLOT(handlePlotEndInsert(int)));
  this->connect(plot, SIGNAL(aboutToRemovePoints(int, int, int)),
      this, SLOT(handlePlotBeginRemove(int, int, int)));
  this->connect(plot, SIGNAL(pointsRemoved(int)),
      this, SLOT(handlePlotEndRemove(int)));
  this->connect(plot, SIGNAL(aboutToChangeMultipleSeries()),
      this, SLOT(handlePlotBeginMultiSeriesChange()));
  this->connect(plot, SIGNAL(changedMultipleSeries()),
      this, SLOT(handlePlotEndMultiSeriesChange()));

  // Update the chart ranges for the new plot.
  this->updateChartRanges(plot);
  emit this->plotsInserted(index, index);
}

void pqLineChartModel::removePlot(pqLineChartPlot *plot)
{
  if(plot)
    {
    // Remove the plot by index.
    this->removePlot(this->Internal->List.indexOf(plot));
    }
}

void pqLineChartModel::removePlot(int index)
{
  if(index < 0 || index >= this->Internal->List.size())
    {
    return;
    }

  emit this->aboutToRemovePlots(index, index);
  pqLineChartPlot *plot = this->Internal->List.takeAt(index);
  this->disconnect(plot, 0, this, 0);

  // Re-calculate the chart ranges.
  this->updateChartRanges();
  emit this->plotsRemoved(index, index);
}

void pqLineChartModel::movePlot(pqLineChartPlot *plot, int index)
{
  if(plot)
    {
    // Move the plot by index.
    this->movePlot(this->Internal->List.indexOf(plot), index);
    }
}

void pqLineChartModel::movePlot(int current, int index)
{
  if(current < 0 || current >= this->Internal->List.size() ||
      index < 0 || index >= this->Internal->List.size())
    {
    return;
    }

  // Adjust the index if it is after the current one.
  if(index > current)
    {
    index--;
    }

  // Move the plot to the new place in the list.
  pqLineChartPlot *plot = this->Internal->List.takeAt(current);
  if(index < this->Internal->List.size())
    {
    this->Internal->List.insert(index, plot);
    }
  else
    {
    this->Internal->List.append(plot);
    }

  emit this->plotMoved(current, index);
}

void pqLineChartModel::movePlotAndOptions(int current, int index)
{
  if(current < 0 || current >= this->Internal->List.size() ||
      index < 0 || index >= this->Internal->List.size())
    {
    return;
    }

  // Check if the options have been set for the current location.
  pqLineChartPlotOptions *options = 0;
  if(current >= 0 && current < this->Internal->Options.size())
    {
    // Remove the options from the list.
    options = this->Internal->Options[current];
    this->Internal->Options.remove(current);
    }

  // Adjust the index if it is after the current one.
  if(index > current)
    {
    index--;
    }

  // Move the plot options to the new place.
  if(index < this->Internal->Options.size())
    {
    this->Internal->Options.insert(index, options);
    }
  else
    {
    this->blockSignals(true);
    this->setOptions(index, options);
    this->blockSignals(false);
    }

  this->movePlot(current, index);
}

void pqLineChartModel::clearPlots()
{
  QList<pqLineChartPlot *>::Iterator iter = this->Internal->List.begin();
  for( ; iter != this->Internal->List.end(); ++iter)
    {
    QObject::disconnect(*iter, 0, this, 0);
    }

  this->Internal->List.clear();
  this->updateChartRanges();
  emit this->plotsReset();
}

void pqLineChartModel::getRangeX(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->MinimumX;
  max = this->Internal->MaximumX;
}

void pqLineChartModel::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->MinimumY;
  max = this->Internal->MaximumY;
}

pqLineChartPlotOptions *pqLineChartModel::getOptions(int index) const
{
  if(index >= 0 && index < this->Internal->Options.size())
    {
    return this->Internal->Options[index];
    }

  return 0;
}

void pqLineChartModel::setOptions(int index, pqLineChartPlotOptions *options)
{
  if(index < 0)
    {
    return;
    }

  // Expand the vector to fit the plot index if needed.
  if(index >= this->Internal->Options.size())
    {
    int i = this->Internal->Options.size();
    this->Internal->Options.resize(index + 1);
    for( ; i < this->Internal->Options.size(); i++)
      {
      this->Internal->Options[i] = 0;
      }
    }

  if(this->Internal->Options[index])
    {
    QObject::disconnect(this->Internal->Options[index], 0, this, 0);
    }

  this->Internal->Options[index] = options;
  if(options)
    {
    // Forward the options changed signal from the object.
    this->connect(options, SIGNAL(optionsChanged()),
        this, SIGNAL(optionsChanged()));
    }

  if(index < this->Internal->List.size())
    {
    emit this->optionsChanged();
    }
}

void pqLineChartModel::clearOptions()
{
  QVector<pqLineChartPlotOptions *>::Iterator iter =
      this->Internal->Options.begin();
  for( ; iter != this->Internal->Options.end(); ++iter)
    {
    QObject::disconnect(*iter, 0, this, 0);
    }

  bool hadOptions = this->Internal->Options.size() > 0;
  this->Internal->Options.clear();
  if(hadOptions && this->Internal->List.size() > 0)
    {
    emit this->optionsChanged();
    }
}

void pqLineChartModel::handlePlotReset()
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
  this->updateChartRanges();
  emit this->plotReset(plot);
}

void pqLineChartModel::handlePlotBeginInsert(int series, int first, int last)
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
  emit this->aboutToInsertPoints(plot, series, first, last);
}

void pqLineChartModel::handlePlotEndInsert(int series)
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
  if(!this->Internal->MultiSeries.contains(plot))
    {
    // Update the chart ranges for the new points.
    this->updateChartRanges(plot);
    }

  emit this->pointsInserted(plot, series);
}

void pqLineChartModel::handlePlotBeginRemove(int series, int first, int last)
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
  emit this->aboutToRemovePoints(plot, series, first, last);
}

void pqLineChartModel::handlePlotEndRemove(int series)
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
  if(!this->Internal->MultiSeries.contains(plot))
    {
    this->updateChartRanges();
    }

  emit this->pointsRemoved(plot, series);
}

void pqLineChartModel::handlePlotBeginMultiSeriesChange()
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
  if(plot)
    {
    this->Internal->MultiSeries.append(plot);
    emit this->aboutToChangeMultipleSeries(plot);
    }
}

void pqLineChartModel::handlePlotEndMultiSeriesChange()
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
  if(plot && this->Internal->MultiSeries.contains(plot))
    {
    // Re-calculate the chart ranges.
    this->updateChartRanges();
    this->Internal->MultiSeries.removeAll(plot);
    emit this->changedMultipleSeries(plot);
    }
}

void pqLineChartModel::handlePlotErrorBoundsChange(int series, int first,
    int last)
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
  if(plot)
    {
    this->updateChartRanges();
    emit this->errorBoundsChanged(plot, series, first, last);
    }
}

void pqLineChartModel::handlePlotErrorWidthChange(int series)
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
  if(plot)
    {
    emit this->errorWidthChanged(plot, series);
    }
}

void pqLineChartModel::updateChartRanges()
{
  // Reset the current chart ranges.
  this->Internal->MinimumX = (int)0;
  this->Internal->MaximumX = (int)0;
  this->Internal->MinimumY = (int)0;
  this->Internal->MaximumY = (int)0;

  // Find the extents of the plot data.
  QList<pqLineChartPlot *>::Iterator plot = this->Internal->List.begin();
  if(plot != this->Internal->List.end())
    {
    (*plot)->getRangeX(this->Internal->MinimumX, this->Internal->MaximumX);
    (*plot)->getRangeY(this->Internal->MinimumY, this->Internal->MaximumY);
    ++plot;
    }

  pqChartValue min, max;
  for( ; plot != this->Internal->List.end(); ++plot)
    {
    (*plot)->getRangeX(min, max);
    if(min < this->Internal->MinimumX)
      {
      this->Internal->MinimumX = min;
      }
    if(max > this->Internal->MaximumX)
      {
      this->Internal->MaximumX = max;
      }

    (*plot)->getRangeY(min, max);
    if(min < this->Internal->MinimumY)
      {
      this->Internal->MinimumY = min;
      }
    if(max > this->Internal->MaximumY)
      {
      this->Internal->MaximumY = max;
      }
    }
}

void pqLineChartModel::updateChartRanges(const pqLineChartPlot *plot)
{
  pqChartCoordinate min, max;
  plot->getRangeX(min.X, max.X);
  plot->getRangeY(min.Y, max.Y);
  if(this->getNumberOfPlots() == 1)
    {
    this->Internal->MinimumX = min.X;
    this->Internal->MaximumX = max.X;
    this->Internal->MinimumY = min.Y;
    this->Internal->MaximumY = max.Y;
    }
  else
    {
    if(min.X < this->Internal->MinimumX)
      {
      this->Internal->MinimumX = min.X;
      }
    if(max.X > this->Internal->MaximumX)
      {
      this->Internal->MaximumX = max.X;
      }

    if(min.Y < this->Internal->MinimumY)
      {
      this->Internal->MinimumY = min.Y;
      }
    if(max.Y > this->Internal->MaximumY)
      {
      this->Internal->MaximumY = max.Y;
      }
    }
}


