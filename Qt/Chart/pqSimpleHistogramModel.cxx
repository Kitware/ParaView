/*=========================================================================

   Program: ParaView
   Module:    pqSimpleHistogramModel.cxx

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

/// \file pqSimpleHistogramModel.cxx
/// \date 8/15/2006

#include "pqSimpleHistogramModel.h"

#include "pqChartCoordinate.h"
#include "pqChartValue.h"
#include <QVector>


class pqSimpleHistogramModelInternal
{
public:
  pqSimpleHistogramModelInternal();
  ~pqSimpleHistogramModelInternal() {}

  QVector<pqChartValue> Values;
  QVector<pqChartValue> Boundaries;
  pqChartCoordinate Minimum;
  pqChartCoordinate Maximum;
  bool InModify;
};


//----------------------------------------------------------------------------
pqSimpleHistogramModelInternal::pqSimpleHistogramModelInternal()
  : Values(), Boundaries(), Minimum(), Maximum()
{
  this->InModify = false;
}


//----------------------------------------------------------------------------
pqSimpleHistogramModel::pqSimpleHistogramModel(QObject *parentObject)
  : pqHistogramModel(parentObject)
{
  this->Internal = new pqSimpleHistogramModelInternal();
}

pqSimpleHistogramModel::~pqSimpleHistogramModel()
{
  delete this->Internal;
}

int pqSimpleHistogramModel::getNumberOfBins() const
{
  return this->Internal->Values.size();
}

void pqSimpleHistogramModel::getBinValue(int index, pqChartValue &bin) const
{
  if(index >= 0 && index < this->Internal->Values.size())
    {
    bin = this->Internal->Values[index];
    }
}

void pqSimpleHistogramModel::getBinRange(int index, pqChartValue &min,
    pqChartValue &max) const
{
  if(index >= 0 && index < this->Internal->Values.size())
    {
    min = this->Internal->Boundaries[index];
    max = this->Internal->Boundaries[index + 1];
    }
}

void pqSimpleHistogramModel::getRangeX(pqChartValue &min,
    pqChartValue &max) const
{
  min = this->Internal->Minimum.X;
  max = this->Internal->Maximum.X;
}

void pqSimpleHistogramModel::getRangeY(pqChartValue &min,
    pqChartValue &max) const
{
  min = this->Internal->Minimum.Y;
  max = this->Internal->Maximum.Y;
}

void pqSimpleHistogramModel::startModifyingData()
{
  this->Internal->InModify = true;
}

void pqSimpleHistogramModel::finishModifyingData()
{
  if(this->Internal->InModify)
    {
    this->Internal->InModify = false;
    emit this->histogramReset();
    }
}

void pqSimpleHistogramModel::addBinRangeBoundary(const pqChartValue &value)
{
  // Find the insertion point.
  int index = -1;
  QVector<pqChartValue>::Iterator iter = this->Internal->Boundaries.begin();
  for( ; iter != this->Internal->Boundaries.end(); ++iter, ++index)
    {
    if(*iter == value)
      {
      // Ignore duplicate entries.
      return;
      }

    if(value < *iter)
      {
      break;
      }
    }

  if(index < 0)
    {
    index = 0;
    }

  bool addingBin = this->Internal->Boundaries.size() > 0;
  if(addingBin && !this->Internal->InModify)
    {
    this->beginInsertBins(index, index);
    }

  // Add the boundary to the list.
  if(iter == this->Internal->Boundaries.end())
    {
    this->Internal->Boundaries.append(value);
    }
  else
    {
    this->Internal->Boundaries.insert(iter, value);
    }

  this->updateXRange();
  if(addingBin)
    {
    // Add a bin to the list of values for the new boundary.
    if(index < this->Internal->Values.size())
      {
      this->Internal->Values.insert(index, pqChartValue());
      }
    else
      {
      this->Internal->Values.append(pqChartValue());
      }

    if(!this->Internal->InModify)
      {
      this->endInsertBins();

      // Adding a new bin can change the previous bin's range.
      if(index > 0 && index < this->Internal->Values.size() - 1)
        {
        index--;
        emit this->binRangesChanged(index, index);
        }
      }
    }
}

void pqSimpleHistogramModel::removeBinRangeBoundary(int index)
{
  if(index >= 0 && index < this->Internal->Boundaries.size())
    {
    int bin = index;
    if(index == this->Internal->Boundaries.size() - 1)
      {
      // Removing the last boundary removes the last bin as well.
      bin--;
      }

    bool removingBin = bin >= 0 && bin < this->Internal->Values.size();
    if(removingBin && !this->Internal->InModify)
      {
      this->beginRemoveBins(bin, bin);
      }

    this->Internal->Boundaries.remove(index);
    this->updateXRange();
    if(removingBin)
      {
      this->Internal->Values.remove(bin);
      this->updateYRange();
      if(!this->Internal->InModify)
        {
        this->endRemoveBins();

        // Removing a boundary can change the range of the previous
        // bin. Nothing has changed if the bin was first or last.
        if(index < this->Internal->Boundaries.size() && --bin >= 0)
          {
          emit this->binRangesChanged(bin, bin);
          }
        }
      }
    }
}

void pqSimpleHistogramModel::clearBinRangeBoundaries()
{
  if(this->Internal->Values.size() > 0)
    {
    this->Internal->Minimum.X = (int)0;
    this->Internal->Minimum.Y = (int)0;
    this->Internal->Maximum.X = (int)0;
    this->Internal->Maximum.Y = (int)0;
    this->Internal->Values.clear();
    this->Internal->Boundaries.clear();
    if(!this->Internal->InModify)
      {
      emit this->histogramReset();
      }
    }
}

void pqSimpleHistogramModel::generateBoundaries(const pqChartValue &min,
    const pqChartValue &max, int intervals)
{
  if(intervals < 1)
    {
    return;
    }

  pqChartValue interval = (max - min) / intervals;
  if(interval == 0)
    {
    return;
    }

  this->clearBinRangeBoundaries();
  for(pqChartValue value = min; value <= max; value += interval)
    {
    this->addBinRangeBoundary(value);
    }

  // Account for roundoff error.
  if(this->Internal->Values.size() < intervals)
    {
    this->addBinRangeBoundary(max);
    }
}

void pqSimpleHistogramModel::setBinValue(int index, const pqChartValue &value)
{
  if(index >= 0 && index < this->Internal->Values.size())
    {
    this->Internal->Values[index] = value;
    this->updateYRange();
    if(!this->Internal->InModify)
      {
      emit this->binValuesChanged(index, index);
      }
    }
}

void pqSimpleHistogramModel::updateXRange()
{
  pqChartValue min, max;
  if(this->Internal->Boundaries.size() > 0)
    {
    min = this->Internal->Boundaries.first();
    max = this->Internal->Boundaries.last();
    }

  if(min != this->Internal->Minimum.X || max != this->Internal->Maximum.X)
    {
    this->Internal->Minimum.X = min;
    this->Internal->Maximum.X = max;
    emit this->histogramRangeChanged();
    }
}

void pqSimpleHistogramModel::updateYRange()
{
  pqChartValue min, max;
  QVector<pqChartValue>::Iterator iter = this->Internal->Values.begin();
  if(iter != this->Internal->Values.end())
    {
    min = *iter;
    max = *iter;
    ++iter;
    }

  for( ; iter != this->Internal->Values.end(); ++iter)
    {
    if(*iter < min)
      {
      min = *iter;
      }

    if(*iter > max)
      {
      max = *iter;
      }
    }

  if(min != this->Internal->Minimum.Y || max != this->Internal->Maximum.Y)
    {
    this->Internal->Minimum.Y = min;
    this->Internal->Maximum.Y = max;
    emit this->histogramRangeChanged();
    }
}


