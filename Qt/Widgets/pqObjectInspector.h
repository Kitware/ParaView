
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
  enum CommitType {
    Individually,
    Collectively
  };

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

  QString getPropertyName(const QModelIndex &index) const;
  QVariant getDomain(const QModelIndex &index) const;

  void setProxy(pqSMAdaptor *adapter, vtkSMProxy *proxy);
  vtkSMProxy *getProxy() const {return this->Proxy;}
  pqSMAdaptor *getAdaptor() const {return this->Adapter;}

  CommitType getCommitType() const {return this->Commit;}

public slots:
  void setCommitType(CommitType commit);
  void commitChanges();

protected:
  void cleanData();

private:
  int getItemIndex(pqObjectInspectorItem *item) const;
  void commitChange(pqObjectInspectorItem *item);

private slots:
  void handleNameChange(pqObjectInspectorItem *item);
  void handleValueChange(pqObjectInspectorItem *item);

private:
  CommitType Commit;
  pqObjectInspectorInternal *Internal;
  pqSMAdaptor *Adapter;
  vtkSMProxy *Proxy;
};

#endif
