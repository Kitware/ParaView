
#ifndef _pqObjectInspectorDelegate_h
#define _pqObjectInspectorDelegate_h


#include <QItemDelegate>


class pqObjectInspectorDelegate : public QItemDelegate
{
public:
  pqObjectInspectorDelegate(QObject *parent=0);
  virtual ~pqObjectInspectorDelegate();

  virtual QWidget *createEditor(QWidget *parent,
      const QStyleOptionViewItem &option, const QModelIndex &index) const;
  virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
  virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
      const QModelIndex &index) const;
};

#endif
