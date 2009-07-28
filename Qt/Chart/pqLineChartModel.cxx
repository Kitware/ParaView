/*=========================================================================

   Program: ParaView
   Module:    pqLineChartModel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

#include "pqChartAxis.h"
#include "pqChartCoordinate.h"
#include "pqChartValue.h"
#include "pqLineChartSeries.h"

#include <QList>
#include <QVector>


class pqLineChartModelItem
{
public:
  pqLineChartModelItem();
  pqLineChartModelItem(const pqLineChartModelItem &other);
  ~pqLineChartModelItem() {}

  bool updateRange(pqChartValue &min, pqChartValue &max);

  pqLineChartModelItem &operator=(const pqLineChartModelItem &other);
  bool operator==(const pqLineChartModelItem &other) const;
  bool operator!=(const pqLineChartModelItem &other) const;

  pqChartValue Minimum;
  pqChartValue Maximum;
  bool IsValid;
};


class pqLineChartModelInternal
{
public:
  pqLineChartModelInternal();
  ~pqLineChartModelInternal() {}

  QList<pqLineChartSeries *> List;
  QList<pqLineChartSeries *> MultiSeries;
  pqLineChartModelItem Axis[4];
  int LocationIndex[4];
};


//----------------------------------------------------------------------------
pqLineChartModelItem::pqLineChartModelItem()
  : Minimum(), Maximum()
{
  this->IsValid = false;
}

pqLineChartModelItem::pqLineChartModelItem(const pqLineChartModelItem &other)
  : Minimum(other.Minimum), Maximum(other.Maximum)
{
  this->IsValid = other.IsValid;
}

bool pqLineChartModelItem::updateRange(pqChartValue &min, pqChartValue &max)
{
  bool changed = false;
  if(this->IsValid)
    {
    if(min < this->Minimum)
      {
      changed = true;
      this->Minimum = min;
      }

    if(max > this->Maximum)
      {
      changed = true;
      this->Maximum = max;
      }
    }
  else
    {
    changed = true;
    this->IsValid = true;
    this->Minimum = min;
    this->Maximum = max;
    }

  return changed;
}

pqLineChartModelItem &pqLineChartModelItem::operator=(
    const pqLineChartModelItem &other)
{
  this->Minimum = other.Minimum;
  this->Maximum = other.Maximum;
  this->IsValid = other.IsValid;
  return *this;
}

bool pqLineChartModelItem::operator==(const pqLineChartModelItem &other) const
{
  return this->IsValid == other.IsValid && this->Minimum == other.Minimum &&
      this->Maximum == other.Maximum;
}

bool pqLineChartModelItem::operator!=(const pqLineChartModelItem &other) const
{
  return this->IsValid != other.IsValid || this->Minimum != other.Minimum ||
      this->Maximum != other.Maximum;
}


//----------------------------------------------------------------------------
pqLineChartModelInternal::pqLineChartModelInternal()
  : List(), MultiSeries()
{
  // Set up the axis location to index map.
  this->LocationIndex[pqChartAxis::Left] = 0;
  this->LocationIndex[pqChartAxis::Bottom] = 1;
  this->LocationIndex[pqChartAxis::Right] = 2;
  this->LocationIndex[pqChartAxis::Top] = 3;
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
  this->connect(series, SIGNAL(chartAxesChanged()),
      this, SLOT(handleSeriesAxesChanged()));
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

bool pqLineChartModel::getAxisRange(const pqChartAxis *axis, pqChartValue &min,
    pqChartValue &max) const
{
  int index = this->Internal->LocationIndex[axis->getLocation()];
  if(this->Internal->Axis[index].IsValid)
    {
    min = this->Internal->Axis[index].Minimum;
    max = this->Internal->Axis[index].Maximum;
    return true;
    }

  return false;
}

void pqLineChartModel::getRangeX(pqChartValue &min, pqChartValue &max) const
{
  int index = this->Internal->LocationIndex[pqChartAxis::Bottom];
  min = this->Internal->Axis[index].Minimum;
  max = this->Internal->Axis[index].Maximum;
}

void pqLineChartModel::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  int index = this->Internal->LocationIndex[pqChartAxis::Left];
  min = this->Internal->Axis[index].Minimum;
  max = this->Internal->Axis[index].Maximum;
}

void pqLineChartModel::handleSeriesAxesChanged()
{
  pqLineChartSeries *series = qobject_cast<pqLineChartSeries *>(this->sender());
  this->updateChartRanges();
  emit this->seriesChartAxesChanged(series);
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
  int index = 0;
  pqLineChartModelItem temp[4];
  pqChartValue seriesMin, seriesMax;
  QList<pqLineChartSeries *>::Iterator series = this->Internal->List.begin();
  for( ; series != this->Internal->List.end(); ++series)
    {
    pqLineChartSeries::ChartAxes axes = (*series)->getChartAxes();
    if(axes == pqLineChartSeries::BottomLeft ||
        axes == pqLineChartSeries::BottomRight)
      {
      index = this->Internal->LocationIndex[pqChartAxis::Bottom];
      }
    else
      {
      index = this->Internal->LocationIndex[pqChartAxis::Top];
      }

    (*series)->getRangeX(seriesMin, seriesMax);
    temp[index].updateRange(seriesMin, seriesMax);

    if(axes == pqLineChartSeries::BottomLeft ||
        axes == pqLineChartSeries::TopLeft)
      {
      index = this->Internal->LocationIndex[pqChartAxis::Left];
      }
    else
      {
      index = this->Internal->LocationIndex[pqChartAxis::Right];
      }

    (*series)->getRangeY(seriesMin, seriesMax);
    temp[index].updateRange(seriesMin, seriesMax);
    }

  bool changed = false;
  for(int i = 0; i < 4; i++)
    {
    if(temp[i] != this->Internal->Axis[i])
      {
      changed = true;
      this->Internal->Axis[i] = temp[i];
      }
    }

  if(changed)
    {
    emit this->chartRangeChanged();
    }
}

void pqLineChartModel::updateChartRanges(const pqLineChartSeries *series)
{
  int index = 0;
  bool xChanged = false;
  bool yChanged = false;
  pqChartValue seriesMin, seriesMax;
  pqLineChartSeries::ChartAxes axes = series->getChartAxes();
  if(axes == pqLineChartSeries::BottomLeft ||
      axes == pqLineChartSeries::BottomRight)
    {
    index = this->Internal->LocationIndex[pqChartAxis::Bottom];
    }
  else
    {
    index = this->Internal->LocationIndex[pqChartAxis::Top];
    }

  series->getRangeX(seriesMin, seriesMax);
  xChanged = this->Internal->Axis[index].updateRange(seriesMin, seriesMax);

  if(axes == pqLineChartSeries::BottomLeft ||
      axes == pqLineChartSeries::TopLeft)
    {
    index = this->Internal->LocationIndex[pqChartAxis::Left];
    }
  else
    {
    index = this->Internal->LocationIndex[pqChartAxis::Right];
    }

  series->getRangeY(seriesMin, seriesMax);
  yChanged = this->Internal->Axis[index].updateRange(seriesMin, seriesMax);

  if(xChanged || yChanged)
    {
    emit this->chartRangeChanged();
    }
}


