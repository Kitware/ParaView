
#include "pqChartValue.h"
#include "pqHistogramChart.h"
#include "pqHistogramListModel.h"
#include "pqHistogramWidget.h"

#include <QApplication>
#include <QColor>
#include <QTimer>
#include <QKeyEvent>
#include <QtTest>

int HistogramInteraction(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Set up the histogram.
  pqHistogramWidget *histogram = new pqHistogramWidget();
  int size[2] = {300,300};
  histogram->resize(size[0], size[1]);

  // Set up the histogram data.
  pqHistogramListModel *model = new pqHistogramListModel();
  model->addBinValue(pqChartValue((int)1));
  model->addBinValue(pqChartValue((int)1));
  model->addBinValue(pqChartValue((int)2));
  model->addBinValue(pqChartValue((int)4));
  model->addBinValue(pqChartValue((int)3));
  model->addBinValue(pqChartValue((int)3));
  model->addBinValue(pqChartValue((int)2));
  model->addBinValue(pqChartValue((int)1));
  model->addBinValue(pqChartValue((int)1));
  pqChartValue min((int)0);
  pqChartValue max((int)90);
  model->setRangeX(min, max);
  histogram->getHistogram().setModel(model);

  histogram->show();

  int status = 0;

  // zoom some
  QTest::keyClick(histogram, Qt::Key_Plus, Qt::NoModifier, 20);
  QTest::keyClick(histogram, Qt::Key_Plus, Qt::NoModifier, 20);
  QTest::keyClick(histogram, Qt::Key_Minus, Qt::NoModifier, 20);
  // pan some
  QTest::keyClick(histogram, Qt::Key_Left, Qt::NoModifier, 20);
  QTest::keyClick(histogram, Qt::Key_Right, Qt::NoModifier, 20);
  QTest::keyClick(histogram, Qt::Key_Down, Qt::NoModifier, 20);
  QTest::keyClick(histogram, Qt::Key_Up, Qt::NoModifier, 20);

  // some mouse interaction
  QTest::mouseClick(histogram->viewport(), Qt::LeftButton, 0, 
                    QPoint(size[0]/2, size[1]/2), 20);
  QTest::mouseClick(histogram->viewport(), Qt::LeftButton, 0, 
                    QPoint(size[0]/3, size[1]/2), 20);

  if(app.arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
    }
  status += QApplication::exec();

  delete histogram;
  delete model;

  return status;
}

