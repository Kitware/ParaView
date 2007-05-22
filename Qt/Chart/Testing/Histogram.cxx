
#include "pqChartCoordinate.h"
#include "pqChartValue.h"
#include "pqHistogramChart.h"
#include "pqHistogramListModel.h"
#include "pqHistogramWidget.h"
#include "pqLineChart.h"
#include "pqLineChartModel.h"
#include "pqLineChartPlotOptions.h"
#include "pqPointMarker.h"
#include "pqSimpleLineChartPlot.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QPen>
#include <QSize>
#include <QTimer>

int Histogram(int argc, char** argv)
{
  QApplication app(argc, argv);

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

  // Add a line chart over the histogram.
  pqLineChartModel *lines = new pqLineChartModel();
  pqSimpleLineChartPlot *plot = new pqSimpleLineChartPlot();
  plot->addSeries(pqLineChartPlot::Line);
  plot->addPoint(0, pqChartCoordinate(pqChartValue((int)0),
      pqChartValue((float)1.2)));
  plot->addPoint(0, pqChartCoordinate(pqChartValue((int)10),
      pqChartValue((float)1.6)));
  plot->addPoint(0, pqChartCoordinate(pqChartValue((int)20),
      pqChartValue((float)1.5)));
  plot->addPoint(0, pqChartCoordinate(pqChartValue((int)30),
      pqChartValue((float)1.8)));
  plot->addPoint(0, pqChartCoordinate(pqChartValue((int)40),
      pqChartValue((float)2.0)));
  plot->addPoint(0, pqChartCoordinate(pqChartValue((int)50),
      pqChartValue((float)1.9)));
  plot->addPoint(0, pqChartCoordinate(pqChartValue((int)60),
      pqChartValue((float)1.4)));
  plot->addPoint(0, pqChartCoordinate(pqChartValue((int)70),
      pqChartValue((float)1.3)));
  plot->addPoint(0, pqChartCoordinate(pqChartValue((int)80),
      pqChartValue((float)1.2)));
  plot->addPoint(0, pqChartCoordinate(pqChartValue((int)90),
      pqChartValue((float)1.0)));
  plot->addSeries(pqLineChartPlot::Point);
  plot->copySeriesPoints(0, 1);
  lines->appendPlot(plot);

  // Set up the drawing options for the plot.
  pqCirclePointMarker circle(QSize(3, 3));
  pqLineChartPlotOptions *options = new pqLineChartPlotOptions();
  options->setPen(0, QPen(QColor(Qt::black), 1.0));
  options->setPen(1, QPen(QColor(Qt::black), 0.5));
  options->setBrush(1, QBrush(Qt::white));
  options->setMarker(1, &circle);

  lines->setOptions(0, options);
  histogram->getLineChart().setModel(lines);

  histogram->show();

  if(app.arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
    }
  int status = QApplication::exec();

  delete histogram;
  delete model;
  delete lines;
  delete plot;
  delete options;

  return status;
}

