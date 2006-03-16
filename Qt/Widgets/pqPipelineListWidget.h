
/// \file pqPipelineListWidget.h
/// \brief
///   The pqPipelineListWidget class is used to display the pipeline
///   in the form of a tree.
///
/// \date 11/25/2005

#ifndef _pqPipelineListWidget_h
#define _pqPipelineListWidget_h

#include "QtWidgetsExport.h"
#include <QWidget>

class pqPipelineListModel;
class QModelIndex;
class QString;
class QTreeView;
class QVTKWidget;
class vtkSMProxy;


/// \class pqPipelineListWidget
/// \brief
///   The pqPipelineListWidget class is used to display the pipeline
///   in the form of a tree.
class QTWIDGETS_EXPORT pqPipelineListWidget : public QWidget
{
  Q_OBJECT

public:
  pqPipelineListWidget(QWidget *parent=0);
  virtual ~pqPipelineListWidget();

  virtual bool eventFilter(QObject *object, QEvent *e);

  pqPipelineListModel *getListModel() const {return this->ListModel;}
  QTreeView *getTreeView() const {return this->TreeView;}

  vtkSMProxy *getSelectedProxy() const;
  vtkSMProxy *getNextProxy() const; // TEMP
  QVTKWidget *getCurrentWindow() const;

signals:
  void proxySelected(vtkSMProxy *proxy);

public slots:
  void selectProxy(vtkSMProxy *proxy);
  void selectWindow(QVTKWidget *window);

  void deleteSelected();
  void deleteProxy(vtkSMProxy *proxy);

private slots:
  void changeCurrent(const QModelIndex &current, const QModelIndex &previous);

private:
  pqPipelineListModel *ListModel;
  QTreeView *TreeView;
};

#endif
