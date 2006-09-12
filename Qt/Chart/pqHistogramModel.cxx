
/// \file pqHistogramModel.cxx
/// \date 8/15/2006

#include "pqHistogramModel.h"


pqHistogramModel::pqHistogramModel(QObject *parentObject)
  : QObject(parentObject)
{
}

void pqHistogramModel::resetBinValues()
{
  emit this->binValuesReset();
}

void pqHistogramModel::beginInsertBinValues(int first, int last)
{
  emit this->aboutToInsertBinValues(first, last);
}

void pqHistogramModel::endInsertBinValues()
{
  emit this->binValuesInserted();
}

void pqHistogramModel::beginRemoveBinValues(int first, int last)
{
  emit this->aboutToRemoveBinValues(first, last);
}

void pqHistogramModel::endRemoveBinValues()
{
  emit this->binValuesRemoved();
}


