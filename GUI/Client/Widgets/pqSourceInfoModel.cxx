
/// \file pqSourceInfoModel.cxx
/// \date 5/26/2006

#include "pqSourceInfoModel.h"

#include <QApplication>
#include <QList>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QtDebug>


class pqSourceInfoModelItem
{
public:
  pqSourceInfoModelItem(pqSourceInfoModelItem *parent=0);
  ~pqSourceInfoModelItem();

  pqSourceInfoModelItem *Parent;
  QList<pqSourceInfoModelItem *> Children;
  QString Name;
  bool IsFolder;
};


pqSourceInfoModelItem::pqSourceInfoModelItem(pqSourceInfoModelItem *parent)
  : Children(), Name()
{
  this->Parent = parent;
  this->IsFolder = false;
}

pqSourceInfoModelItem::~pqSourceInfoModelItem()
{
  QList<pqSourceInfoModelItem *>::Iterator iter = this->Children.begin();
  for( ; iter != this->Children.end(); ++iter)
    {
    delete *iter;
    }

  this->Children.clear();
}


pqSourceInfoModel::pqSourceInfoModel(const QStringList &sources,
    QObject *parentObject)
 : QAbstractItemModel(parentObject)
{
  this->Root = new pqSourceInfoModelItem();

  // Add the list of available sources to the root. These sources
  // will be used to filter sources added by the source info map.
  // Make a copy of the list in order to sort it.
  if(sources.size() > 0)
    {
    QStringList copy = sources;
    copy.sort();
    this->beginInsertRows(QModelIndex(), 0, copy.size() - 1);
    pqSourceInfoModelItem *item = 0;
    QStringList::Iterator iter = copy.begin();
    for( ; iter != copy.end(); ++iter)
      {
      item = new pqSourceInfoModelItem(this->Root);
      item->Name = *iter;
      item->IsFolder = false;
      this->Root->Children.append(item);
      }

    this->endInsertRows();
    }
}

pqSourceInfoModel::~pqSourceInfoModel()
{
  if(this->Root)
    {
    delete this->Root;
    }
}

int pqSourceInfoModel::rowCount(const QModelIndex &parentIndex) const
{
  pqSourceInfoModelItem *parentItem = this->getItemFor(parentIndex);
  if(parentItem)
    {
    return parentItem->Children.size();
    }

  return 0;
}

int pqSourceInfoModel::columnCount(const QModelIndex&) const
{
  return 1;
}

bool pqSourceInfoModel::hasChildren(const QModelIndex &parentIndex) const
{
  return this->rowCount(parentIndex) > 0;
}

QModelIndex pqSourceInfoModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  int rows = this->rowCount(parentIndex);
  int columns = this->columnCount(parentIndex);
  pqSourceInfoModelItem *parentItem = this->getItemFor(parentIndex);
  if(parentItem && row >= 0 && row < rows && column >= 0 && column < columns)
    {
    return this->createIndex(row, column, parentItem->Children[row]);
    }

  return QModelIndex();
}

QModelIndex pqSourceInfoModel::parent(const QModelIndex &idx) const
{
  pqSourceInfoModelItem *item = this->getItemFor(idx);
  if(item && item->Parent && item->Parent != this->Root)
    {
    int row = item->Parent->Parent->Children.indexOf(item->Parent);
    return this->createIndex(row, 0, item->Parent);
    }

  return QModelIndex();
}

QVariant pqSourceInfoModel::data(const QModelIndex &idx, int role) const
{
  pqSourceInfoModelItem *item = this->getItemFor(idx);
  if(item && item != this->Root)
    {
    switch(role)
      {
      case Qt::DisplayRole:
      case Qt::ToolTipRole:
      case Qt::EditRole:
        {
        return QVariant(item->Name);
        }
      case Qt::DecorationRole:
        {
        // TODO: Allow the user to specify a custom pixmap.
        if(item->IsFolder)
          {
          if(item->Parent == this->Root && item->Name == "Favorites")
            {
            return QVariant(QPixmap(":/pqWidgets/pqFavorites16.png"));
            }

          return QVariant(QPixmap(":/pqWidgets/pqFolder16.png"));
          }
        else
          {
          return QVariant(QPixmap(":/pqWidgets/pqFilter16.png"));
          }
        }
      case Qt::WhatsThisRole:
        {
        // TODO: Put the filter's description from the xml file as the
        // what's this tip.
        break;
        }
      }
    }

  return QVariant();
}

Qt::ItemFlags pqSourceInfoModel::flags(const QModelIndex &/*idx*/) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool pqSourceInfoModel::isSource(const QModelIndex &idx) const
{
  pqSourceInfoModelItem *item = this->getItemFor(idx);
  return item != 0 && item != this->Root && !item->IsFolder;
}

void pqSourceInfoModel::clearGroups()
{
  if(!this->Root)
    {
    return;
    }

  // Remove all the groups from the root. Leave the list of sources
  // on the root since they are the available list.
  int firstSource = 0;
  for( ; firstSource < this->Root->Children.size(); firstSource++)
    {
    if(!this->Root->Children[firstSource]->IsFolder)
      {
      break;
      }
    }

  if(firstSource > 0)
    {
    QList<pqSourceInfoModelItem *> toDelete;
    this->beginRemoveRows(QModelIndex(), 0, firstSource - 1);
    for(int i = firstSource - 1; i >= 0; i--)
      {
      toDelete.append(this->Root->Children.takeAt(i));
      }

    this->endRemoveRows();
    QList<pqSourceInfoModelItem *>::Iterator iter = toDelete.begin();
    for( ; iter != toDelete.end(); ++iter)
      {
      delete *iter;
      }
    }
}

void pqSourceInfoModel::addGroup(const QString &group)
{
  if(group.isEmpty())
    {
    qDebug() << "Unable to add empty group to the source info model.";
    return;
    }

  // Split the group path in order to get the parent group.
  QStringList paths = group.split("/", QString::SkipEmptyParts);
  QString groupName = paths.takeLast();
  pqSourceInfoModelItem *parentItem = this->Root;
  if(paths.size() > 0)
    {
    QString groupPath = paths.join("/");
    parentItem = this->getGroupItemFor(groupPath);
    }

  if(!parentItem)
    {
    qDebug() << "Group's parent path not found in the source info model.";
    return;
    }

  // Make sure the parent group does not already have the sub-group.
  if(this->isNameInItem(groupName, parentItem))
    {
    qDebug() << "Group already exists in source info model.";
    return;
    }

  // Create a new model item for the group.
  pqSourceInfoModelItem *groupItem = new pqSourceInfoModelItem(parentItem);
  if(groupItem)
    {
    groupItem->Name = groupName;
    groupItem->IsFolder = true;

    // Add the group to the parent in alphabetical order.
    this->addChildItem(groupItem);
    }
}

void pqSourceInfoModel::removeGroup(const QString &group)
{
  if(group.isEmpty())
    {
    qDebug() << "Unable to remove empty group from the source info model.";
    return;
    }

  pqSourceInfoModelItem *groupItem = this->getGroupItemFor(group);
  if(groupItem)
    {
    this->removeChildItem(groupItem);
    }
  else
    {
    qDebug() << "Specified group not found in the source info model.";
    }
}

void pqSourceInfoModel::addSource(const QString &name, const QString &group)
{
  if(name.isEmpty())
    {
    qDebug() << "Unable to add empty source to source info model.";
    return;
    }

  pqSourceInfoModelItem *parentItem = this->getGroupItemFor(group);
  if(!parentItem)
    {
    qDebug() << "Source's parent path not found in the source info model.";
    return;
    }

  // Make sure the parent group does not already have the source.
  if(this->isNameInItem(name, parentItem))
    {
    qDebug() << "Source already exists in the specified group.";
    return;
    }

  // Make sure the source is in the available list.
  if(!this->isNameInItem(name, this->Root))
    {
    return;
    }

  // Create a new model item for the source.
  pqSourceInfoModelItem *source = new pqSourceInfoModelItem(parentItem);
  if(source)
    {
    source->Name = name;
    source->IsFolder = false;

    // Add the group to the parent in alphabetical order.
    this->addChildItem(source);
    }
}

void pqSourceInfoModel::removeSource(const QString &name, const QString &group)
{
  if(name.isEmpty())
    {
    qDebug() << "Unable to remove empty source from source info model.";
    return;
    }

  pqSourceInfoModelItem *parentItem = this->getGroupItemFor(group);
  if(!parentItem)
    {
    qDebug() << "Source's parent path not found in the source info model.";
    return;
    }

  // Find the source in the parent item.
  pqSourceInfoModelItem *source = this->getChildItem(parentItem, name);
  if(source)
    {
    this->removeChildItem(source);
    }
  else
    {
    qDebug() << "Source not found in specified group.";
    }
}

pqSourceInfoModelItem *pqSourceInfoModel::getItemFor(
    const QModelIndex &idx) const
{
  if(!idx.isValid())
    {
    return this->Root;
    }
  else if(idx.model() == this)
    {
    return reinterpret_cast<pqSourceInfoModelItem *>(idx.internalPointer());
    }

  return 0;
}

pqSourceInfoModelItem *pqSourceInfoModel::getGroupItemFor(
    const QString &group) const
{
  if(group.isEmpty())
    {
    return this->Root;
    }

  pqSourceInfoModelItem *item = this->Root;
  QStringList paths = group.split("/", QString::SkipEmptyParts);
  QStringList::Iterator iter = paths.begin();
  for( ; item && iter != paths.end(); ++iter)
    {
    item = this->getChildItem(item, *iter);
    }

  return item;
}

pqSourceInfoModelItem *pqSourceInfoModel::getChildItem(
    pqSourceInfoModelItem *item, const QString &name) const
{
  QList<pqSourceInfoModelItem *>::Iterator iter = item->Children.begin();
  for( ; iter != item->Children.end(); ++iter)
    {
    if((*iter)->Name == name)
      {
      return *iter;
      }
    }

  return 0;
}

bool pqSourceInfoModel::isNameInItem(const QString &name,
    pqSourceInfoModelItem *item) const
{
  return this->getChildItem(item, name) != 0;
}

void pqSourceInfoModel::addChildItem(pqSourceInfoModelItem *item)
{
  int row = 0;
  QModelIndex parentIndex;
  if(item->Parent != this->Root)
    {
    row = item->Parent->Parent->Children.indexOf(item->Parent);
    parentIndex = this->createIndex(row, 0, item->Parent);
    }

  // Find the correct insertion location. Groups should be before sources.
  // Set up the index limits based on the type of child added.
  int total = item->Parent->Children.size();
  int firstSource = 0;
  for( ; firstSource < total; firstSource++)
    {
    if(!item->Parent->Children[firstSource]->IsFolder)
      {
      break;
      }
    }

  row = 0;
  if(item->IsFolder)
    {
    total = firstSource;
    }
  else
    {
    row = firstSource;
    }

  for( ; row < total; row++)
    {
    if(QString::compare(item->Name, item->Parent->Children[row]->Name) < 0)
      {
      break;
      }
    }

  this->beginInsertRows(parentIndex, row, row);
  item->Parent->Children.insert(row, item);
  this->endInsertRows();
}

void pqSourceInfoModel::removeChildItem(pqSourceInfoModelItem *item)
{
  int row = 0;
  QModelIndex parentIndex;
  if(item->Parent != this->Root)
    {
    row = item->Parent->Parent->Children.indexOf(item->Parent);
    parentIndex = this->createIndex(row, 0, item->Parent);
    }

  row = item->Parent->Children.indexOf(item);
  this->beginRemoveRows(parentIndex, row, row);
  item->Parent->Children.removeAt(row);
  this->endRemoveRows();
  delete item;
}



