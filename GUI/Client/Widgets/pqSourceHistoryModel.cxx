/*=========================================================================

   Program: ParaView
   Module:    pqSourceHistoryModel.cxx

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

/// \file pqSourceHistoryModel.cxx
/// \date 5/26/2006

#include "pqSourceHistoryModel.h"

#include <QList>
#include <QString>
#include <QStringList>


class pqSourceHistoryModelInternal : public QList<QString> {};


pqSourceHistoryModel::pqSourceHistoryModel(QObject *parentObject)
  : QAbstractListModel(parentObject)
{
  this->Internal = new pqSourceHistoryModelInternal();
  this->Icons = 0;
  this->Pixmap = pqSourceInfoIcons::Invalid;
  this->Limit = 20;
}

pqSourceHistoryModel::~pqSourceHistoryModel()
{
  if(this->Internal)
    {
    delete this->Internal;
    }
}

int pqSourceHistoryModel::rowCount(const QModelIndex &parentIndex) const
{
  if(this->Internal && !parentIndex.isValid())
    {
    return this->Internal->size();
    }

  return 0;
}

QModelIndex pqSourceHistoryModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  if(this->Internal && !parentIndex.isValid() && column == 0 && row >= 0 &&
      row < this->Internal->size())
    {
    return this->createIndex(row, column, 0);
    }

  return QModelIndex();
}

QVariant pqSourceHistoryModel::data(const QModelIndex &idx, int role) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    QString itemName = (*this->Internal)[idx.row()];
    switch(role)
      {
      case Qt::DisplayRole:
      case Qt::ToolTipRole:
      case Qt::EditRole:
        {
        return QVariant(itemName);
        }
      case Qt::DecorationRole:
        {
        if(this->Icons)
          {
          // Get the user specified icon.
          return QVariant(this->Icons->getPixmap(itemName, this->Pixmap));
          }
        else
          {
          // Default to the source pixmap.
          return QVariant(QPixmap(":/pqWidgets/pqSource16.png"));
          }
        }
      }
    }

  return QVariant();
}

Qt::ItemFlags pqSourceHistoryModel::flags(const QModelIndex&) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QString pqSourceHistoryModel::getSourceName(const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()];
    }

  return QString();
}

QModelIndex pqSourceHistoryModel::getIndexFor(const QString&) const
{
  return QModelIndex();
}

void pqSourceHistoryModel::setHistoryLimit(int limit)
{
  if(this->Internal && limit > 0 && limit != this->Limit)
    {
    this->Limit = limit;

    // If there are too many items on the list, prune it.
    if(this->Internal->size() > this->Limit)
      {
      this->beginRemoveRows(QModelIndex(), this->Limit,
          this->Internal->size());
      QList<QString>::Iterator iter = this->Internal->begin();
      iter += this->Limit;
      this->Internal->erase(iter, this->Internal->end());
      this->endRemoveRows();
      }
    }
}

void pqSourceHistoryModel::getHistoryList(QStringList &list) const
{
  if(this->Internal)
    {
    QList<QString>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      list.append(*iter);
      }
    }
}

void pqSourceHistoryModel::setHistoryList(const QStringList &list)
{
  if(this->Internal)
    {
    this->Internal->clear();
    QStringList::ConstIterator iter = list.begin();
    for(int i = 0; iter != list.end() && i < this->Limit; ++iter, ++i)
      {
      this->Internal->append(*iter);
      }

    // Signal the view that everything has changed.
    this->reset();
    }
}

void pqSourceHistoryModel::addRecentSource(const QString &source)
{
  if(this->Internal && !source.isEmpty())
    {
    // See if the source is in the history list.
    int row = this->Internal->indexOf(source);
    if(row != -1)
      {
      // Remove the source from the list.
      this->beginRemoveRows(QModelIndex(), row, row);
      this->Internal->removeAt(row);
      this->endRemoveRows();
      }

    // Add the source to the front of the list.
    this->beginInsertRows(QModelIndex(), 0, 0);
    this->Internal->prepend(source);
    this->endInsertRows();

    // Make sure the list stays within its limit.
    if(this->Internal->size() > this->Limit)
      {
      row = this->Internal->size() - 1;
      this->beginRemoveRows(QModelIndex(), row, row);
      this->Internal->removeAt(row);
      this->endRemoveRows();
      }
    }
}

void pqSourceHistoryModel::setIcons(pqSourceInfoIcons *icons,
    pqSourceInfoIcons::DefaultPixmap type)
{
  this->Icons = icons;
  this->Pixmap = type;

  // Listen for pixmap updates.
  QObject::connect(this->Icons, SIGNAL(pixmapChanged(const QString &)),
      this, SLOT(updatePixmap(const QString &)));
}

void pqSourceHistoryModel::updatePixmap(const QString &name)
{
  QModelIndex idx = this->getIndexFor(name);
  if(idx.isValid())
    {
    emit this->dataChanged(idx, idx);
    }
}


