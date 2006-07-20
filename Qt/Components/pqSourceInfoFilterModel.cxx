/*=========================================================================

   Program: ParaView
   Module:    pqSourceInfoFilterModel.cxx

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

/// \file pqSourceInfoFilterModel.cxx
/// \date 6/26/2006

#include "pqSourceInfoFilterModel.h"

#include "pqSourceInfoModel.h"

#include <QList>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QString>
#include <QStringList>


/// \class pqSourceInfoFilterModelItem
class pqSourceInfoFilterModelItem
{
public:
  pqSourceInfoFilterModelItem(pqSourceInfoFilterModelItem *parent=0);
  ~pqSourceInfoFilterModelItem();

  QPersistentModelIndex SourceIndex;
  pqSourceInfoFilterModelItem *Parent;
  QList<pqSourceInfoFilterModelItem *> Children;
};


/// \class pqSourceInfoFilterModelInternal
class pqSourceInfoFilterModelInternal
{
public:
  pqSourceInfoFilterModelInternal();
  ~pqSourceInfoFilterModelInternal() {}

  QStringList Allowed;
  QList<pqSourceInfoFilterModelItem *> ToRemove;
};


//-----------------------------------------------------------------------------
pqSourceInfoFilterModelItem::pqSourceInfoFilterModelItem(
    pqSourceInfoFilterModelItem *parent)
  : SourceIndex(), Children()
{
  this->Parent = parent;
}

pqSourceInfoFilterModelItem::~pqSourceInfoFilterModelItem()
{
  QList<pqSourceInfoFilterModelItem *>::Iterator iter = this->Children.begin();
  for( ; iter != this->Children.end(); ++iter)
    {
    delete *iter;
    }

  this->Children.clear();
}


//-----------------------------------------------------------------------------
pqSourceInfoFilterModelInternal::pqSourceInfoFilterModelInternal()
  : Allowed(), ToRemove()
{
}


//-----------------------------------------------------------------------------
pqSourceInfoFilterModel::pqSourceInfoFilterModel(QObject *parentObject)
  : QAbstractProxyModel(parentObject)
{
  this->Internal = new pqSourceInfoFilterModelInternal();
  this->Root = new pqSourceInfoFilterModelItem();
  this->SourceInfo = 0;
}

pqSourceInfoFilterModel::~pqSourceInfoFilterModel()
{
  delete this->Internal;
  delete this->Root;
}

int pqSourceInfoFilterModel::rowCount(const QModelIndex &parentIndex) const
{
  pqSourceInfoFilterModelItem *item = this->getModelItem(parentIndex);
  if(item && (item == this->Root || parentIndex.column() == 0))
    {
    return item->Children.size();
    }

  return 0;
}

int pqSourceInfoFilterModel::columnCount(const QModelIndex &parentIndex) const
{
  if(this->sourceModel())
    {
    QModelIndex sourceIndex = this->mapToSource(parentIndex);
    return this->sourceModel()->columnCount(sourceIndex);
    }

  return 0;
}

bool pqSourceInfoFilterModel::hasChildren(const QModelIndex &parentIndex) const
{
  return this->rowCount(parentIndex) > 0;
}

QModelIndex pqSourceInfoFilterModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  pqSourceInfoFilterModelItem *item = this->getModelItem(parentIndex);
  if(item && column >= 0 && row >= 0 && row < item->Children.size())
    {
    pqSourceInfoFilterModelItem *child = item->Children[row];
    return this->createIndex(row, column, child);
    }

  return QModelIndex();
}

QModelIndex pqSourceInfoFilterModel::parent(const QModelIndex &idx) const
{
  pqSourceInfoFilterModelItem *item = this->getModelItem(idx);
  if(item && item->Parent && item->Parent != this->Root)
    {
    int row = item->Parent->Parent->Children.indexOf(item->Parent);
    return this->createIndex(row, 0, item->Parent);
    }

  return QModelIndex();
}

QVariant pqSourceInfoFilterModel::data(const QModelIndex &idx, int role) const
{
  if(this->sourceModel())
    {
    QModelIndex sourceIndex = this->mapToSource(idx);
    return this->sourceModel()->data(sourceIndex, role);
    }

  return QVariant();
}

Qt::ItemFlags pqSourceInfoFilterModel::flags(const QModelIndex &idx) const
{
  if(this->sourceModel())
    {
    QModelIndex sourceIndex = this->mapToSource(idx);
    return this->sourceModel()->flags(sourceIndex);
    }

  return Qt::ItemIsEnabled;
}

void pqSourceInfoFilterModel::setSourceModel(QAbstractItemModel *source)
{
  QAbstractProxyModel::setSourceModel(source);
  this->SourceInfo = qobject_cast<pqSourceInfoModel *>(source);

  // Listen for model changes.
  QObject::connect(source, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
      this, SLOT(addModelRows(const QModelIndex &, int, int)));
  QObject::connect(source,
      SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
      this, SLOT(startRemovingRows(const QModelIndex &, int, int)));
  QObject::connect(source, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
      this, SLOT(finishRemovingRows(const QModelIndex &, int, int)));
  QObject::connect(source, SIGNAL(modelReset()),
      this, SLOT(handleSourceReset()));

  // Clean up the data for the old model. Fill in the data for the new model.
  this->clearData();
  this->loadData();

  // Inform the view that the model has changed.
  this->reset();
}

QModelIndex pqSourceInfoFilterModel::mapFromSource(
    const QModelIndex &sourceIndex) const
{
  pqSourceInfoFilterModelItem *item = this->getModelItemFromSource(sourceIndex);
  if(item && item != this->Root)
    {
    int row = item->Parent->Children.indexOf(item);
    return this->createIndex(row, sourceIndex.column(), item);
    }

  return QModelIndex();
}

QModelIndex pqSourceInfoFilterModel::mapToSource(
    const QModelIndex &proxyIndex) const
{
  pqSourceInfoFilterModelItem *item = this->getModelItem(proxyIndex);
  if(item && item != this->Root)
    {
    return item->SourceIndex.sibling(item->SourceIndex.row(),
        proxyIndex.column());
    }

  return QModelIndex();
}

void pqSourceInfoFilterModel::setAllowedNames(const QStringList &allowed)
{
  // Clean up the current model data.
  this->clearData();

  // Load the model data using the new list of allowed names.
  this->Internal->Allowed = allowed;
  this->loadData();

  // Notify the view that the model has changed.
  this->reset();
}

void pqSourceInfoFilterModel::addModelRows(const QModelIndex &sourceIndex,
    int start, int end)
{
  pqSourceInfoFilterModelItem *item =
      this->getModelItemFromSource(sourceIndex);
  if(item)
    {
    // Find the location to add the new rows.
    int row = 0;
    QList<pqSourceInfoFilterModelItem *>::Iterator iter =
        item->Children.begin();
    for( ; iter != item->Children.end(); ++iter, ++row)
      {
      if(start <= (*iter)->SourceIndex.row())
        {
        break;
        }
      }

    // Use the allowed list to determine if the rows can be added.
    QString name;
    QModelIndex idx;
    pqSourceInfoFilterModelItem *child = 0;
    QList<pqSourceInfoFilterModelItem *> toAdd;
    QAbstractItemModel *source = this->sourceModel();
    for(int i = start; i <= end; i++)
      {
      idx = source->index(i, 0, sourceIndex);
      if(this->SourceInfo && !this->SourceInfo->isSource(idx))
        {
        // Folders always pass the filter.
        child = new pqSourceInfoFilterModelItem(item);
        }
      else
        {
        name = source->data(idx, Qt::DisplayRole).toString();
        if(this->Internal->Allowed.contains(name))
          {
          child = new pqSourceInfoFilterModelItem(item);
          }
        }

      if(child)
        {
        toAdd.append(child);
        child->SourceIndex = idx;
        this->loadData(source, idx, child);
        }
      }

    // Add the rows that passed the filter to the model.
    if(toAdd.size() > 0)
      {
      this->beginInsertRows(this->mapFromSource(sourceIndex), row,
          row + toAdd.size() - 1);
      for(iter = toAdd.begin(); iter != toAdd.end(); ++iter, ++row)
        {
        item->Children.insert(row, *iter);
        }

      this->endInsertRows();
      }
    }
}

void pqSourceInfoFilterModel::startRemovingRows(const QModelIndex &sourceIndex,
    int start, int end)
{
  // Remove the associated proxy indexes. Place them on a list to be
  // deleted after the rows have been removed.
  pqSourceInfoFilterModelItem *item =
      this->getModelItemFromSource(sourceIndex);
  if(item)
    {
    // Find the location to remove the rows.
    int row = 0;
    QList<pqSourceInfoFilterModelItem *>::Iterator iter =
        item->Children.begin();
    for( ; iter != item->Children.end(); ++iter, ++row)
      {
      if(start <= (*iter)->SourceIndex.row())
        {
        break;
        }
      }

    // There's nothing to remove if the start is not in the list.
    if(row > item->Children.size())
      {
      return;
      }

    // Find the end in the filtered list.
    int endRow = row;
    for( ; iter != item->Children.end(); ++iter, ++endRow)
      {
      if((*iter)->SourceIndex.row() > end)
        {
        break;
        }
      }

    // The endRow will always be one more than the actual endRow.
    endRow--;

    // Notify the view that the model is changing.
    this->beginRemoveRows(this->mapFromSource(sourceIndex), row, endRow);
    for(int i = endRow; i >= row; i--)
      {
      // Remove the item from the list of children and save the item
      // for clean up later.
      this->Internal->ToRemove.prepend(item->Children.takeAt(i));
      }
    }
}

void pqSourceInfoFilterModel::finishRemovingRows(
    const QModelIndex &/*sourceIndex*/, int /*start*/, int /*end*/)
{
  // Signal the view that the removal is complete. Then, delete the
  // proxy items on the list.
  if(this->Internal->ToRemove.size() > 0)
    {
    this->endRemoveRows();
    QList<pqSourceInfoFilterModelItem *>::Iterator iter =
        this->Internal->ToRemove.begin();
    for( ; iter != this->Internal->ToRemove.end(); ++iter)
      {
      delete *iter;
      }

    this->Internal->ToRemove.clear();
    }
}

void pqSourceInfoFilterModel::handleSourceReset()
{
  // Clean up the the current model data and reload the new data.
  this->clearData();
  this->loadData();

  // Notify the view that the model has changed.
  this->reset();
}

pqSourceInfoFilterModelItem *pqSourceInfoFilterModel::getModelItem(
    const QModelIndex &proxyIndex) const
{
  if(!proxyIndex.isValid())
    {
    return this->Root;
    }
  else if(proxyIndex.model() == this)
    {
    return reinterpret_cast<pqSourceInfoFilterModelItem *>(
        proxyIndex.internalPointer());
    }

  return 0;
}

pqSourceInfoFilterModelItem *pqSourceInfoFilterModel::getModelItemFromSource(
    const QModelIndex &sourceIndex) const
{
  if(!sourceIndex.isValid())
    {
    return this->Root;
    }

  QModelIndex idx = sourceIndex;
  if(idx.column() != 0)
    {
    idx = idx.sibling(idx.row(), 0);
    }

  // Get the parent chain.
  QList<QModelIndex> chain;
  while(idx.isValid())
    {
    chain.prepend(idx);
    idx = idx.parent();
    }

  // Start with the proxy model root and find the index at the end of
  // the source model chain.
  int row = 0;
  pqSourceInfoFilterModelItem *child = 0;
  pqSourceInfoFilterModelItem *item = this->Root;
  QList<QModelIndex>::Iterator iter = chain.begin();
  for( ; item && iter != chain.end(); ++iter)
    {
    child = 0;
    for(row = (*iter).row(); row >= 0; row--)
      {
      child = item->Children[row];
      if(child->SourceIndex == *iter)
        {
        break;
        }
      }

    if(row < 0)
      {
      item = 0;
      break;
      }

    item = child;
    }

  return item;
}

void pqSourceInfoFilterModel::clearData()
{
  QList<pqSourceInfoFilterModelItem *>::Iterator iter =
      this->Root->Children.begin();
  for( ; iter != this->Root->Children.end(); ++iter)
    {
    delete *iter;
    }

  this->Root->Children.clear();
}

void pqSourceInfoFilterModel::loadData()
{
  // Leave the model empty if the allowed names is empty.
  QAbstractItemModel *source = this->sourceModel();
  if(this->Internal->Allowed.size() == 0 || source == 0)
    {
    return;
    }

  // Start with the root and load all the data recursively.
  this->loadData(source, QModelIndex(), this->Root);
}

void pqSourceInfoFilterModel::loadData(QAbstractItemModel *source,
    const QModelIndex &sourceIndex, pqSourceInfoFilterModelItem *item)
{
  // Filter the items using the list of allowed names.
  QString name;
  QModelIndex idx;
  pqSourceInfoFilterModelItem *child = 0;
  int count = source->rowCount(sourceIndex);
  for(int row = 0; row < count; row ++)
    {
    child = 0;
    idx = source->index(row, 0, sourceIndex);
    if(this->SourceInfo && !this->SourceInfo->isSource(idx))
      {
      // Folders always pass the filter.
      child = new pqSourceInfoFilterModelItem(item);
      }
    else
      {
      name = source->data(idx, Qt::DisplayRole).toString();
      if(this->Internal->Allowed.contains(name))
        {
        child = new pqSourceInfoFilterModelItem(item);
        }
      }

    if(child)
      {
      item->Children.append(child);
      child->SourceIndex = idx;
      this->loadData(source, idx, child);
      }
    }
}


