/// \file pqLineChartPlot.cxx
/// \date 9/7/2006

#include "pqLineChartPlot.h"

#include "pqChartCoordinate.h"
#include "pqChartValue.h"


pqLineChartPlot::pqLineChartPlot(QObject *parentObject)
  : QObject(parentObject)
{
}

void pqLineChartPlot::resetPlot()
{
  emit this->plotReset();
}

void pqLineChartPlot::beginInsertPoints(int series, int first, int last)
{
  emit this->aboutToInsertPoints(series, first, last);
}

void pqLineChartPlot::endInsertPoints(int series)
{
  emit this->pointsInserted(series);
}

void pqLineChartPlot::beginRemovePoints(int series, int first, int last)
{
  emit this->aboutToRemovePoints(series, first, last);
}

void pqLineChartPlot::endRemovePoints(int series)
{
  emit this->pointsRemoved(series);
}

void pqLineChartPlot::beginMultiSeriesChange()
{
  emit this->aboutToChangeMultipleSeries();
}

void pqLineChartPlot::endMultiSeriesChange()
{
  emit this->changedMultipleSeries();
}


