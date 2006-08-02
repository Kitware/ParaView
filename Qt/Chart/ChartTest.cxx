
#include "pqChartValue.h"
#include "pqColorMapColorChanger.h"
#include "pqColorMapWidget.h"

#include <QApplication>
#include <QColor>

// TODO: Expand the chart test to test other charts.

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  // Set up the color map.
  pqColorMapWidget *colorMap = new pqColorMapWidget();
  colorMap->resize(250, 50);
  //colorMap->setTableSize(13);
  colorMap->addPoint(pqChartValue((double)0.0), QColor::fromHsv(240, 255, 255));
  colorMap->addPoint(pqChartValue((double)1.0), QColor::fromHsv(0, 255, 255));
  colorMap->addPoint(pqChartValue((double)0.5), QColor::fromHsv(60, 255, 255));

  // The color changer will be cleaned up when the color map is deleted.
  new pqColorMapColorChanger(colorMap);

  colorMap->show();
  int status = app.exec();

  delete colorMap;

  return status;
}


