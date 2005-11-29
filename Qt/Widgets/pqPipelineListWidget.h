
/// \file pqPipelineListWidget.h
/// \brief
///   The pqPipelineListWidget class is used to display the pipeline
///   in the form of a list.
///
/// \date 11/25/2005

#ifndef _pqPipelineListWidget_h
#define _pqPipelineListWidget_h

#include "QtWidgetsExport.h"
#include <QWidget>

class pqPipelineListModel;
class QTreeView;

class QTWIDGETS_EXPORT pqPipelineListWidget : public QWidget
{
public:
  pqPipelineListWidget(QWidget *parent=0);
  virtual ~pqPipelineListWidget();

  pqPipelineListModel *getListModel() const {return this->ListModel;}
  QTreeView *getTreeView() const {return this->TreeView;}

private:
  pqPipelineListModel *ListModel;
  QTreeView *TreeView;
};

#endif
