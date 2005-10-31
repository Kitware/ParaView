
#ifndef _pqObjectInspector_h
#define _pqObjectInspector_h


#include <QAbstractItemModel>

class pqObjectInspectorInternal;
class pqObjectInspectorItem;
class pqSMAdaptor;
class vtkSMProxy;


class pqObjectInspector : public QAbstractItemModel
{
  Q_OBJECT

public:
  pqObjectInspector(QObject *parent=0);
  virtual ~pqObjectInspector();

  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;
  virtual int columnCount(const QModelIndex &parent=QModelIndex()) const;
  virtual bool hasChildren(const QModelIndex &parent=QModelIndex()) const;

  virtual QModelIndex index(int row, int column,
      const QModelIndex &parent=QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &index) const;

  virtual QVariant data(const QModelIndex &index,
      int role=Qt::DisplayRole) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value,
      int role=Qt::EditRole);

  virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  void SetProxy(pqSMAdaptor *adapter, vtkSMProxy *proxy);

protected:
  void CleanData(bool notify=true);

private:
  int GetItemIndex(pqObjectInspectorItem *item) const;

private slots:
  void HandleNameChange(pqObjectInspectorItem *item);
  void HandleValueChange(pqObjectInspectorItem *item);

private:
  pqObjectInspectorInternal *Internal;
  pqSMAdaptor *Adapter;
  vtkSMProxy *Proxy;
};

#endif
