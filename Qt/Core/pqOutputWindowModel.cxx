/*=========================================================================

   Program: ParaView
   Module:    pqOutputWindow.cxx

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

#include <QDebug>
#include <QStyleFactory>
#include <QTableView>
#include <QIcon>


#include "pqOutputWindowModel.h"
#include "pqOutputWindow.h"
#include "pqCheckBoxDelegate.h"

#include <iostream>

namespace
{
const int EXPANDED_ROW_EXTRA = 1;
// WARNING: The string order has to match the order in pqOutputWindow::MessageType
const QStyle::StandardPixmap MessageTypeIcon[] =
{
  QStyle::SP_MessageBoxCritical,
  QStyle::SP_MessageBoxWarning,
  QStyle::SP_MessageBoxInformation
};
enum ColumnLocation
{
  COLUMN_EXPANDED,
  COLUMN_TYPE,
  COLUMN_COUNT,
  COLUMN_MESSAGE,
  COUNT
};
};

struct pqOutputWindowModel::pqInternals
{
  QList<QIcon> Icons;
};

pqOutputWindowModel::pqOutputWindowModel(
  QObject *_parent, const QList<MessageT>& messages) :
  QAbstractTableModel(_parent),
  Messages(messages),
  View(NULL),
  Internals(new pqInternals())
{
  Q_ASSERT (EXPANDED_ROW_EXTRA == 1);
}

pqOutputWindowModel::~pqOutputWindowModel()
{
}

int pqOutputWindowModel::rowCount(const QModelIndex &_parent) const
{
  (void)_parent;
  return this->Rows.size();
}

int pqOutputWindowModel::columnCount(const QModelIndex &_parent) const
{
  (void)_parent;
  return COUNT;
}

Qt::ItemFlags pqOutputWindowModel::flags(const QModelIndex & _index) const
{
  int r = _index.row();
  Qt::ItemFlags f;
  if (! this->Messages[this->Rows[r]].Location.isEmpty() ||
      _index.column() != COLUMN_EXPANDED)
    {
    f |= Qt::ItemIsEnabled;
    }
  return f;
}

QVariant pqOutputWindowModel::data(const QModelIndex &_index, int role) const
{
  int r = _index.row();
  switch(role)
    {
    case Qt::DisplayRole:
      {
      if (r - EXPANDED_ROW_EXTRA >= 0 &&
          this->Rows[r] == this->Rows[r - EXPANDED_ROW_EXTRA])
        {
        // row expansion.
        return (_index.column() == COLUMN_MESSAGE) ? 
          this->Messages[this->Rows[r]].Location :
          QVariant();
        }
      else
        {
        // regular row (not an expansion)
        switch (_index.column())
          {
          case COLUMN_EXPANDED:
            return this->Messages[this->Rows[r]].Location.isEmpty() ? 
              pqCheckBoxDelegate::NOT_EXPANDED_DISABLED :
            ((r == this->Rows.size() - 1 || 
              this->Rows[r] != this->Rows[r + 1]) ? 
             pqCheckBoxDelegate::NOT_EXPANDED : pqCheckBoxDelegate::EXPANDED);
          case COLUMN_COUNT:
            return QString::number(this->Messages[this->Rows[r]].Count);
          case COLUMN_MESSAGE:
            return this->Messages[this->Rows[r]].Message;
          }
        return QVariant();
        }
      break;
      }
    case Qt::TextAlignmentRole:
      {      
      if (_index.column() == COLUMN_COUNT)
        {
        return Qt::AlignCenter;
        }
      break;
      }
    case Qt::DecorationRole:
      {
      if ((r - EXPANDED_ROW_EXTRA < 0 ||
           this->Rows[r] != this->Rows[r - EXPANDED_ROW_EXTRA]) &&
          (_index.column() == COLUMN_TYPE))
        {
        return this->Internals->Icons[this->Messages[this->Rows[r]].Type];
        }
      break;
      }
    }
  return QVariant();
}

bool pqOutputWindowModel::setData(const QModelIndex & _index, 
                                  const QVariant & value, int role)
{
  int r = _index.row();  
  if (role == Qt::EditRole)
    {
      if (r - EXPANDED_ROW_EXTRA < 0 ||
          this->Rows[r] != this->Rows[r - EXPANDED_ROW_EXTRA])
        {
        // regular row (not an expansion)
        if (_index.column() == COLUMN_EXPANDED)
          {
          switch (value.toInt())
            {
            case pqCheckBoxDelegate::EXPANDED:
              this->expandRow(r);
              break;
            case pqCheckBoxDelegate::NOT_EXPANDED:
              this->contractRow(r);
              break;
            }
          }
        }
    }
  return true;
}

void pqOutputWindowModel::appendLastRow()
{
  this->beginInsertRows(QModelIndex(), this->Rows.size(), this->Rows.size());
  this->Rows.push_back(this->Messages.size() - 1);
  this->endInsertRows();
  if (this->Rows.size() == 1)
    {
    for (int i = 0; i < 3; ++i)
      {
      this->View->resizeColumnToContents(i);
      }
    }
  this->View->resizeRowToContents(this->Rows.size() - 1);
}

void pqOutputWindowModel::expandRow(int r)
{
  this->beginInsertRows(QModelIndex(), r+1, r+1);
  this->Rows.insert(r+1, this->Rows[r]);
  this->endInsertRows();
  this->View->resizeRowToContents(r+1);
}

void pqOutputWindowModel::contractRow(int r)
{
  beginRemoveRows(QModelIndex(), r+1, r+1);
  this->Rows.removeAt(r+1);
  endRemoveRows();
}


void pqOutputWindowModel::clear()
{
  if (this->Rows.size() > 0)
    {
    beginRemoveRows(QModelIndex(), 0, this->Rows.size() - 1);
    this->Rows.clear();
    endRemoveRows();
    }
}

void pqOutputWindowModel::ShowMessages(bool* show)
{
  this->clear();
  int _rowCount = 0;
  for (int i = 0; i < this->Messages.size(); ++i)
    {
    for (int j = 0; j < pqOutputWindow::MESSAGE_TYPE_COUNT; ++j)
      {
      if (show[j] && this->Messages[i].Type == j)
        {
        ++_rowCount;
        break;
        }
      }
    }
  this->Rows.reserve(_rowCount);
  this->beginInsertRows(QModelIndex(), 0, _rowCount - 1);
  for (int i = 0; i < this->Messages.size(); ++i)
    {
    for (int j = 0; j < pqOutputWindow::MESSAGE_TYPE_COUNT; ++j)
      {
      if (show[j] && this->Messages[i].Type == j)
        {
        this->Rows.push_back(i);
        break;
        }
      }
    }
  this->endInsertRows();
  this->View->resizeRowsToContents();
}

void pqOutputWindowModel::setView(QTableView* view)
{
  Q_ASSERT(this->View == NULL);
  this->View = view;
  QStyle* style = this->View->style();
  // WARNING: the order has to match pqOutputWindow::MessageType
  // error
  this->Internals->Icons.push_back(
    style->standardIcon(QStyle::SP_MessageBoxCritical));
  // warning; warning looks similar with error (also red), so we use a different
  // icon
  this->Internals->Icons.push_back(
    QIcon(":/pqWidgets/Icons/warning.png"));
  // debug
  this->Internals->Icons.push_back(
    style->standardIcon(QStyle::SP_MessageBoxInformation));
}
