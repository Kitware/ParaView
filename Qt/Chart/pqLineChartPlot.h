/// \file pqLineChartPlot.h
/// \date 9/7/2006

#ifndef _pqLineChartPlot_h
#define _pqLineChartPlot_h


#include "QtChartExport.h"
#include <QObject>

class pqChartCoordinate;
class pqChartValue;


class QTCHART_EXPORT pqLineChartPlot : public QObject
{
  Q_OBJECT

public:
  enum SeriesType
    {
    Invalid,
    Point,
    Line
    };

public:
  pqLineChartPlot(QObject *parent=0);
  virtual ~pqLineChartPlot() {}

  virtual int getNumberOfSeries() const=0;
  virtual SeriesType getSeriesType(int series) const=0;
  virtual int getNumberOfPoints(int series) const=0;
  virtual void getPoint(int series, int index,
      pqChartCoordinate &coord) const=0;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const=0;
  virtual void getMinimumX(pqChartValue &min) const=0;
  virtual void getMaximumX(pqChartValue &max) const=0;

  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const=0;
  virtual void getMinimumY(pqChartValue &min) const=0;
  virtual void getMaximumY(pqChartValue &max) const=0;

signals:
  void plotReset();
  void aboutToInsertPoints(int series, int first, int last);
  void pointsInserted(int series);
  void aboutToRemovePoints(int series, int first, int last);
  void pointsRemoved(int series);
  void aboutToChangeMultipleSeries();
  void changedMultipleSeries();

protected:
  void resetPlot();
  void beginInsertPoints(int series, int first, int last);
  void endInsertPoints(int series);
  void beginRemovePoints(int series, int first, int last);
  void endRemovePoints(int series);
  void beginMultiSeriesChange();
  void endMultiSeriesChange();
};

#endif
