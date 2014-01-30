/*=========================================================================

   Program: ParaView
   Module:    pqColorPresetModel.cxx

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

/// \file pqColorPresetModel.cxx
/// \date 3/12/2007

#include "pqColorPresetModel.h"

#include "pqChartValue.h"
#include "pqColorMapModel.h"
#include <QList>
#include <QPixmap>
#include <QString>

class pqColorPresetModelItem
{
public:
  pqColorPresetModelItem();
  pqColorPresetModelItem(const pqColorMapModel &colorMap, const QString &name);
  ~pqColorPresetModelItem() {}

  QString Name;
  QPixmap Gradient;
  pqColorMapModel Colors;
  int Id;
};


class pqColorPresetModelInternal 
{
public:
  pqColorPresetModelInternal();
  ~pqColorPresetModelInternal() {}

  QList<pqColorPresetModelItem *> Presets;
  int Builtins;
  int NextId;
};


//----------------------------------------------------------------------------
pqColorPresetModelItem::pqColorPresetModelItem()
  : Name(), Gradient(), Colors()
{
  this->Id = 0;
}

pqColorPresetModelItem::pqColorPresetModelItem(const pqColorMapModel &colorMap,
    const QString &name)
  : Name(name), Gradient(), Colors(colorMap)
{
  this->Id = 0;

  // Use the color map to generate the gradient.
  this->Gradient = this->Colors.generateGradient(QSize(100, 20));
}


//----------------------------------------------------------------------------
pqColorPresetModelInternal::pqColorPresetModelInternal()
  : Presets()
{
  this->Builtins = 0;
  this->NextId = 0;
}


//----------------------------------------------------------------------------
pqColorPresetModel::pqColorPresetModel(QObject *parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Internal = new pqColorPresetModelInternal();
}

pqColorPresetModel::~pqColorPresetModel()
{
  QList<pqColorPresetModelItem *>::Iterator iter =
      this->Internal->Presets.begin();
  for( ; iter != this->Internal->Presets.end(); ++iter)
    {
    delete *iter;
    }

  delete this->Internal;
}

int pqColorPresetModel::rowCount(const QModelIndex &parentIndex) const
{
  if(!parentIndex.isValid())
    {
    return this->Internal->Presets.size();
    }

  return 0;
}

int pqColorPresetModel::columnCount(const QModelIndex &parentIndex) const
{
  if(!parentIndex.isValid())
    {
    return 2;
    }

  return 0;
}

QModelIndex pqColorPresetModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  if(!parentIndex.isValid() && column >= 0 && column < 2 &&
      row >= 0 && row < this->Internal->Presets.size())
    {
    return this->createIndex(row, column);
    }

  return QModelIndex();
}

QModelIndex pqColorPresetModel::parent(const QModelIndex &) const
{
  return QModelIndex();
}

Qt::ItemFlags pqColorPresetModel::flags(const QModelIndex &idx) const
{
  Qt::ItemFlags indexFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if(idx.isValid() && idx.model() == this &&
      this->Internal->Presets[idx.row()]->Id != -1)
    {
    // The default presets are not editable.
    indexFlags |= Qt::ItemIsEditable;
    }

  return indexFlags;
}

QVariant pqColorPresetModel::data(const QModelIndex &idx, int role) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqColorPresetModelItem *item = this->Internal->Presets[idx.row()];
    switch(role)
      {
      case Qt::DisplayRole:
      case Qt::ToolTipRole:
      case Qt::EditRole:
        {
        if(idx.column() == 0)
          {
          return QVariant(item->Name);
          }
        else if(idx.column() == 1)
          {
          switch(item->Colors.getColorSpace())
            {
            case pqColorMapModel::RgbSpace:
              return QVariant("RGB");
            case pqColorMapModel::HsvSpace:
              return QVariant("HSV");
            case pqColorMapModel::WrappedHsvSpace:
              return QVariant("Wrapped HSV");
            case pqColorMapModel::LabSpace:
              return QVariant("CIELAB");
            case pqColorMapModel::DivergingSpace:
              return QVariant("Diverging");
            }
          }

        break;
        }
      case Qt::DecorationRole:
        {
        if(idx.column() == 0)
          {
          return QVariant(item->Gradient);
          }

        break;
        }
      }
    }

  return QVariant();
}

bool pqColorPresetModel::setData(const QModelIndex &idx,
    const QVariant &value, int)
{
  if(idx.isValid() && idx.model() == this && idx.column() == 0)
    {
    this->Internal->Presets[idx.row()]->Name = value.toString();
    this->Modified = true;
    emit this->dataChanged(idx, idx);
    return true;
    }

  return false;
}

QVariant pqColorPresetModel::headerData(int section, Qt::Orientation orient,
    int role) const
{
  if(role == Qt::DisplayRole && orient == Qt::Horizontal)
    {
    if(section == 0)
      {
      return QVariant("Name");
      }
    else if(section == 1)
      {
      return QVariant("Color Space");
      }
    }

  return QVariant();
}

void pqColorPresetModel::addBuiltinColorMap(const pqColorMapModel &colorMap,
    const QString &name)
{
  pqColorPresetModelItem *item = new pqColorPresetModelItem(colorMap, name);
  item->Id = -1;

  // Add the new builtin color map to the appropriate location.
  this->beginInsertRows(QModelIndex(), this->Internal->Builtins,
      this->Internal->Builtins);
  this->Internal->Presets.insert(this->Internal->Builtins, item);
  this->Internal->Builtins++;
  this->endInsertRows();
}

void pqColorPresetModel::addColorMap(const pqColorMapModel &colorMap,
    const QString &name)
{
  pqColorPresetModelItem *item = new pqColorPresetModelItem(colorMap, name);
  item->Id = this->Internal->NextId++;

  // Add the new color map to the end of the list.
  int row = this->Internal->Presets.size();
  this->beginInsertRows(QModelIndex(), row, row);
  this->Internal->Presets.append(item);
  this->Modified = true;
  this->endInsertRows();
}

void pqColorPresetModel::normalizeColorMap(int idx)
{
  if(idx >= 0 && idx < this->Internal->Presets.size())
    {
    pqColorPresetModelItem *item = this->Internal->Presets[idx];
    item->Colors.setValueRange(pqChartValue((double)0.0),
        pqChartValue((double)1.0));
    this->Modified = true;
    }
}

void pqColorPresetModel::removeColorMap(int idx)
{
  if(idx >= 0 && idx < this->Internal->Presets.size())
    {
    pqColorPresetModelItem *item = this->Internal->Presets[idx];
    if(item->Id == -1)
      {
      this->Internal->Builtins--;
      }

    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->Internal->Presets.removeAt(idx);
    this->Modified = true;
    this->endRemoveRows();
    delete item;
    }
}

const pqColorMapModel *pqColorPresetModel::getColorMap(int idx) const
{
  if(idx >= 0 && idx < this->Internal->Presets.size())
    {
    pqColorPresetModelItem *item = this->Internal->Presets[idx];
    return &item->Colors;
    }

  return 0;
}


