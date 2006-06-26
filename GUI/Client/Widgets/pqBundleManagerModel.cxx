/*=========================================================================

   Program: ParaView
   Module:    pqBundleManagerModel.cxx

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

/// \file pqBundleManagerModel.cxx
/// \date 6/23/2006

#include "pqBundleManagerModel.h"

#include <QList>
#include <QPixmap>
#include <QString>
#include <QtDebug>


class pqBundleManagerModelInternal : public QList<QString> {};


pqBundleManagerModel::pqBundleManagerModel(QObject *parentObject)
  : QAbstractListModel(parentObject)
{
  this->Internal = new pqBundleManagerModelInternal();
}

pqBundleManagerModel::~pqBundleManagerModel()
{
  delete this->Internal;
}

int pqBundleManagerModel::rowCount(const QModelIndex &parentIndex) const
{
  if(this->Internal && !parentIndex.isValid())
    {
    return this->Internal->size();
    }

  return 0;
}

QModelIndex pqBundleManagerModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  if(this->Internal && !parentIndex.isValid() && column == 0 && row >= 0 &&
      row < this->Internal->size())
    {
    return this->createIndex(row, column, 0);
    }

  return QModelIndex();
}

QVariant pqBundleManagerModel::data(const QModelIndex &idx, int role) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    switch(role)
      {
      case Qt::DisplayRole:
      case Qt::ToolTipRole:
      case Qt::EditRole:
        {
        return QVariant((*this->Internal)[idx.row()]);
        }
      case Qt::DecorationRole:
        {
        return QVariant(QPixmap(":/pqWidgets/pqBundle16.png"));
        }
      }
    }

  return QVariant();
}

Qt::ItemFlags pqBundleManagerModel::flags(const QModelIndex &) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QString pqBundleManagerModel::getBundleName(const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()];
    }

  return QString();
}

QModelIndex pqBundleManagerModel::getIndexFor(const QString &bundle) const
{
  if(this->Internal && !bundle.isEmpty())
    {
    int row = this->Internal->indexOf(bundle);
    if(row != -1)
      {
      return this->createIndex(row, 0, 0);
      }
    }

  return QModelIndex();
}

void pqBundleManagerModel::addBundle(QString name)
{
  if(!this->Internal || name.isEmpty())
    {
    return;
    }

  // Make sure the name is new.
  if(this->Internal->contains(name))
    {
    qDebug() << "Duplicate compound proxy definition added.";
    return;
    }

  // Insert the bundle in alphabetical order.
  int row = 0;
  for( ; row < this->Internal->size(); row++)
    {
    if(QString::compare(name, (*this->Internal)[row]) < 0)
      {
      break;
      }
    }

  this->beginInsertRows(QModelIndex(), row, row);
  this->Internal->insert(row, name);
  this->endInsertRows();

  emit this->bundleAdded(name);
}

void pqBundleManagerModel::removeBundle(QString name)
{
  if(!this->Internal || name.isEmpty())
    {
    return;
    }

  // Find the row for the bundle.
  int row = this->Internal->indexOf(name);
  if(row == -1)
    {
    qDebug() << "Compound proxy definition not found in the model.";
    return;
    }

  // Notify the view that the index is going away.
  this->beginRemoveRows(QModelIndex(), row, row);
  this->Internal->removeAt(row);
  this->endRemoveRows();
}


