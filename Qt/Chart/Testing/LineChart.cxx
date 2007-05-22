
#include "pqChartValue.h"
#include "pqChartCoordinate.h"
#include "pqLineChartWidget.h"
#include "pqLineChart.h"
#include "pqLineChartModel.h"
#include "pqLineChartPlotOptions.h"
#include "pqPointMarker.h"
#include "pqSimpleLineChartPlot.h"
#include "pqChartLegend.h"
#include "pqMarkerPen.h"
#include "pqChartLabel.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QPen>
#include <QSize>
#include <QTimer>

int LineChart(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Set up the line chart.
  pqLineChartWidget *lineChart = new pqLineChartWidget();
  
  // Add a line chart over the histogram.
  pqLineChartModel *lines = new pqLineChartModel();
  pqSimpleLineChartPlot *plot = new pqSimpleLineChartPlot(lines);
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

  lineChart->getLineChart().setModel(lines);

  // add a legend
  lineChart->getLegend().addEntry(new pqPlusMarkerPen(QPen(Qt::DotLine),
                                                      QSize(10,10),
                                                      QPen(Qt::SolidLine)),
                                  new pqChartLabel("label1", 
                                                   &lineChart->getLegend()));

  lineChart->show();

  if(app.arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
    }
  int status = QApplication::exec();

  delete lineChart;
  delete lines;

  return status;
}

