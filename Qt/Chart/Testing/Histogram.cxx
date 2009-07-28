
#include "pqChartArea.h"
#include "pqChartAxis.h"
#include "pqChartAxisOptions.h"
#include "pqChartCoordinate.h"
#include "pqChartInteractor.h"
#include "pqChartInteractorSetup.h"
#include "pqChartMouseSelection.h"
#include "pqChartSeriesOptionsGenerator.h"
#include "pqChartValue.h"
#include "pqChartWidget.h"
#include "pqHistogramChart.h"
#include "pqLineChartModel.h"
#include "pqLineChartOptions.h"
#include "pqLineChartSeriesOptions.h"
#include "pqLineChart.h"
#include "pqPointMarker.h"
#include "pqSimpleHistogramModel.h"
#include "pqSimpleLineChartSeries.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QPen>
#include <QSize>
#include <QTimer>

int Histogram(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Set up the chart widget and get the chart area.
  pqChartWidget *chart = new pqChartWidget();
  pqChartArea *chartArea = chart->getChartArea();

  // Set up the default interactor.
  pqChartMouseSelection *selection =
      pqChartInteractorSetup::createDefault(chartArea);

  // Set up the histogram.
  pqHistogramChart *histogram = new pqHistogramChart(chartArea);
  chartArea->insertLayer(chartArea->getAxisLayerIndex(), histogram);
  selection->setHistogram(histogram);
  selection->setSelectionMode("Histogram-Bin");

  // Set up the histogram data.
  pqSimpleHistogramModel *model = new pqSimpleHistogramModel();
  model->generateBoundaries(pqChartValue((int)0), pqChartValue((int)90), 9);

  model->setBinValue(0, pqChartValue((float)1.35));
  model->setBinValue(1, pqChartValue((float)1.40));
  model->setBinValue(2, pqChartValue((float)1.60));
  model->setBinValue(3, pqChartValue((float)2.00));
  model->setBinValue(4, pqChartValue((float)1.50));
  model->setBinValue(5, pqChartValue((float)1.80));
  model->setBinValue(6, pqChartValue((float)1.40));
  model->setBinValue(7, pqChartValue((float)1.30));
  model->setBinValue(8, pqChartValue((float)1.20));
  histogram->setModel(model);

  // Set up the line chart.
  pqLineChart *lineView = new pqLineChart(chartArea);
  lineView->getOptions()->getGenerator()->setColorScheme(
      pqChartSeriesOptionsGenerator::WildFlower);
  chartArea->addLayer(lineView);

  // Change the color of the right axis to differentiate it.
  chartArea->getAxis(pqChartAxis::Right)->getOptions()->setAxisColor(
      Qt::darkBlue);

  // Set up the line chart data.
  pqLineChartModel *lines = new pqLineChartModel();
  pqSimpleLineChartSeries *plot = new pqSimpleLineChartSeries();
  plot->setChartAxes(pqLineChartSeries::BottomRight);
  plot->addSequence(pqLineChartSeries::Line);
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
  plot->addSequence(pqLineChartSeries::Point);
  plot->copySequencePoints(0, 1);
  lines->appendSeries(plot);

  lineView->setModel(lines);

  // Set up the drawing options for the plot.
  pqCirclePointMarker circle(QSize(3, 3));
  pqLineChartSeriesOptions *options = lineView->getOptions()->getSeriesOptions(0);
  options->setSequenceDependent(true);
  options->setPen(QPen(QColor(Qt::black), 1.0), 0);
  options->setPen(QPen(QColor(Qt::black), 0.5), 1);
  options->setBrush(QBrush(Qt::white), 1);
  options->setMarker(&circle, 1);

  chart->show();

  if(app.arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
    }
  int status = QApplication::exec();

  delete chart;
  delete model;
  delete lines;
  delete plot;

  return status;
}

