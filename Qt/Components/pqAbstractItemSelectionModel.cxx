#include <QTreeWidgetItem>
#include <QDebug>

#include "pqAbstractItemSelectionModel.h"


pqAbstractItemSelectionModel::pqAbstractItemSelectionModel(QObject* parent)
: QAbstractItemModel(parent)
, RootItem(new QTreeWidgetItem())
{
};

pqAbstractItemSelectionModel::~pqAbstractItemSelectionModel()
{
  qDeleteAll(RootItem->takeChildren());
  delete RootItem;
};

int pqAbstractItemSelectionModel::rowCount(const QModelIndex & parent) const
{
  QTreeWidgetItem* parentItem = !parent.isValid() ? RootItem :
    static_cast<QTreeWidgetItem*>(parent.internalPointer());

  return parentItem->childCount();
};

int pqAbstractItemSelectionModel::columnCount(const QModelIndex & parent) const
{
  QTreeWidgetItem* parentItem = !parent.isValid() ? RootItem :
    static_cast<QTreeWidgetItem*>(parent.internalPointer());

  return parentItem->columnCount();
};

QModelIndex pqAbstractItemSelectionModel::index(int row, int column, const QModelIndex & parent) const
{
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  QTreeWidgetItem* parentItem = parent.isValid() ?
   static_cast<QTreeWidgetItem*>(parent.internalPointer()) : RootItem;

  QTreeWidgetItem* childItem = parentItem->child(row);

  return (childItem ? this->createIndex(row, column, childItem) : QModelIndex());
};

QModelIndex pqAbstractItemSelectionModel::parent(const QModelIndex & index) const
{
  if (!index.isValid())
    return QModelIndex();

  QTreeWidgetItem* childItem = static_cast<QTreeWidgetItem*>(index.internalPointer());
  QTreeWidgetItem* parentItem = childItem->parent();

  if (parentItem == RootItem)
    return QModelIndex();

  return QAbstractItemModel::createIndex(parentItem->indexOfChild(childItem), 0, parentItem);
};

QVariant pqAbstractItemSelectionModel::data(const QModelIndex & index, int role) const
{
  if (!this->isIndexValid(index))
    return QVariant();

  QTreeWidgetItem* item = RootItem->child(index.row());
  if (role == Qt::DisplayRole)
    {
    return item->data(index.column(), role);
    }
  else if (role == Qt::CheckStateRole && index.column() == 0)
    {
    return item->checkState(index.column());
    }
  else
    return QVariant();
};

bool pqAbstractItemSelectionModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (!this->isIndexValid(index))
      return false;

    QTreeWidgetItem* item = static_cast<QTreeWidgetItem*>(index.internalPointer());

    if (role == Qt::CheckStateRole) // index.column() == 0
      {
      item->setCheckState(0, static_cast<Qt::CheckState>(value.toInt()));
      emit QAbstractItemModel::headerDataChanged(Qt::Horizontal, index.column(), index.column());
      //emit QAbstractItemModel::dataChanged(index, index); // not needed since the change comes from the view
      return true;
      }

    return false;
}

QVariant pqAbstractItemSelectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
    return RootItem->data(section, role);
    }

  return QVariant();
};

Qt::ItemFlags pqAbstractItemSelectionModel::flags(const QModelIndex & index) const
{
  if(index.isValid() && index.column() == 0)
  {
    return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
  }

  return QAbstractItemModel::flags(index);
}

bool pqAbstractItemSelectionModel::isIndexValid(const QModelIndex & index) const
{
  if (!index.isValid())
    return false;

  QModelIndex const & parent = index.parent();
  if (index.row() >= this->rowCount(parent) || index.column() >= this->columnCount(parent))
    return false;

  return true;
}
