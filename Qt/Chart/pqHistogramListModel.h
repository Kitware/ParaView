
/// \file pqHistogramListModel.h
/// \date 8/15/2006

#ifndef _pqHistogramListModel_h
#define _pqHistogramListModel_h


#include "QtChartExport.h"
#include "pqHistogramModel.h"

class pqChartValue;
class pqChartValueList;
class pqHistogramListModelInternal;


class QTCHART_EXPORT pqHistogramListModel : public pqHistogramModel
{
  Q_OBJECT

public:
  pqHistogramListModel(QObject *parent=0);
  virtual ~pqHistogramListModel();

  virtual int getNumberOfBins() const;
  virtual void getBinValue(int index, pqChartValue &bin) const;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const;
  virtual void getMinimumX(pqChartValue &min) const;
  virtual void getMaximumX(pqChartValue &max) const;

  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const;
  virtual void getMinimumY(pqChartValue &min) const;
  virtual void getMaximumY(pqChartValue &max) const;

  void clearBinValues();
  void setBinValues(const pqChartValueList &values);
  void addBinValue(const pqChartValue &bin);
  void insertBinValue(int index, const pqChartValue &bin);
  void removeBinValue(int index);

  void setRangeX(const pqChartValue &min, const pqChartValue &max);
  void setMinimumX(const pqChartValue &min);
  void setMaximumX(const pqChartValue &max);

private:
  pqHistogramListModelInternal *Internal; ///< Stores the histogram data.
};

#endif
