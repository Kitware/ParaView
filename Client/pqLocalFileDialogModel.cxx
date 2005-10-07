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

class FileModel :
  public QAbstractItemModel
{
public:
  void setViewDirectory(const QString& Path)
  {
    ViewDirectory.setPath(QDir::cleanPath(Path));
    fileList = ViewDirectory.entryInfoList();
    
    emit layoutChanged();
    emit dataChanged(QModelIndex(), QModelIndex());
  }

  QString getFilePath(const QModelIndex& Index)
  {
    if(Index.row() >= fileList.size())
      return QString();
      
    QFileInfo& file = fileList[Index.row()];
    return QDir::convertSeparators(ViewDirectory.path() + "/" + file.fileName());
  }

  bool isDir(const QModelIndex& Index)
  {
    if(Index.row() >= fileList.size())
      return false;
      
    QFileInfo& file = fileList[Index.row()];
    return file.isDir();
  }

  int columnCount(const QModelIndex& parent) const
  {
    return 3;
  }

  QVariant data(const QModelIndex & index, int role) const
  {
    if(!index.isValid())
      return QVariant();

    if(index.row() >= fileList.size())
      return QVariant();

    const QFileInfo& file = fileList[index.row()];

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

  QModelIndex index(int row, int column, const QModelIndex& parent) const
  {
    return createIndex(row, column);
  }

  QModelIndex parent(const QModelIndex& index) const
  {
    return QModelIndex();
  }

  int rowCount(const QModelIndex& parent) const
  {
    return fileList.size();
  }

  bool hasChildren(const QModelIndex& parent) const
  {
    if(!parent.isValid())
      return true;
      
    return false;
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const
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

  QDir ViewDirectory;
  QFileInfoList fileList;
};

class FavoriteModel :
  public QAbstractItemModel
{
public:
  FavoriteModel()
  {
    favoriteList = QDir::drives();
    favoriteList.push_back(QFileInfo(QDir::homePath()));
  }

  QString getFilePath(const QModelIndex& Index)
  {
    if(Index.row() >= favoriteList.size())
      return QString();
      
    QFileInfo& file = favoriteList[Index.row()];
    return QDir::convertSeparators(file.absoluteFilePath());
  }

  bool isDir(const QModelIndex& Index)
  {
    if(Index.row() >= favoriteList.size())
      return false;
      
    QFileInfo& file = favoriteList[Index.row()];
    return file.isDir();
  }

  virtual int columnCount(const QModelIndex& parent) const
  {
    return 1;
  }
  
  virtual QVariant data(const QModelIndex & index, int role) const
  {
    if(!index.isValid())
      return QVariant();

    if(index.row() >= favoriteList.size())
      return QVariant();

    const QFileInfo& file = favoriteList[index.row()];
    switch(role)
      {
      case Qt::DisplayRole:
        switch(index.column())
          {
          case 0:
            if(file.isRoot())
              return file.absoluteFilePath();
            else
              return file.fileName();
          }
      case Qt::DecorationRole:
        switch(index.column())
          {
          case 0:
            if(file.isRoot())
              return Icons().icon(QFileIconProvider::Drive);
            if(file.isDir())
              return Icons().icon(QFileIconProvider::Folder);
            return Icons().icon(QFileIconProvider::File);
          }
      }
      
    return QVariant();
  }
  
  virtual QModelIndex index(int row, int column, const QModelIndex& parent) const
  {
    return createIndex(row, column);
  }
  
  virtual QModelIndex parent(const QModelIndex& index) const
  {
    return QModelIndex();
  }
  
  virtual int rowCount(const QModelIndex& parent) const
  {
    return favoriteList.size();
  }
  
  virtual bool hasChildren(const QModelIndex& parent) const
  {
    if(!parent.isValid())
      return true;
      
    return false;
  }
  
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
  {
    switch(role)
      {
      case Qt::DisplayRole:
        switch(section)
          {
          case 0:
            return tr("Favorites");
          }
      }
      
    return QVariant();
  }
  
  QFileInfoList favoriteList;
};

} // namespace

class pqLocalFileDialogModel::Implementation
{
public:
  Implementation() :
    fileModel(new FileModel()),
    favoriteModel(new FavoriteModel())
  {
  }
  
  ~Implementation()
  {
    delete fileModel;
    delete favoriteModel;
  }

  FileModel* const fileModel;
  FavoriteModel* const favoriteModel;
};

pqLocalFileDialogModel::pqLocalFileDialogModel(QObject* Parent) :
  pqFileDialogModel(Parent),
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
  implementation->fileModel->setViewDirectory(Path);
}

QString pqLocalFileDialogModel::getViewDirectory()
{
  return QDir::convertSeparators(implementation->fileModel->ViewDirectory.path());
}

QString pqLocalFileDialogModel::getFilePath(const QModelIndex& Index)
{
  if(Index.model() == implementation->fileModel)
    return implementation->fileModel->getFilePath(Index);
  
  if(Index.model() == implementation->favoriteModel)
    return implementation->favoriteModel->getFilePath(Index);  

  return QString();    
}

bool pqLocalFileDialogModel::isDir(const QModelIndex& Index)
{
  if(Index.model() == implementation->fileModel)
    return implementation->fileModel->isDir(Index);
  
  if(Index.model() == implementation->favoriteModel)
    return implementation->favoriteModel->isDir(Index);  

  return false;    
}

QAbstractItemModel* pqLocalFileDialogModel::fileModel()
{
  return implementation->fileModel;
}

QAbstractItemModel* pqLocalFileDialogModel::favoriteModel()
{
  return implementation->favoriteModel;
}

void pqLocalFileDialogModel::navigateUp()
{
  QDir temp = implementation->fileModel->ViewDirectory;
  temp.cdUp();
  
  setViewDirectory(temp.path());
}

void pqLocalFileDialogModel::navigateDown(const QModelIndex& Index)
{
  if(isDir(Index))
    setViewDirectory(getFilePath(Index));
}

