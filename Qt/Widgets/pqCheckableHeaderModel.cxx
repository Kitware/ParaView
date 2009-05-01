/*=========================================================================

   Program: ParaView
   Module:    pqCheckableHeaderModel.cxx

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

/// \file pqCheckableHeaderModel.cxx
/// \date 8/17/2007

#include "pqCheckableHeaderModel.h"

#include <QList>
#include <QPixmap>


class pqCheckableHeaderModelItem
{
public:
  pqCheckableHeaderModelItem();
  pqCheckableHeaderModelItem(const pqCheckableHeaderModelItem &other);
  ~pqCheckableHeaderModelItem() {}

  pqCheckableHeaderModelItem &operator=(
      const pqCheckableHeaderModelItem &other);

  QPixmap Pixmap;
  int State;
  int Previous;
  bool Checkable;
};


class pqCheckableHeaderModelInternal
{
public:
  pqCheckableHeaderModelInternal();
  ~pqCheckableHeaderModelInternal() {}

  QList<pqCheckableHeaderModelItem> Horizontal;
  QList<pqCheckableHeaderModelItem> Vertical;
  bool IgnoreUpdate;
};


//----------------------------------------------------------------------------
pqCheckableHeaderModelItem::pqCheckableHeaderModelItem()
  : Pixmap()
{
  this->State = Qt::Unchecked;
  this->Previous = Qt::Unchecked;
  this->Checkable = false;
}

pqCheckableHeaderModelItem::pqCheckableHeaderModelItem(
    const pqCheckableHeaderModelItem &other)
  : Pixmap(other.Pixmap)
{
  this->State = other.State;
  this->Previous = other.Previous;
  this->Checkable = other.Checkable;
}

pqCheckableHeaderModelItem &pqCheckableHeaderModelItem::operator=(
    const pqCheckableHeaderModelItem &other)
{
  this->Pixmap = other.Pixmap;
  this->State = other.State;
  this->Previous = other.Previous;
  this->Checkable = other.Checkable;
  return *this;
}


//----------------------------------------------------------------------------
pqCheckableHeaderModelInternal::pqCheckableHeaderModelInternal()
  : Horizontal(), Vertical()
{
  this->IgnoreUpdate = false;
}


//----------------------------------------------------------------------------
pqCheckableHeaderModel::pqCheckableHeaderModel(QObject *parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Internal = new pqCheckableHeaderModelInternal();
}

pqCheckableHeaderModel::~pqCheckableHeaderModel()
{
  delete this->Internal;
}

QVariant pqCheckableHeaderModel::headerData(int section,
    Qt::Orientation orient, int role) const
{
  pqCheckableHeaderModelItem *item = this->getItem(section, orient);
  if(item && item->Checkable)
    {
    if(role == Qt::CheckStateRole)
      {
      return QVariant(item->State);
      }
    else if(role == Qt::DecorationRole)
      {
      return QVariant(item->Pixmap);
      }
    }

  return QVariant();
}

bool pqCheckableHeaderModel::setHeaderData(int section, Qt::Orientation orient,
    const QVariant &value, int role)
{
  if(role == Qt::CheckStateRole)
    {
    return this->setCheckState(section, orient, value.toInt());
    }
  else if(role == Qt::DecorationRole)
    {
    pqCheckableHeaderModelItem *item = this->getItem(section, orient);
    if(item && item->Checkable && value.canConvert(QVariant::Pixmap))
      {
      item->Pixmap = qvariant_cast<QPixmap>(value);
      emit this->headerDataChanged(orient, section, section);
      return true;
      }
    }

  return false;
}

bool pqCheckableHeaderModel::isCheckable(int section,
    Qt::Orientation orient) const
{
  pqCheckableHeaderModelItem *item = this->getItem(section, orient);
  return item && item->Checkable;
}

void pqCheckableHeaderModel::setCheckable(int section, Qt::Orientation orient,
    bool checkable)
{
  pqCheckableHeaderModelItem *item = this->getItem(section, orient);
  if(item && item->Checkable != checkable)
    {
    item->Checkable = checkable;
    if(!item->Checkable)
      {
      item->State = Qt::Unchecked;
      item->Previous = Qt::Unchecked;
      }

    emit this->headerDataChanged(orient, section, section);
    }
}

int pqCheckableHeaderModel::getCheckState(int section,
    Qt::Orientation orient) const
{
  pqCheckableHeaderModelItem *item = this->getItem(section, orient);
  if(item && item->Checkable)
    {
    return item->State;
    }

  return Qt::Unchecked;
}

bool pqCheckableHeaderModel::setCheckState(int section, Qt::Orientation orient,
    int state)
{
  pqCheckableHeaderModelItem *item = this->getItem(section, orient);
  if(item && item->Checkable)
    {
    if(item->State != state)
      {
      item->State = state;
      emit this->headerDataChanged(orient, section, section);
      }

    return true;
    }

  return false;
}

void pqCheckableHeaderModel::updateCheckState(int section,
    Qt::Orientation orient)
{
  if(this->Internal->IgnoreUpdate)
    {
    return;
    }

  pqCheckableHeaderModelItem *item = this->getItem(section, orient);
  if(item && item->Checkable)
    {
    // Loop through the model indexes for the given section to
    // determine the check state.
    bool found = false;
    int state = Qt::Unchecked;
    bool incrementRow = orient == Qt::Horizontal;
    int total = incrementRow ? this->rowCount() : this->columnCount();
    for(int i = 0; i < total; i++)
      {
      int row = incrementRow ? i : section;
      int column = incrementRow ? section : i;
      QModelIndex idx = this->index(row, column);
      if(this->flags(idx) & Qt::ItemIsUserCheckable)
        {
        int indexState = this->data(idx, Qt::CheckStateRole).toInt();
        if(!found)
          {
          state = indexState;
          found = true;
          }
        else if(indexState != state)
          {
          state = Qt::PartiallyChecked;
          break;
          }
        }
      }

    if(item->State != state)
      {
      item->State = state;
      item->Previous = item->State;
      this->beginMultiStateChange();
      emit this->headerDataChanged(orient, section, section);
      this->endMultipleStateChange();
      }
    }
}

int pqCheckableHeaderModel::getNumberOfHeaderSections(
    Qt::Orientation orient) const
{
  if(orient == Qt::Horizontal)
    {
    return this->Internal->Horizontal.size();
    }
  else
    {
    return this->Internal->Vertical.size();
    }
}

void pqCheckableHeaderModel::clearHeaderSections(Qt::Orientation orient)
{
  if(orient == Qt::Horizontal)
    {
    this->Internal->Horizontal.clear();
    }
  else
    {
    this->Internal->Vertical.clear();
    }
}

void pqCheckableHeaderModel::insertHeaderSections(Qt::Orientation orient,
    int first, int last)
{
  if(first < 0)
    {
    return;
    }

  QList<pqCheckableHeaderModelItem> *list = 0;
  if(orient == Qt::Horizontal)
    {
    list = &this->Internal->Horizontal;
    }
  else
    {
    list = &this->Internal->Vertical;
    }

  bool doAdd = first >= list->size();
  for(int i = first; i <= last; i++)
    {
    if(doAdd)
      {
      list->append(pqCheckableHeaderModelItem());
      }
    else
      {
      list->insert(i, pqCheckableHeaderModelItem());
      }
    }
}

void pqCheckableHeaderModel::removeHeaderSections(Qt::Orientation orient,
    int first, int last)
{
  QList<pqCheckableHeaderModelItem> *list = 0;
  if(orient == Qt::Horizontal)
    {
    list = &this->Internal->Horizontal;
    }
  else
    {
    list = &this->Internal->Vertical;
    }

  if(last >= list->size())
    {
    last = list->size() - 1;
    }

  if(first >= 0 && first <= last)
    {
    for(int i = last; i >= first; i--)
      {
      list->removeAt(i);
      }
    }
}

void pqCheckableHeaderModel::beginMultiStateChange()
{
  this->Internal->IgnoreUpdate = true;
}

void pqCheckableHeaderModel::endMultipleStateChange()
{
  this->Internal->IgnoreUpdate = false;
}

void pqCheckableHeaderModel::setIndexCheckState(Qt::Orientation orient,
    int first, int last)
{
  if (this->Internal->IgnoreUpdate)
    {
    return;
    }
  this->beginMultiStateChange();
  for(int section = first; section <= last; section++)
    {
    pqCheckableHeaderModelItem *item = this->getItem(section, orient);
    if(item && item->Checkable && item->State != item->Previous)
      {
      // Update the check state of the model indexes if needed.
      if(item->State != Qt::PartiallyChecked)
        {
        bool incrementRow = orient == Qt::Horizontal;
        int total = incrementRow ? this->rowCount() : this->columnCount();
        for(int i = 0; i < total; i++)
          {
          int row = incrementRow ? i : section;
          int column = incrementRow ? section : i;
          QModelIndex idx = this->index(row, column);
          if(this->flags(idx) & Qt::ItemIsUserCheckable)
            {
            this->setData(idx, QVariant(item->State), Qt::CheckStateRole);
            }
          }
        }

      item->Previous = item->State;
      }
    }

  this->endMultipleStateChange();
}

pqCheckableHeaderModelItem *pqCheckableHeaderModel::getItem(int section,
    Qt::Orientation orient) const
{
  if(orient == Qt::Horizontal)
    {
    if(section >= 0 && section < this->Internal->Horizontal.size())
      {
      return &this->Internal->Horizontal[section];
      }
    }
  else
    {
    if(section >= 0 && section < this->Internal->Vertical.size())
      {
      return &this->Internal->Vertical[section];
      }
    }

  return 0;
}


