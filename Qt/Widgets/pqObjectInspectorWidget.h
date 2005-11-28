
/// \file pqObjectInspectorWidget.h
/// \brief
///   The pqObjectInspectorWidget class is used to display the properties
///   of an object in an editable list.
///
/// \date 11/25/2005

#ifndef _pqObjectInspectorWidget_h
#define _pqObjectInspectorWidget_h


#include <QWidget>

class pqObjectInspector;
class pqObjectInspectorDelegate;
class QTreeView;


class pqObjectInspectorWidget : public QWidget
{
public:
  pqObjectInspectorWidget(QWidget *parent=0);
  virtual ~pqObjectInspectorWidget();

  pqObjectInspector *getObjectModel() const {return this->Inspector;}
  QTreeView *getTreeView() const {return this->TreeView;}

private:
  pqObjectInspector *Inspector;
  pqObjectInspectorDelegate *Delegate;
  QTreeView *TreeView;
};

#endif
