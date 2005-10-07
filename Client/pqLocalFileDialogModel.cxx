/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqLocalFileDialogModel.h"

#include <QDateTime>
#include <QFileIconProvider>

namespace
{

QFileIconProvider& Icons()
{
  static QFileIconProvider* icons = 0;
  if(!icons)
    icons = new QFileIconProvider();
    
  return *icons;
}

} // namespace

class pqLocalFileDialogModel::Implementation
{
public:
  QDir ViewDirectory;
  QFileInfoList ViewList;
};

pqLocalFileDialogModel::pqLocalFileDialogModel(QObject* Parent) :
  base(Parent),
  implementation(new Implementation())
{
}

pqLocalFileDialogModel::~pqLocalFileDialogModel()
{
  delete implementation;
}

QString pqLocalFileDialogModel::getStartDirectory()
{
  return QDir::currentPath();
}

void pqLocalFileDialogModel::setViewDirectory(const QString& Path)
{
  implementation->ViewDirectory.setPath(QDir::cleanPath(Path));
  implementation->ViewList = implementation->ViewDirectory.entryInfoList();
  
  emit layoutChanged();
  emit dataChanged(QModelIndex(), QModelIndex());
}

QString pqLocalFileDialogModel::getViewDirectory()
{
  return QDir::convertSeparators(implementation->ViewDirectory.path());
}

QString pqLocalFileDialogModel::getFilePath(const QModelIndex& Index)
{
  if(Index.row() >= implementation->ViewList.size())
    return QString();
    
  QFileInfo& file = implementation->ViewList[Index.row()];
  return QDir::convertSeparators(implementation->ViewDirectory.path() + "/" + file.fileName());
}

bool pqLocalFileDialogModel::isDir(const QModelIndex& Index)
{
  if(Index.row() >= implementation->ViewList.size())
    return false;
    
  QFileInfo& file = implementation->ViewList[Index.row()];
  return file.isDir();
}

void pqLocalFileDialogModel::navigateUp()
{
  QDir temp = implementation->ViewDirectory;
  temp.cdUp();
  
  setViewDirectory(temp.path());
}

void pqLocalFileDialogModel::navigateDown(const QModelIndex& Index)
{
  if(Index.row() >= implementation->ViewList.size())
    return;
    
  QFileInfo& file = implementation->ViewList[Index.row()];
  if(file.isDir())
    {
    setViewDirectory(implementation->ViewDirectory.path() + "/" + file.fileName());
    return;
    }
}

int pqLocalFileDialogModel::columnCount(const QModelIndex& parent) const
{
  return 3;
}

QVariant pqLocalFileDialogModel::data(const QModelIndex & index, int role) const
{
  if(!index.isValid())
    return QVariant();

  if(index.row() >= implementation->ViewList.size())
    return QVariant();

  QFileInfo& file = implementation->ViewList[index.row()];

  switch(role)
    {
    case Qt::DisplayRole:
      switch(index.column())
        {
        case 0:
          return file.fileName();
        case 1:
          return file.size();
        case 2:
          return file.lastModified();
        }
    case Qt::DecorationRole:
      switch(index.column())
        {
        case 0:
          return Icons().icon(file.isDir() ? QFileIconProvider::Folder : QFileIconProvider::File);
        }
    }
    
  return QVariant();
}

QModelIndex pqLocalFileDialogModel::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column);
}

QModelIndex pqLocalFileDialogModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int pqLocalFileDialogModel::rowCount(const QModelIndex& parent) const
{
  return implementation->ViewList.size();
}

bool pqLocalFileDialogModel::hasChildren(const QModelIndex& parent) const
{
  if(!parent.isValid())
    return true;
    
  return false;
}

QVariant pqLocalFileDialogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  switch(role)
    {
    case Qt::DisplayRole:
      switch(section)
        {
        case 0:
          return tr("Filename");
        case 1:
          return tr("Size");
        case 2:
          return tr("Modified");
        }
    }
    
  return QVariant();
}

