/*=========================================================================

   Program: ParaView
   Module:    pqColorTableModel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

/// \file pqColorTableModel.cxx
/// \date 8/7/2006

#include "pqColorTableModel.h"

#include <QColor>
#include <QVector>


class pqColorTableModelInternal : public QVector<QColor> {};


pqColorTableModel::pqColorTableModel(QObject *parentObject)
  : QAbstractListModel(parentObject)
{
  this->Internal = new pqColorTableModelInternal();
}

pqColorTableModel::~pqColorTableModel()
{
  delete this->Internal;
}

int pqColorTableModel::rowCount(const QModelIndex &) const
{
  return this->Internal->size();
}

QModelIndex pqColorTableModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  if(column == 0 && row >= 0 && row < this->rowCount() &&
      !parentIndex.isValid())
    {
    return this->createIndex(row, column);
    }

  return QModelIndex();
}

QVariant pqColorTableModel::data(const QModelIndex &idx, int role) const
{
  if(idx.isValid() && idx.model() == this && role == Qt::DisplayRole)
    {
    return QVariant(this->Internal->at(idx.row()));
    }

  return QVariant();
}

Qt::ItemFlags pqColorTableModel::flags(const QModelIndex &) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void pqColorTableModel::setTableSize(int tableSize)
{
  int rows = this->rowCount();
  if(rows == tableSize)
    {
    return;
    }

  // Resize the color table to fit the new size.
  if(tableSize < rows)
    {
    // Truncate the current table to be the correct size.
    this->beginRemoveRows(QModelIndex(), tableSize, rows - 1);
    this->Internal->resize(tableSize);
    this->endRemoveRows();
    }
  else
    {
    // Add new colors to the end of the table. Use the last color to
    // fill the new spaces.
    QColor color = rows > 0 ? this->Internal->last() : QColor(255, 0, 0);
    this->Internal->reserve(tableSize);
    this->beginInsertRows(QModelIndex(), rows, tableSize - 1);
    for(int i = tableSize - rows; i > 0; i--)
      {
      this->Internal->append(color);
      }

    this->endInsertRows();
    }
}

void pqColorTableModel::getColor(int idx, QColor &color) const
{
  if(idx >= 0 && idx < this->Internal->size())
    {
    color = this->Internal->at(idx);
    }
}

void pqColorTableModel::getColor(const QModelIndex &idx, QColor &color) const
{
  if(idx.isValid() && idx.model() == this)
    {
    this->getColor(idx.row(), color);
    }
}

void pqColorTableModel::setColor(int idx, const QColor &color)
{
  this->setColor(this->index(idx, 0), color);
}

void pqColorTableModel::setColor(const QModelIndex &idx, const QColor &color)
{
  if(idx.isValid() && idx.model() == this)
    {
    (*this->Internal)[idx.row()] = color;
    emit this->dataChanged(idx, idx);
    emit this->colorChanged(idx.row(), color);
    }
}

void pqColorTableModel::buildGradient(const QModelIndex &first,
    const QModelIndex &last)
{
  if(first.isValid() && first.model() == this && last.isValid() &&
      last.model() == this && first.row() != last.row())
    {
    // Make sure the indexes are in order.
    QModelIndex idx1 = first;
    QModelIndex idx2 = last;
    if(last.row() < first.row())
      {
      idx1 = last;
      idx2 = first;
      }

    // Build the gradient between the two indexes.

    // Notify the view of the changes.
    emit this->dataChanged(idx1, idx2);
    emit this->colorRangeChanged(idx1.row(), idx2.row());
    }
}


