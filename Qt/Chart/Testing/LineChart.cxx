
#include "pqChartCoordinate.h"
#include "pqChartLegend.h"
#include "pqChartLegendModel.h"
#include "pqChartValue.h"
#include "pqChartWidget.h"
#include "pqChartSeriesOptionsGenerator.h"
#include "pqLineChart.h"
#include "pqLineChartModel.h"
#include "pqLineChartOptions.h"
#include "pqLineChartSeriesOptions.h"
#include "pqLineChartWidget.h"
#include "pqPointMarker.h"
#include "pqSimpleLineChartSeries.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QPen>
#include <QTimer>

int LineChart(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Create the chart widget.
  pqLineChart *lineChart = 0;
  pqChartWidget *chart = pqLineChartWidget::createLineChart(0, &lineChart);
  lineChart->getOptions()->getGenerator()->setColorScheme(
      pqChartSeriesOptionsGenerator::WildFlower);
  
  // Set up the line chart model.
  pqLineChartModel *lines = new pqLineChartModel();
  pqSimpleLineChartSeries *plot = new pqSimpleLineChartSeries(lines);
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

  lineChart->setModel(lines);

  // add a legend
  pqChartLegendModel *legendModel = new pqChartLegendModel();
  pqPlusPointMarker plus(QSize(10,10));
  QPen plusPen(Qt::SolidLine);
  legendModel->addEntry(pqChartLegendModel::generateLineIcon(
      QPen(Qt::DotLine), &plus, &plusPen), "label1");
  pqChartLegend *legend = new pqChartLegend();
  legend->setModel(legendModel);
  legend->setLocation(pqChartLegend::Right);
  legend->setFlow(pqChartLegend::TopToBottom);
  chart->setLegend(legend);

  chart->show();

  if(app.arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
    }
  int status = QApplication::exec();

  delete chart;
  delete lines;
  delete legendModel;

  return status;
}

