
/// \file pqHistogramModel.h
/// \date 8/15/2006

#ifndef _pqHistogramModel_h
#define _pqHistogramModel_h


#include "QtChartExport.h"
#include <QObject>

class pqChartValue;


class QTCHART_EXPORT pqHistogramModel : public QObject
{
  Q_OBJECT

public:
  pqHistogramModel(QObject *parent=0);
  virtual ~pqHistogramModel() {}

  virtual int getNumberOfBins() const=0;
  virtual void getBinValue(int index, pqChartValue &bin) const=0;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const=0;
  virtual void getMinimumX(pqChartValue &min) const=0;
  virtual void getMaximumX(pqChartValue &max) const=0;

  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const=0;
  virtual void getMinimumY(pqChartValue &min) const=0;
  virtual void getMaximumY(pqChartValue &max) const=0;

signals:
  void binValuesReset();
  void aboutToInsertBinValues(int first, int last);
  void binValuesInserted();
  void aboutToRemoveBinValues(int first, int last);
  void binValuesRemoved();
  void rangeChanged(const pqChartValue &min, const pqChartValue &max);

protected:
  void resetBinValues();
  void beginInsertBinValues(int first, int last);
  void endInsertBinValues();
  void beginRemoveBinValues(int first, int last);
  void endRemoveBinValues();
};

#endif
