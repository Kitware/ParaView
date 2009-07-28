
#include "pqChartArea.h"
#include "pqChartValue.h"
#include "pqChartWidget.h"
#include "pqHistogramChart.h"
#include "pqHistogramWidget.h"
#include "pqSimpleHistogramModel.h"

#include <QApplication>
#include <QTimer>

#include "QTestApp.h"

int HistogramInteraction(int argc, char* argv[])
{
  QTestApp app(argc, argv);

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

  // zoom some
  pqChartArea *chartArea = chart->getChartArea();
  QTestApp::keyClick(chartArea, Qt::Key_Plus, Qt::NoModifier, 20);
  QTestApp::keyClick(chartArea, Qt::Key_Plus, Qt::NoModifier, 20);
  QTestApp::keyClick(chartArea, Qt::Key_Minus, Qt::NoModifier, 20);

  // pan some
  QTestApp::keyClick(chartArea, Qt::Key_Left, Qt::NoModifier, 20);
  QTestApp::keyClick(chartArea, Qt::Key_Right, Qt::NoModifier, 20);
  QTestApp::keyClick(chartArea, Qt::Key_Down, Qt::NoModifier, 20);
  QTestApp::keyClick(chartArea, Qt::Key_Up, Qt::NoModifier, 20);

  // some mouse interaction
  //QTestApp::mouseClick(chartArea, QPoint(size[0]/2, size[1]/2), 
  //                     Qt::LeftButton, 0, 20);
  //QTestApp::mouseClick(chartArea, QPoint(size[0]/3, size[1]/2),
  //                     Qt::LeftButton, 0, 20);
  
  //QTestApp::mouseDown(chartArea, QPoint(size[0]/4, size[1]/2),
  //                     Qt::LeftButton, 0, 40);
  //QTestApp::mouseMove(chartArea, QPoint(size[0]/2, size[1]/2),
  //                     Qt::LeftButton, 0, 40);
  //QTestApp::mouseUp(chartArea, QPoint(size[0]/2, size[1]/2),
  //                     Qt::LeftButton, 0, 40);

  if(QCoreApplication::arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
    }
  
  int status = QTestApp::exec();

  delete chart;
  delete model;

  return status;
}

