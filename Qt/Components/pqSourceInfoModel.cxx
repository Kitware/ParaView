/*=========================================================================

   Program: ParaView
   Module:    pqSourceInfoModel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

/// \file pqSourceInfoModel.cxx
/// \date 5/26/2006

#include "pqSourceInfoModel.h"

#include <QApplication>
#include <QList>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QtDebug>


/// \class pqSourceInfoModelItem
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


//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
pqSourceInfoModel::pqSourceInfoModel(const QStringList &sources,
    QObject *parentObject)
 : QAbstractItemModel(parentObject)
{
  this->Root = new pqSourceInfoModelItem();
  this->Icons = 0;
  this->Pixmap = pqSourceInfoIcons::Invalid;

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
        if(item->IsFolder)
          {
          if(item->Parent == this->Root && item->Name == "Favorites")
            {
            return QVariant(QPixmap(":/pqWidgets/Icons/pqFavorites16.png"));
            }

          return QVariant(QPixmap(":/pqWidgets/Icons/pqFolder16.png"));
          }
        else if(this->Icons)
          {
          // Get the user specified icon.
          return QVariant(this->Icons->getPixmap(item->Name, this->Pixmap));
          }
        else
          {
          // Default to the source pixmap.
          return QVariant(QPixmap(":/pqCore/Icons/pqSource16.png"));
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

bool pqSourceInfoModel::isSource(const QString &name) const
{
  // Look in the root's list of sources for the name.
  if(!name.isEmpty())
    {
    pqSourceInfoModelItem *item = this->getChildItem(this->Root, name);
    return item != 0 && !item->IsFolder;
    }

  return false;
}

void pqSourceInfoModel::getGroup(const QModelIndex &_index,
    QString &group) const
{
  pqSourceInfoModelItem *item = this->getItemFor(_index);
  if(item && item != this->Root)
    {
    QStringList path;
    if(item->IsFolder)
      {
      path.append(item->Name);
      }

    pqSourceInfoModelItem *item_parent = item->Parent;
    while(item_parent && item_parent != this->Root)
      {
      path.prepend(item_parent->Name);
      item_parent = item_parent->Parent;
      }

    group = path.join("/");
    }
}

void pqSourceInfoModel::setIcons(pqSourceInfoIcons *icons,
    pqSourceInfoIcons::DefaultPixmap type)
{
  this->Icons = icons;
  this->Pixmap = type;

  // Listen for pixmap updates.
  QObject::connect(this->Icons, SIGNAL(pixmapChanged(const QString &)),
      this, SLOT(updatePixmap(const QString &)));
}

void pqSourceInfoModel::getAvailableSources(QStringList &list) const
{
  if(this->Root)
    {
    QList<pqSourceInfoModelItem *>::ConstIterator iter =
        this->Root->Children.begin();
    for( ; iter != this->Root->Children.end(); ++iter)
      {
      if(!(*iter)->IsFolder)
        {
        list.append((*iter)->Name);
        }
      }
    }
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

void pqSourceInfoModel::updatePixmap(const QString &name)
{
  // Signal the view to refresh the icon for the specified source.
  // The source can be in multiple places.
  QModelIndex idx;
  pqSourceInfoModelItem *item = this->getNextItem(this->Root);
  while(item)
    {
    if(!item->IsFolder && item->Name == name)
      {
      idx = this->getIndexFor(item);
      emit this->dataChanged(idx, idx);
      }

    item = this->getNextItem(item);
    }
}

QModelIndex pqSourceInfoModel::getIndexFor(pqSourceInfoModelItem *item) const
{
  if(item->Parent)
    {
    int row = item->Parent->Children.indexOf(item);
    return this->createIndex(row, 0, item);
    }

  return QModelIndex();
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

pqSourceInfoModelItem *pqSourceInfoModel::getNextItem(
    pqSourceInfoModelItem *item) const
{
  if(item->Children.size() > 0)
    {
    return item->Children.first();
    }

  // Search up the ancestors for an item with multiple children.
  // The next item will be the next child.
  int row = 0;
  int count = 0;
  while(item->Parent)
    {
    count = item->Parent->Children.size();
    if(count > 1)
      {
      row = item->Parent->Children.indexOf(item) + 1;
      if(row < count)
        {
        return item->Parent->Children[row];
        }
      }

    item = item->Parent;
    }

  return 0;
}



