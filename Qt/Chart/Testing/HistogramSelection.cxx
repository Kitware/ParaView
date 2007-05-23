
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

int HistogramSelection(int argc, char* argv[])
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

  histogram->show();

  histogram->setBackgroundColor(QColor("purple"));
  histogram->setFont(QFont("Courier", -1, -1, true));

  histogram->selectAll();
  histogram->selectNone();
  histogram->selectInverse();
  histogram->setInteractMode(pqHistogramWidget::Value);
  histogram->selectAll();
  histogram->selectNone();
  histogram->selectInverse();

  if(app.arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
    }
  int status = QApplication::exec();

  delete histogram;
  delete model;

  return status;
}

