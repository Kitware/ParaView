
#include "pqChartValue.h"
#include "pqHistogramChart.h"
#include "pqHistogramListModel.h"
#include "pqHistogramWidget.h"

#include <QApplication>
#include <QColor>
#include <QTimer>
#include <QKeyEvent>

int sendEvent(QWidget* w, QEvent* e)
{
  QApplication::sendEvent(w, e);
  QTimer::singleShot(50, QApplication::instance(), SLOT(quit()));
  return QApplication::exec();
}

int sendKeyEvent(QWidget* w, Qt::Key k, Qt::KeyboardModifiers m = 0)
{
  QKeyEvent kd(QEvent::KeyPress, k, m);
  QKeyEvent ku(QEvent::KeyRelease, k, m);
  return sendEvent(w, &kd) + sendEvent(w, &ku);
}

int HistogramInteraction(int argc, char* argv[])
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

  int status = 0;

  // zoom some
  status += sendKeyEvent(histogram, Qt::Key_Plus);
  status += sendKeyEvent(histogram, Qt::Key_Plus);
  status += sendKeyEvent(histogram, Qt::Key_Minus);
  // pan some
  status += sendKeyEvent(histogram, Qt::Key_Left);
  status += sendKeyEvent(histogram, Qt::Key_Right);
  status += sendKeyEvent(histogram, Qt::Key_Down);
  status += sendKeyEvent(histogram, Qt::Key_Up);

  // TODO  add some mouse interaction

  if(app.arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
    }
  status += QApplication::exec();

  delete histogram;
  delete model;

  return status;
}

