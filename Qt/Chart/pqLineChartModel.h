/// \file pqLineChartModel.h
/// \date 9/8/2006

#ifndef _pqLineChartModel_h
#define _pqLineChartModel_h


#include "QtChartExport.h"
#include <QObject>

class pqLineChartModelInternal;
class pqLineChartPlot;


class QTCHART_EXPORT pqLineChartModel : public QObject
{
  Q_OBJECT

public:
  pqLineChartModel(QObject *parent=0);
  virtual ~pqLineChartModel();

  int getNumberOfPlots() const;
  const pqLineChartPlot *getPlot(int index) const;
  void appendPlot(const pqLineChartPlot *plot);
  void insertPlot(const pqLineChartPlot *plot, int index);
  void removePlot(const pqLineChartPlot *plot);
  void removePlot(int index);
  void movePlot(const pqLineChartPlot *plot, int index);
  void movePlot(int current, int index);
  void clearPlots();

signals:
  void plotsReset();
  void aboutToInsertPlots(int first, int last);
  void plotsInserted(int first, int last);
  void aboutToRemovePlots(int first, int last);
  void plotsRemoved(int first, int last);
  void plotMoved();
  void plotReset(const pqLineChartPlot *plot);
  void aboutToInsertPoints(const pqLineChartPlot *plot, int series, int first,
      int last);
  void pointsInserted(const pqLineChartPlot *plot, int series);
  void aboutToRemovePoints(const pqLineChartPlot *plot, int series, int first,
      int last);
  void pointsRemoved(const pqLineChartPlot *plot, int series);
  void aboutToChangeMultipleSeries(const pqLineChartPlot *plot);
  void changedMultipleSeries(const pqLineChartPlot *plot);

private slots:
  void handlePlotReset();
  void handlePlotBeginInsert(int series, int first, int last);
  void handlePlotEndInsert(int series);
  void handlePlotBeginRemove(int series, int first, int last);
  void handlePlotEndRemove(int series);
  void handlePlotBeginMultiSeriesChange();
  void handlePlotEndMultiSeriesChange();

private:
  pqLineChartModelInternal *Internal; ///< Stores the list of plots.
};

#endif
