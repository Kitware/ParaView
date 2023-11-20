// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqIconListModel.h"

#include <QFileInfo>
#include <QIcon>

struct pqIconListModel::pqInternal
{
  QFileInfoList Icons;

  /**
   * Returns a unique string for the given origin. Not for display purpose.
   */
  static QString iconOrigin(const QString& iconPath)
  {
    if (isResource(iconPath))
    {
      return "Resources";
    }

    return "UserDefined";
  }

  static bool isResource(const QString& iconPath) { return iconPath.startsWith(":"); }
};

//-----------------------------------------------------------------------------
pqIconListModel::pqIconListModel(QObject* parent)
  : Superclass(parent)
  , Internal(new pqInternal())
{
}

//-----------------------------------------------------------------------------
pqIconListModel::~pqIconListModel() = default;

//-----------------------------------------------------------------------------
int pqIconListModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return this->Internal->Icons.size();
}

//-----------------------------------------------------------------------------
QVariant pqIconListModel::data(const QModelIndex& index, int role) const
{
  switch (role)
  {
    case Qt::DisplayRole:
    {
      return this->Internal->Icons[index.row()].completeBaseName();
    }
    case Qt::ToolTipRole:
    {
      return this->Internal->Icons[index.row()].fileName();
    }
    case Qt::DecorationRole:
    {
      return QIcon(this->Internal->Icons[index.row()].absoluteFilePath());
    }
    case OriginRole:
    {
      return pqInternal::iconOrigin(this->Internal->Icons[index.row()].absoluteFilePath());
    }
    case PathRole:
    {
      return this->Internal->Icons[index.row()].absoluteFilePath();
    }
    default:
      break;
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
bool pqIconListModel::insertRows(int row, int count, const QModelIndex& parent)
{
  this->beginInsertRows(parent, row, row + count);
  this->endInsertRows();
  return true;
}

//-----------------------------------------------------------------------------
void pqIconListModel::addIcons(const QFileInfoList& iconInfoList)
{
  this->beginResetModel();
  auto idx = this->rowCount();
  this->Internal->Icons += iconInfoList;
  this->insertRows(idx, iconInfoList.size());
  this->endResetModel();
  Q_EMIT this->dataChanged(this->index(idx), this->index(this->rowCount() - 1));
}

//-----------------------------------------------------------------------------
void pqIconListModel::addIcon(const QFileInfo& iconInfo)
{
  this->addIcons(QFileInfoList() << iconInfo);
}

//-----------------------------------------------------------------------------
void pqIconListModel::clearAll()
{
  this->beginResetModel();
  this->Internal->Icons.clear();
  this->removeRows(0, this->rowCount());
  this->endResetModel();
}

//-----------------------------------------------------------------------------
bool pqIconListModel::isUserDefined(const QModelIndex& index)
{
  auto path = index.data(PathRole).toString();
  return !pqInternal::isResource(path);
}
