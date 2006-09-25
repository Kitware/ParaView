/*=========================================================================

   Program: ParaView
   Module:    pqHistogramListModel.cxx

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

/// \file pqHistogramListModel.cxx
/// \date 8/15/2006

#include "pqHistogramListModel.h"

#include "pqChartValue.h"
#include <QList>


class pqHistogramListModelInternal
{
public:
  pqHistogramListModelInternal();
  ~pqHistogramListModelInternal() {}

  QList<pqChartValue> Values;
  pqChartValue MinimumX;
  pqChartValue MaximumX;
  pqChartValue MinimumY;
  pqChartValue MaximumY;
};


//----------------------------------------------------------------------------
pqHistogramListModelInternal::pqHistogramListModelInternal()
  : Values(), MinimumX(), MaximumX(), MinimumY(), MaximumY()
{
}


//----------------------------------------------------------------------------
pqHistogramListModel::pqHistogramListModel(QObject *parentObject)
  : pqHistogramModel(parentObject)
{
  this->Internal = new pqHistogramListModelInternal();
}

pqHistogramListModel::~pqHistogramListModel()
{
  delete this->Internal;
}

int pqHistogramListModel::getNumberOfBins() const
{
  return this->Internal->Values.size();
}

void pqHistogramListModel::getBinValue(int index, pqChartValue &bin) const
{
  if(index >= 0 && index < this->Internal->Values.size())
    {
    bin = this->Internal->Values[index];
    }
}

void pqHistogramListModel::getRangeX(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->MinimumX;
  max = this->Internal->MaximumX;
}

void pqHistogramListModel::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->MinimumY;
  max = this->Internal->MaximumY;
}

void pqHistogramListModel::clearBinValues()
{
  if(this->Internal->Values.size() > 0)
    {
    this->Internal->Values.clear();
    this->resetBinValues();
    }
}

void pqHistogramListModel::setBinValues(const pqChartValueList &values)
{
  // Clear the previous bin values.
  this->Internal->Values.clear();

  // Copy the new list of bin values. Find the range of the new
  // values as well.
  pqChartValue yMin;
  pqChartValueList::ConstIterator iter = values.begin();
  if(iter != values.end())
    {
    yMin = *iter;
    }

  pqChartValue yMax = yMin;
  for( ; iter != values.end(); ++iter)
    {
    this->Internal->Values.append(*iter);
    if(*iter > yMax)
      {
      yMax = *iter;
      }
    else if(*iter < yMin)
      {
      yMin = *iter;
      }
    }

  // Set the bin value range and notify the chart that the model has
  // changed.
  this->Internal->MinimumY = yMin;
  this->Internal->MaximumY = yMax;
  this->resetBinValues();
}

void pqHistogramListModel::addBinValue(const pqChartValue &value)
{
  int index = this->Internal->Values.size();
  this->insertBinValue(index, value);
}

void pqHistogramListModel::insertBinValue(int index, const pqChartValue &value)
{
  // Insert the new bin value into the list. Adjust the bin value
  // range before finishing the insert.
  this->beginInsertBinValues(index, index);
  this->Internal->Values.insert(index, value);
  if(this->Internal->Values.size() == 1)
    {
    this->Internal->MinimumY = value;
    this->Internal->MaximumY = value;
    }
  else if(value < this->Internal->MinimumY)
    {
    this->Internal->MinimumY = value;
    }
  else if(value > this->Internal->MaximumY)
    {
    this->Internal->MaximumY = value;
    }

  this->endInsertBinValues();
}

void pqHistogramListModel::removeBinValue(int index)
{
  if(index >= 0 && index < this->Internal->Values.size())
    {
    // Remove the bin value from the list. Recalculate the bin value
    // range after removing the value.
    this->beginRemoveBinValues(index, index);
    this->Internal->Values.removeAt(index);
    if(this->Internal->Values.size() == 0)
      {
      this->Internal->MinimumY = pqChartValue();
      this->Internal->MaximumY = pqChartValue();
      }
    else
      {
      QList<pqChartValue>::Iterator iter = this->Internal->Values.begin();
      this->Internal->MinimumY = *iter;
      this->Internal->MaximumY = *iter;
      for( ; iter != this->Internal->Values.end(); ++iter)
        {
        if(*iter < this->Internal->MinimumY)
          {
          this->Internal->MinimumY = *iter;
          }
        else if(*iter > this->Internal->MaximumY)
          {
          this->Internal->MaximumY = *iter;
          }
        }
      }

    this->endRemoveBinValues();
    }
}

void pqHistogramListModel::setRangeX(const pqChartValue &min,
    const pqChartValue &max)
{
  bool isRangeChanged = false;
  if(this->Internal->MinimumX != min)
    {
    this->Internal->MinimumX = min;
    isRangeChanged = true;
    }

  if(this->Internal->MaximumX != max)
    {
    this->Internal->MaximumX = max;
    isRangeChanged = true;
    }

  if(isRangeChanged)
    {
    emit this->rangeChanged(this->Internal->MinimumX, this->Internal->MaximumX);
    }
}

void pqHistogramListModel::setMinimumX(const pqChartValue &min)
{
  this->setRangeX(min, this->Internal->MaximumX);
}

void pqHistogramListModel::setMaximumX(const pqChartValue &max)
{
  this->setRangeX(this->Internal->MinimumX, max);
}


