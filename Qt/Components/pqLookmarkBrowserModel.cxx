/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkBrowserModel.cxx

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

/// \file pqLookmarkBrowserModel.cxx
/// \date 6/23/2006

#include "pqLookmarkBrowserModel.h"

#include <QList>
#include <QImage>
#include <QString>
#include <QtDebug>
#include <QPointer>

#include "pqApplicationCore.h"
#include "pqLookmarkModel.h"
#include "pqLookmarkManagerModel.h"

class pqLookmarkBrowserModelInternal : public QList<QPointer<pqLookmarkModel> >{};


pqLookmarkBrowserModel::pqLookmarkBrowserModel(const pqLookmarkManagerModel *model, QObject *parentObject)
  : QAbstractListModel(parentObject)
{
  this->Internal = new pqLookmarkBrowserModelInternal();

  // populate our contents based on model
  QList<pqLookmarkModel*> lookmarks = model->getAllLookmarks();
  QList<pqLookmarkModel*>::iterator iter;
  for(iter=lookmarks.begin(); iter!=lookmarks.end(); iter++)
    {
    this->addLookmark(*iter);
    }
}


pqLookmarkBrowserModel::~pqLookmarkBrowserModel()
{
  foreach (pqLookmarkModel* lookmark, *this->Internal)
    {
    if (lookmark)
      {
      delete lookmark;
      }
    }
  delete this->Internal;
}

int pqLookmarkBrowserModel::rowCount(const QModelIndex &parentIndex) const
{
  if(this->Internal && !parentIndex.isValid())
    {
    return this->Internal->size();
    }

  return 0;
}

QModelIndex pqLookmarkBrowserModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  if(this->Internal && !parentIndex.isValid() && column == 0 && row >= 0 &&
      row < this->Internal->size())
    {
    return this->createIndex(row, column, 0);
    }

  return QModelIndex();
}

QVariant pqLookmarkBrowserModel::data(const QModelIndex &idx,
    int role) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    pqLookmarkModel *lmk = (*this->Internal)[idx.row()];
    switch(role)
      {
      case Qt::DisplayRole:
      case Qt::EditRole:
        {
        return QVariant(lmk->getName());
        }
      case Qt::DecorationRole:
        {
        return QVariant(lmk->getIcon().scaled(48,48));
        }
      }
    }

  return QVariant();
}

Qt::ItemFlags pqLookmarkBrowserModel::flags(const QModelIndex &) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QString pqLookmarkBrowserModel::getNameFor(
    const QModelIndex &idx) const
{
  if(idx.isValid())
    {
    return (*this->Internal)[idx.row()]->getName();
    }
  return QString();
}

QModelIndex pqLookmarkBrowserModel::getIndexFor(
    const QString &lookmark) const
{
  if(this->Internal && !lookmark.isEmpty())
    {
    int row = 0;
    for( ; row < this->Internal->size(); row++)
      {
      QString compName = (*this->Internal)[row]->getName();
      if(QString::compare(lookmark, compName) == 0)
        {
        break;
        }
      }
    if(row != this->Internal->size())
      {
      return this->createIndex(row, 0, 0);
      }
    }

  return QModelIndex();
}


pqLookmarkModel* pqLookmarkBrowserModel::getLookmarkAtIndex(
    const QModelIndex &idx)
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    return (*this->Internal)[idx.row()];
    }
  return 0;
}

void pqLookmarkBrowserModel::addLookmark(pqLookmarkModel *lmk)
{
  if(!this->Internal || lmk->getName().isEmpty())
    {
    return;
    }

  int row = this->Internal->size();

  this->beginInsertRows(QModelIndex(), row, row);
  pqLookmarkModel *newLmk = new pqLookmarkModel(*lmk);
  this->Internal->insert(row, newLmk);
  this->endInsertRows();

  emit this->lookmarkAdded(lmk->getName());
}

void pqLookmarkBrowserModel::removeLookmark(const QModelIndex &idx)
{
  if(!this->Internal)
    {
    return;
    }

  QString lmkName;
  // Notify the view that the index is going away.
  this->beginRemoveRows(QModelIndex(), idx.row(), idx.row());
  pqLookmarkModel *lmk = (*this->Internal)[idx.row()];
  lmkName = lmk->getName();
  delete lmk;
  this->Internal->removeAt(idx.row());
  this->endRemoveRows();

  emit this->lookmarkRemoved(lmkName);
}


void pqLookmarkBrowserModel::removeLookmark(QString name)
{
  if(!this->Internal || name.isEmpty())
    {
    return;
    }

  // Find the row for the lookmark.
  int row = 0;
  for( ; row < this->Internal->size(); row++)
    {
    if(QString::compare(name, (*this->Internal)[row]->getName()) == 0)
      {
      break;
      }
    }
  if(row==this->Internal->size())
    {
    return;
    }

  pqLookmarkModel *lmk = (*this->Internal)[row];
  this->beginRemoveRows(QModelIndex(), row, row);
  delete lmk;
  this->Internal->removeAt(row);
  this->endRemoveRows();
  emit this->lookmarkRemoved(name);
}

void pqLookmarkBrowserModel::removeLookmarks(QModelIndexList &selection)
{
  QList<QModelIndex>::iterator iter;
  QList<QString> names;
  for(iter=selection.begin(); iter!=selection.end(); iter++)
    {
    names.push_back((*this->Internal)[(*iter).row()]->getName());
    }
  QList<QString>::iterator iter2;
  for(iter2=names.begin(); iter2!=names.end(); iter2++)
    {
    this->removeLookmark(*iter2);
    }
}


void pqLookmarkBrowserModel::exportLookmarks(const QModelIndexList &selection, const QStringList &files)
{
  QList<QModelIndex>::const_iterator iter;
  QList<pqLookmarkModel*> lookmarks;
  for(iter=selection.begin(); iter!=selection.end(); iter++)
    {
    lookmarks.push_back((*this->Internal)[(*iter).row()]);
    }
  emit this->exportLookmarks(lookmarks, files);
}


void pqLookmarkBrowserModel::onLookmarkModified(pqLookmarkModel *lmk)
{
  QModelIndex idx = this->getIndexFor(lmk->getName());
  emit this->dataChanged(idx,idx);
}

