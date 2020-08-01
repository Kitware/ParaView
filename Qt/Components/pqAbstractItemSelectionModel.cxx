/*=========================================================================

   Program: ParaView
   Module:  pqAbstractItemSelectionModel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
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
#include <QDebug>
#include <QTreeWidgetItem>

#include "pqAbstractItemSelectionModel.h"

// -----------------------------------------------------------------------------
pqAbstractItemSelectionModel::pqAbstractItemSelectionModel(QObject* parent_)
  : QAbstractItemModel(parent_)
  , RootItem(new QTreeWidgetItem())
{
}

// -----------------------------------------------------------------------------
pqAbstractItemSelectionModel::~pqAbstractItemSelectionModel()
{
  qDeleteAll(RootItem->takeChildren());
  delete RootItem;
}

// -----------------------------------------------------------------------------
int pqAbstractItemSelectionModel::rowCount(const QModelIndex& parent_) const
{
  QTreeWidgetItem* parentItem =
    !parent_.isValid() ? RootItem : static_cast<QTreeWidgetItem*>(parent_.internalPointer());

  return parentItem->childCount();
}

// -----------------------------------------------------------------------------
int pqAbstractItemSelectionModel::columnCount(const QModelIndex& parent_) const
{
  QTreeWidgetItem* parentItem =
    !parent_.isValid() ? RootItem : static_cast<QTreeWidgetItem*>(parent_.internalPointer());

  return parentItem->columnCount();
}

// -----------------------------------------------------------------------------
QModelIndex pqAbstractItemSelectionModel::index(
  int row, int column, const QModelIndex& parent_) const
{
  if (!hasIndex(row, column, parent_))
  {
    return QModelIndex();
  }

  QTreeWidgetItem* parentItem =
    parent_.isValid() ? static_cast<QTreeWidgetItem*>(parent_.internalPointer()) : RootItem;

  QTreeWidgetItem* childItem = parentItem->child(row);

  return (childItem ? this->createIndex(row, column, childItem) : QModelIndex());
}

// -----------------------------------------------------------------------------
QModelIndex pqAbstractItemSelectionModel::parent(const QModelIndex& index_) const
{
  if (!index_.isValid())
  {
    return QModelIndex();
  }

  QTreeWidgetItem* childItem = static_cast<QTreeWidgetItem*>(index_.internalPointer());
  QTreeWidgetItem* parentItem = childItem->parent();

  if (parentItem == RootItem)
  {
    return QModelIndex();
  }

  return QAbstractItemModel::createIndex(parentItem->indexOfChild(childItem), 0, parentItem);
}

// -----------------------------------------------------------------------------
QVariant pqAbstractItemSelectionModel::data(const QModelIndex& index_, int role) const
{
  if (!this->isIndexValid(index_))
    return QVariant();

  QModelIndex parentIndex = index_.parent();
  QTreeWidgetItem* parent_ = (parentIndex.isValid())
    ? static_cast<QTreeWidgetItem*>(parentIndex.internalPointer())
    : this->RootItem;

  QTreeWidgetItem* item = parent_->child(index_.row());
  if (role == Qt::DisplayRole)
  {
    return item->data(index_.column(), role);
  }
  else if (role == Qt::CheckStateRole && index_.column() == 0)
  {
    return item->checkState(index_.column());
  }
  else
  {
    return QVariant();
  }
}

// -----------------------------------------------------------------------------
bool pqAbstractItemSelectionModel::setData(
  const QModelIndex& index_, const QVariant& value, int role)
{
  if (!this->isIndexValid(index_))
  {
    return false;
  }

  QTreeWidgetItem* item = static_cast<QTreeWidgetItem*>(index_.internalPointer());

  if (role == Qt::CheckStateRole)
  {
    item->setCheckState(0, static_cast<Qt::CheckState>(value.toInt()));
    Q_EMIT QAbstractItemModel::headerDataChanged(Qt::Horizontal, index_.column(), index_.column());

    // No need to Q_EMIT QAbstractItemModel::dataChanged(idx, idx) since in this case
    // the change comes from the view and it handles it directly.

    return true;
  }

  return false;
}

// -----------------------------------------------------------------------------
QVariant pqAbstractItemSelectionModel::headerData(
  int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    return RootItem->data(section, role);
  }

  return QVariant();
}

// -----------------------------------------------------------------------------
Qt::ItemFlags pqAbstractItemSelectionModel::flags(const QModelIndex& index_) const
{
  if (index_.isValid() && index_.column() == 0)
  {
    return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
  }

  return QAbstractItemModel::flags(index_);
}

// -----------------------------------------------------------------------------
bool pqAbstractItemSelectionModel::isIndexValid(const QModelIndex& index_) const
{
  if (!index_.isValid())
  {
    return false;
  }

  QModelIndex const& parent_ = index_.parent();
  if (index_.row() >= this->rowCount(parent_) || index_.column() >= this->columnCount(parent_))
  {
    return false;
  }

  return true;
}
