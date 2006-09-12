/// \file pqLineChartModel.cxx
/// \date 9/8/2006

#include "pqLineChartModel.h"

#include "pqLineChartPlot.h"
#include <QList>


class pqLineChartModelInternal : public QList<const pqLineChartPlot *> {};


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
  return this->Internal->size();
}

const pqLineChartPlot *pqLineChartModel::getPlot(int index) const
{
  if(index >= 0 && index < this->Internal->size())
    {
    return this->Internal->at(index);
    }

  return 0;
}

void pqLineChartModel::appendPlot(const pqLineChartPlot *plot)
{
  this->insertPlot(plot, this->Internal->size());
}

void pqLineChartModel::insertPlot(const pqLineChartPlot *plot, int index)
{
  // Make sure the plot is not in the list.
  if(!plot || this->Internal->indexOf(plot) != -1)
    {
    return;
    }

  if(index < 0 || index > this->Internal->size())
    {
    index = this->Internal->size();
    }

  // Insert the new plot. Listen to the plot signals for rebroadcast.
  emit this->aboutToInsertPlots(index, index);
  this->Internal->insert(index, plot);
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
  emit this->plotsInserted(index, index);
}

void pqLineChartModel::removePlot(const pqLineChartPlot *plot)
{
  if(plot)
    {
    // Remove the plot by index.
    this->removePlot(this->Internal->indexOf(plot));
    }
}

void pqLineChartModel::removePlot(int index)
{
  if(index < 0 || index >= this->Internal->size())
    {
    return;
    }

  emit this->aboutToRemovePlots(index, index);
  const pqLineChartPlot *plot = this->Internal->takeAt(index);
  this->disconnect(plot, 0, this, 0);
  emit this->plotsRemoved(index, index);
}

void pqLineChartModel::movePlot(const pqLineChartPlot *plot, int index)
{
  if(plot)
    {
    // Move the plot by index.
    this->movePlot(this->Internal->indexOf(plot), index);
    }
}

void pqLineChartModel::movePlot(int current, int index)
{
  if(current < 0 || current >= this->Internal->size())
    {
    return;
    }

  if(index < 0 || index >= this->Internal->size())
    {
    index = this->Internal->size() - 1;
    }

  // Move the chart to the new place in the list.
  const pqLineChartPlot *plot = this->Internal->takeAt(current);
  this->Internal->insert(index, plot);
  emit this->plotMoved();
}

void pqLineChartModel::clearPlots()
{
  this->Internal->clear();
  emit this->plotsReset();
}

void pqLineChartModel::handlePlotReset()
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
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
  emit this->pointsRemoved(plot, series);
}

void pqLineChartModel::handlePlotBeginMultiSeriesChange()
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
  emit this->aboutToChangeMultipleSeries(plot);
}

void pqLineChartModel::handlePlotEndMultiSeriesChange()
{
  pqLineChartPlot *plot = qobject_cast<pqLineChartPlot *>(this->sender());
  emit this->changedMultipleSeries(plot);
}


