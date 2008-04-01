
#include "pqChartValue.h"
#include "pqColorMapColorChanger.h"
#include "pqColorMapWidget.h"
#include "pqColorMapModel.h"

#include <QApplication>
#include <QTimer>

int ColorMap(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Set up the color map.
  pqColorMapWidget *colorMap = new pqColorMapWidget();
  pqColorMapModel* cmodel = new pqColorMapModel(colorMap);
  colorMap->setModel(cmodel);
  colorMap->resize(250, 50);
  //colorMap->setTableSize(13);
  cmodel->addPoint(pqChartValue((double)0.0), QColor::fromHsv(240, 255, 255));
  cmodel->addPoint(pqChartValue((double)1.0), QColor::fromHsv(0, 255, 255));
  cmodel->addPoint(pqChartValue((double)0.5), QColor::fromHsv(60, 255, 255));

  // The color changer will be cleaned up when the color map is deleted.
  new pqColorMapColorChanger(colorMap);

  colorMap->show();

  if(app.arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
    }
  int status = QApplication::exec();

  delete colorMap;

  return status;
}

