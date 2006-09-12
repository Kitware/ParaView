
#include "pqChartValue.h"
#include "pqColorMapColorChanger.h"
#include "pqColorMapWidget.h"
#include "pqHistogramChart.h"
#include "pqHistogramListModel.h"
#include "pqHistogramWidget.h"

#include <QApplication>
#include <QColor>

// TODO: Expand the chart test to test other charts.

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  // Set up the color map.
  /*pqColorMapWidget *colorMap = new pqColorMapWidget();
  colorMap->resize(250, 50);
  //colorMap->setTableSize(13);
  colorMap->addPoint(pqChartValue((double)0.0), QColor::fromHsv(240, 255, 255));
  colorMap->addPoint(pqChartValue((double)1.0), QColor::fromHsv(0, 255, 255));
  colorMap->addPoint(pqChartValue((double)0.5), QColor::fromHsv(60, 255, 255));

  // The color changer will be cleaned up when the color map is deleted.
  new pqColorMapColorChanger(colorMap);

  colorMap->show();*/

  // Set up the histogram.
  pqHistogramWidget *histogram = new pqHistogramWidget();

  // Set up the histogram data.
  pqHistogramListModel *model = new pqHistogramListModel();
  model->addBinValue(pqChartValue((float)1.35));
  model->addBinValue(pqChartValue((float)1.40));
  model->addBinValue(pqChartValue((float)1.60));
  model->addBinValue(pqChartValue((float)2.00));
  model->addBinValue(pqChartValue((float)1.50));
  model->addBinValue(pqChartValue((float)1.80));
  model->addBinValue(pqChartValue((float)1.40));
  model->addBinValue(pqChartValue((float)1.30));
  model->addBinValue(pqChartValue((float)1.20));
  pqChartValue min((int)0);
  pqChartValue max((int)90);
  model->setRangeX(min, max);
  histogram->getHistogram().setModel(model);

  histogram->show();
  int status = app.exec();

  //delete colorMap;
  delete histogram;
  delete model;

  return status;
}


