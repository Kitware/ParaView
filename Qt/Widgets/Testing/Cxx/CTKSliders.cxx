#include "ctkDoubleRangeSlider.h"
#include "ctkRangeSlider.h"

#include <QItemSelectionModel>
#include <QObject>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>

#include "QTestApp.h"
#include "SignalCatcher.h"

int CTKSliders(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  QWidget* root = new QWidget;
  QVBoxLayout* layout = new QVBoxLayout;

  ctkRangeSlider* rangeSlider = new ctkRangeSlider(Qt::Horizontal, 0);
  ctkDoubleRangeSlider* doubleSlider = new ctkDoubleRangeSlider(Qt::Horizontal);

  layout->addWidget(rangeSlider);
  layout->addWidget(doubleSlider);

  root->setLayout(layout);

  SignalCatcher* catcher = new SignalCatcher();

  rangeSlider->connect(
    rangeSlider, SIGNAL(valuesChanged(int, int)), catcher, SLOT(onValuesChanged(int, int)));

  doubleSlider->connect(doubleSlider, SIGNAL(valuesChanged(double, double)), catcher,
    SLOT(onValuesChanged(double, double)));

  root->show();

  int status = QTestApp::exec();

  delete root;
  delete catcher;

  return status;
}
