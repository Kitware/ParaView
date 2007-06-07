
#include "pqChartValue.h"
#include "pqChartWidget.h"
#include "pqHistogramChart.h"
#include "pqHistogramWidget.h"
#include "pqSimpleHistogramModel.h"

#include <QApplication>
#include <QFont>
#include <QTimer>

int HistogramSelection(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Set up the histogram.
  pqHistogramChart *histogram = 0;
  pqChartWidget *chart = pqHistogramWidget::createHistogram(0, &histogram);

  // Resize the chart.
  int size[2] = {300,300};
  chart->resize(size[0], size[1]);

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

  chart->show();

  // TODO
  //histogram->setBackgroundColor(QColor("purple"));
  chart->setFont(QFont("Courier", -1, -1, true));

  //histogram->selectAll();
  //histogram->selectNone();
  //histogram->selectInverse();
  //histogram->setInteractMode(pqHistogramWidget::Value);
  //histogram->selectAll();
  //histogram->selectNone();
  //histogram->selectInverse();

  if(app.arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
    }
  int status = QApplication::exec();

  delete chart;
  delete model;

  return status;
}

