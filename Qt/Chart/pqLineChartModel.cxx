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
#include "pqLineChartSeries.h"

#include <QList>
#include <QVector>


class pqLineChartModelInternal
{
public:
  pqLineChartModelInternal();
  ~pqLineChartModelInternal() {}

  QList<pqLineChartSeries *> List;
  QList<pqLineChartSeries *> MultiSeries;
  pqChartCoordinate Minimum;
  pqChartCoordinate Maximum;
};


//----------------------------------------------------------------------------
pqLineChartModelInternal::pqLineChartModelInternal()
  : List(), MultiSeries(), Minimum(), Maximum()
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

int pqLineChartModel::getNumberOfSeries() const
{
  return this->Internal->List.size();
}

int pqLineChartModel::getIndexOf(pqLineChartSeries *series) const
{
  return this->Internal->List.indexOf(series);
}

pqLineChartSeries *pqLineChartModel::getSeries(int index) const
{
  if(index >= 0 && index < this->Internal->List.size())
    {
    return this->Internal->List.at(index);
    }

  return 0;
}

void pqLineChartModel::appendSeries(pqLineChartSeries *series)
{
  this->insertSeries(series, this->Internal->List.size());
}

void pqLineChartModel::insertSeries(pqLineChartSeries *series, int index)
{
  // Make sure the series is not in the list.
  if(!series || this->Internal->List.indexOf(series) != -1)
    {
    return;
    }

  if(index < 0 || index > this->Internal->List.size())
    {
    index = this->Internal->List.size();
    }

  // Insert the new series. Listen to the series signals for rebroadcast.
  emit this->aboutToInsertSeries(index, index);
  this->Internal->List.insert(index, series);
  this->connect(series, SIGNAL(seriesReset()),
      this, SLOT(handleSeriesReset()));
  this->connect(series, SIGNAL(aboutToInsertPoints(int, int, int)),
      this, SLOT(handleSeriesBeginInsert(int, int, int)));
  this->connect(series, SIGNAL(pointsInserted(int)),
      this, SLOT(handleSeriesEndInsert(int)));
  this->connect(series, SIGNAL(aboutToRemovePoints(int, int, int)),
      this, SLOT(handleSeriesBeginRemove(int, int, int)));
  this->connect(series, SIGNAL(pointsRemoved(int)),
      this, SLOT(handleSeriesEndRemove(int)));
  this->connect(series, SIGNAL(aboutToChangeMultipleSequences()),
      this, SLOT(startSeriesMultiSequenceChange()));
  this->connect(series, SIGNAL(changedMultipleSequences()),
      this, SLOT(finishSeriesMultiSequenceChange()));

  // Update the axis ranges for the new series. If the series can fit
  // in the current layout, only the new series need to be layed out.
  this->updateChartRanges(series);
  emit this->seriesInserted(index, index);
}

void pqLineChartModel::removeSeries(pqLineChartSeries *series)
{
  if(series)
    {
    // Remove the series by index.
    this->removeSeries(this->Internal->List.indexOf(series));
    }
}

void pqLineChartModel::removeSeries(int index)
{
  if(index < 0 || index >= this->Internal->List.size())
    {
    return;
    }

  emit this->aboutToRemoveSeries(index, index);
  pqLineChartSeries *series = this->Internal->List.takeAt(index);
  this->disconnect(series, 0, this, 0);

  // Re-calculate the chart ranges.
  this->updateChartRanges();
  emit this->seriesRemoved(index, index);
}

void pqLineChartModel::moveSeries(pqLineChartSeries *series, int index)
{
  if(series)
    {
    // Move the series by index.
    this->moveSeries(this->Internal->List.indexOf(series), index);
    }
}

void pqLineChartModel::moveSeries(int current, int index)
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

  // Move the series to the new place in the list.
  pqLineChartSeries *series = this->Internal->List.takeAt(current);
  if(index < this->Internal->List.size())
    {
    this->Internal->List.insert(index, series);
    }
  else
    {
    this->Internal->List.append(series);
    }

  emit this->seriesMoved(current, index);
}

void pqLineChartModel::removeAll()
{
  QList<pqLineChartSeries *>::Iterator iter = this->Internal->List.begin();
  for( ; iter != this->Internal->List.end(); ++iter)
    {
    QObject::disconnect(*iter, 0, this, 0);
    }

  this->Internal->List.clear();
  this->updateChartRanges();
  emit this->modelReset();
}

void pqLineChartModel::getRangeX(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->Minimum.X;
  max = this->Internal->Maximum.X;
}

void pqLineChartModel::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->Minimum.Y;
  max = this->Internal->Maximum.Y;
}

void pqLineChartModel::handleSeriesReset()
{
  pqLineChartSeries *series = qobject_cast<pqLineChartSeries *>(this->sender());
  this->updateChartRanges();
  emit this->seriesReset(series);
}

void pqLineChartModel::handleSeriesBeginInsert(int sequence, int first, int last)
{
  pqLineChartSeries *series = qobject_cast<pqLineChartSeries *>(this->sender());
  emit this->aboutToInsertPoints(series, sequence, first, last);
}

void pqLineChartModel::handleSeriesEndInsert(int sequence)
{
  pqLineChartSeries *series = qobject_cast<pqLineChartSeries *>(this->sender());
  if(!this->Internal->MultiSeries.contains(series))
    {
    // Update the chart ranges for the new points.
    this->updateChartRanges(series);
    }

  emit this->pointsInserted(series, sequence);
}

void pqLineChartModel::handleSeriesBeginRemove(int sequence, int first, int last)
{
  pqLineChartSeries *series = qobject_cast<pqLineChartSeries *>(this->sender());
  emit this->aboutToRemovePoints(series, sequence, first, last);
}

void pqLineChartModel::handleSeriesEndRemove(int sequence)
{
  pqLineChartSeries *series = qobject_cast<pqLineChartSeries *>(this->sender());
  if(!this->Internal->MultiSeries.contains(series))
    {
    this->updateChartRanges();
    }

  emit this->pointsRemoved(series, sequence);
}

void pqLineChartModel::startSeriesMultiSequenceChange()
{
  pqLineChartSeries *series = qobject_cast<pqLineChartSeries *>(this->sender());
  if(series)
    {
    this->Internal->MultiSeries.append(series);
    emit this->aboutToChangeMultipleSeries(series);
    }
}

void pqLineChartModel::finishSeriesMultiSequenceChange()
{
  pqLineChartSeries *series = qobject_cast<pqLineChartSeries *>(this->sender());
  if(series && this->Internal->MultiSeries.contains(series))
    {
    // Re-calculate the chart ranges.
    this->updateChartRanges();
    this->Internal->MultiSeries.removeAll(series);
    emit this->changedMultipleSeries(series);
    }
}

void pqLineChartModel::handleSeriesErrorBoundsChange(int sequence, int first,
    int last)
{
  pqLineChartSeries *series = qobject_cast<pqLineChartSeries *>(this->sender());
  if(series)
    {
    this->updateChartRanges();
    emit this->errorBoundsChanged(series, sequence, first, last);
    }
}

void pqLineChartModel::handleSeriesErrorWidthChange(int sequence)
{
  pqLineChartSeries *series = qobject_cast<pqLineChartSeries *>(this->sender());
  if(series)
    {
    emit this->errorWidthChanged(series, sequence);
    }
}

void pqLineChartModel::updateChartRanges()
{
  // Find the extents of the series data.
  pqChartCoordinate min, max;
  QList<pqLineChartSeries *>::Iterator series = this->Internal->List.begin();
  if(series != this->Internal->List.end())
    {
    (*series)->getRangeX(min.X, max.X);
    (*series)->getRangeY(min.Y, max.Y);
    ++series;
    }

  pqChartValue seriesMin, seriesMax;
  for( ; series != this->Internal->List.end(); ++series)
    {
    (*series)->getRangeX(seriesMin, seriesMax);
    if(seriesMin < min.X)
      {
      min.X = seriesMin;
      }
    if(seriesMax > max.X)
      {
      max.X = seriesMax;
      }

    (*series)->getRangeY(seriesMin, seriesMax);
    if(seriesMin < min.Y)
      {
      min.Y = seriesMin;
      }
    if(seriesMax > max.Y)
      {
      max.Y = seriesMax;
      }
    }

  if(min.X != this->Internal->Minimum.X ||
      max.X != this->Internal->Maximum.X ||
      min.Y != this->Internal->Minimum.Y ||
      max.Y != this->Internal->Maximum.Y)
    {
    this->Internal->Minimum.X = min.X;
    this->Internal->Maximum.X = max.X;
    this->Internal->Minimum.Y = min.Y;
    this->Internal->Maximum.Y = max.Y;
    emit this->chartRangeChanged();
    }
}

void pqLineChartModel::updateChartRanges(const pqLineChartSeries *series)
{
  pqChartCoordinate min, max;
  series->getRangeX(min.X, max.X);
  series->getRangeY(min.Y, max.Y);
  if(this->getNumberOfSeries() > 1)
    {
    if(this->Internal->Minimum.X < min.X)
      {
      min.X = this->Internal->Minimum.X;
      }

    if(this->Internal->Maximum.X > max.X)
      {
      max.X = this->Internal->Maximum.X;
      }

    if(this->Internal->Minimum.Y < min.Y)
      {
      min.Y = this->Internal->Minimum.Y;
      }

    if(this->Internal->Maximum.Y > max.Y)
      {
      max.Y = this->Internal->Maximum.Y;
      }
    }

  if(min.X != this->Internal->Minimum.X ||
      max.X != this->Internal->Maximum.X ||
      min.Y != this->Internal->Minimum.Y ||
      max.Y != this->Internal->Maximum.Y)
    {
    this->Internal->Minimum.X = min.X;
    this->Internal->Maximum.X = max.X;
    this->Internal->Minimum.Y = min.Y;
    this->Internal->Maximum.Y = max.Y;
    emit this->chartRangeChanged();
    }
}


