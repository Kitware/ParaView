
#ifndef _pqChartContextMenu_h
#define _pqChartContextMenu_h


#include "QtWidgetsExport.h"
#include <QObject>

class QMenu;
class QWidget;


class QTWIDGETS_EXPORT pqChartContextMenu : public QObject
{
  Q_OBJECT

public:
  pqChartContextMenu(QObject *parent=0);
  virtual ~pqChartContextMenu() {}

  void addMenuActions(QMenu &menu, QWidget *chart) const;

protected slots:
  void printChart();
  void savePDF();
  void savePNG();
};

#endif
