
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

void pqHistogramListModel::getMinimumX(pqChartValue &min) const
{
  min = this->Internal->MinimumX;
}

void pqHistogramListModel::getMaximumX(pqChartValue &max) const
{
  max = this->Internal->MaximumX;
}

void pqHistogramListModel::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->MinimumY;
  max = this->Internal->MaximumY;
}

void pqHistogramListModel::getMinimumY(pqChartValue &min) const
{
  min = this->Internal->MinimumY;
}

void pqHistogramListModel::getMaximumY(pqChartValue &max) const
{
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


