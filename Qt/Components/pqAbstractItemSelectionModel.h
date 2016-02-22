#include <QAbstractItemModel>


class QTreeWidgetItem;
class vtkSMProxy;

/// @brief Abstract class for simple a simple model with checkable
/// items (QTreeWidgetItem).
class pqAbstractItemSelectionModel : public QAbstractItemModel
{
  Q_OBJECT

public:

  pqAbstractItemSelectionModel(QObject* parent = NULL);
  ~pqAbstractItemSelectionModel();

  int rowCount(const QModelIndex & parent = QModelIndex()) const;
  int columnCount(const QModelIndex & parent = QModelIndex()) const;
 
  QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex & index) const;
  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex & index, const QVariant & value, int role);
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;

  void toggleCheckState(const QModelIndex & index);
  virtual void populateModel(void* proxy) = 0;

protected:
  
  virtual void initializeRootItem() = 0;

  bool isIndexValid(const QModelIndex & index) const;
  
  /////////////////////////////////////////////////////////////////////////////
  
  QTreeWidgetItem* RootItem;
};
